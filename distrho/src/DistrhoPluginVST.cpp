/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
#endif

#ifndef __cdecl
# define __cdecl
#endif

#define VESTIGE_HEADER
#define VST_FORCE_DEPRECATED 0

#include <clocale>
#include <map>
#include <string>

#ifdef VESTIGE_HEADER
# include "vestige/vestige.h"
#define effFlagsProgramChunks (1 << 5)
#define effSetProgramName 4
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effGetProgramNameIndexed 29
#define effGetPlugCategory 35
#define effEditKeyDown 59
#define effEditKeyUp 60
#define kVstVersion 2400
struct ERect {
    int16_t top, left, bottom, right;
};
#else
# include "vst/aeffectx.h"
#endif

START_NAMESPACE_DISTRHO

typedef std::map<const String, String> StringMap;

static const int kVstMidiEventSize = static_cast<int>(sizeof(VstMidiEvent));

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static const writeMidiFunc writeMidiCallback = nullptr;
#endif

// -----------------------------------------------------------------------

void strncpy(char* const dst, const char* const src, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    if (const size_t len = std::min(std::strlen(src), size-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

void snprintf_param(char* const dst, const float value, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, "%f", value);
    dst[size-1] = '\0';
}

void snprintf_iparam(char* const dst, const int32_t value, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, "%d", value);
    dst[size-1] = '\0';
}

// -----------------------------------------------------------------------

class ScopedSafeLocale {
public:
    ScopedSafeLocale() noexcept
        : locale(::strdup(::setlocale(LC_NUMERIC, nullptr)))
    {
        ::setlocale(LC_NUMERIC, "C");
    }

    ~ScopedSafeLocale() noexcept
    {
        if (locale != nullptr)
        {
            ::setlocale(LC_NUMERIC, locale);
            std::free(locale);
        }
    }

private:
    char* const locale;

    DISTRHO_DECLARE_NON_COPY_CLASS(ScopedSafeLocale)
    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

class ParameterCheckHelper
{
public:
    ParameterCheckHelper()
        : parameterChecks(nullptr),
          parameterValues(nullptr) {}

    virtual ~ParameterCheckHelper()
    {
        if (parameterChecks != nullptr)
        {
            delete[] parameterChecks;
            parameterChecks = nullptr;
        }
        if (parameterValues != nullptr)
        {
            delete[] parameterValues;
            parameterValues = nullptr;
        }
    }

    bool*  parameterChecks;
    float* parameterValues;

#if DISTRHO_PLUGIN_WANT_STATE
    virtual void setStateFromUI(const char* const newKey, const char* const newValue) = 0;
#endif
};

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------

class UIVst
{
public:
    UIVst(const audioMasterCallback audioMaster, AEffect* const effect, ParameterCheckHelper* const uiHelper, PluginExporter* const plugin, const intptr_t winId, const float scaleFactor)
        : fAudioMaster(audioMaster),
          fEffect(effect),
          fUiHelper(uiHelper),
          fPlugin(plugin),
          fUI(this, winId, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, setSizeCallback, scaleFactor, plugin->getInstancePointer()),
          fShouldCaptureVstKeys(false)
    {
        // FIXME only needed for windows?
//#ifdef DISTRHO_OS_WINDOWS
        char strBuf[0xff+1];
        std::memset(strBuf, 0, sizeof(char)*(0xff+1));
        hostCallback(audioMasterGetProductString, 0, 0, strBuf);
        d_stdout("Plugin UI running in '%s'", strBuf);

        // TODO make a white-list of needed hosts
        if (/*std::strcmp(strBuf, "") == 0*/ true)
            fShouldCaptureVstKeys = true;
//#endif
    }

    // -------------------------------------------------------------------

    void idle()
    {
        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
        {
            if (fUiHelper->parameterChecks[i])
            {
                fUiHelper->parameterChecks[i] = false;
                fUI.parameterChanged(i, fUiHelper->parameterValues[i]);
            }
        }

        fUI.idle();
    }

    int16_t getWidth() const
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const
    {
        return fUI.getHeight();
    }

    void setSampleRate(const double newSampleRate)
    {
        fUI.setSampleRate(newSampleRate, true);
    }

    // -------------------------------------------------------------------
    // functions called from the plugin side, may block

# if DISTRHO_PLUGIN_WANT_STATE
    void setStateFromPlugin(const char* const key, const char* const value)
    {
        fUI.stateChanged(key, value);
    }
# endif

    int handlePluginKeyEvent(const bool down, int32_t index, const intptr_t value)
    {
# if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        if (! fShouldCaptureVstKeys)
            return 0;

        d_stdout("handlePluginKeyEvent %i %i %li\n", down, index, (long int)value);

        int special = 0;
        switch (value)
        {
        // convert some specials to normal keys
        case  1: index = kCharBackspace; break;
        case  6: index = kCharEscape;    break;
        case  7: index = ' ';            break;
        case 22: index = kCharDelete;    break;

        // handle rest of special keys
        case 40: special = kKeyF1;       break;
        case 41: special = kKeyF2;       break;
        case 42: special = kKeyF3;       break;
        case 43: special = kKeyF4;       break;
        case 44: special = kKeyF5;       break;
        case 45: special = kKeyF6;       break;
        case 46: special = kKeyF7;       break;
        case 47: special = kKeyF8;       break;
        case 48: special = kKeyF9;       break;
        case 49: special = kKeyF10;      break;
        case 50: special = kKeyF11;      break;
        case 51: special = kKeyF12;      break;
        case 11: special = kKeyLeft;     break;
        case 12: special = kKeyUp;       break;
        case 13: special = kKeyRight;    break;
        case 14: special = kKeyDown;     break;
        case 15: special = kKeyPageUp;   break;
        case 16: special = kKeyPageDown; break;
        case 10: special = kKeyHome;     break;
        case  9: special = kKeyEnd;      break;
        case 21: special = kKeyInsert;   break;
        case 54: special = kKeyShift;    break;
        case 55: special = kKeyControl;  break;
        case 56: special = kKeyAlt;      break;
        }

        if (special != 0)
            return fUI.handlePluginSpecial(down, static_cast<Key>(special));

        if (index >= 0)
            return fUI.handlePluginKeyboard(down, static_cast<uint>(index));
# endif

        return 0;
    }

    // -------------------------------------------------------------------

protected:
    intptr_t hostCallback(const int32_t opcode,
                          const int32_t index = 0,
                          const intptr_t value = 0,
                          void* const ptr = nullptr,
                          const float opt = 0.0f)
    {
        return fAudioMaster(fEffect, opcode, index, value, ptr, opt);
    }

    void editParameter(const uint32_t index, const bool started)
    {
        hostCallback(started ? audioMasterBeginEdit : audioMasterEndEdit, index);
    }

    void setParameterValue(const uint32_t index, const float realValue)
    {
        const ParameterRanges& ranges(fPlugin->getParameterRanges(index));
        const float perValue(ranges.getNormalizedValue(realValue));

        fPlugin->setParameterValue(index, realValue);
        hostCallback(audioMasterAutomate, index, 0, nullptr, perValue);
    }

    void setState(const char* const key, const char* const value)
    {
# if DISTRHO_PLUGIN_WANT_STATE
        fUiHelper->setStateFromUI(key, value);
# else
        return; // unused
        (void)key;
        (void)value;
# endif
    }

    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
# if 0 //DISTRHO_PLUGIN_WANT_MIDI_INPUT
        // TODO
# else
        return; // unused
        (void)channel;
        (void)note;
        (void)velocity;
# endif
    }

    void setSize(const uint width, const uint height)
    {
        fUI.setWindowSize(width, height);
        hostCallback(audioMasterSizeWindow, width, height);
    }

private:
    // Vst stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;
    ParameterCheckHelper* const fUiHelper;
    PluginExporter* const fPlugin;

    // Plugin UI
    UIExporter fUI;
    bool fShouldCaptureVstKeys;

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((UIVst*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->editParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->setState(key, value);
    }

    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(channel, note, velocity);
    }

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        handlePtr->setSize(width, height);
    }

    #undef handlePtr
};
#endif

// -----------------------------------------------------------------------

class PluginVst : public ParameterCheckHelper
{
public:
    PluginVst(const audioMasterCallback audioMaster, AEffect* const effect)
        : fPlugin(this, writeMidiCallback),
          fAudioMaster(audioMaster),
          fEffect(effect)
    {
        std::memset(fProgramName, 0, sizeof(char)*(32+1));
        std::strcpy(fProgramName, "Default");

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fMidiEventCount = 0;
#endif

#if DISTRHO_PLUGIN_HAS_UI
        fVstUI          = nullptr;
        fVstRect.top    = 0;
        fVstRect.left   = 0;
        fVstRect.bottom = 0;
        fVstRect.right  = 0;

        if (const uint32_t paramCount = fPlugin.getParameterCount())
        {
            parameterChecks = new bool[paramCount];
            parameterValues = new float[paramCount];

            for (uint32_t i=0; i < paramCount; ++i)
            {
                parameterChecks[i] = false;
                parameterValues[i] = NAN;
            }
        }
# if DISTRHO_OS_MAC
#  ifdef __LP64__
        fUsingNsView = true;
#  else
#   ifndef DISTRHO_NO_WARNINGS
#    warning 32bit VST UIs on OSX only work if the host supports "hasCockosViewAsConfig"
#   endif
        fUsingNsView = false;
#  endif
# endif // DISTRHO_OS_MAC
#endif // DISTRHO_PLUGIN_HAS_UI

#if DISTRHO_PLUGIN_WANT_STATE
        fStateChunk = nullptr;

        for (uint32_t i=0, count=fPlugin.getStateCount(); i<count; ++i)
        {
            const String& dkey(fPlugin.getStateKey(i));
            fStateMap[dkey] = fPlugin.getStateDefaultValue(i);
        }
#endif
    }

    ~PluginVst()
    {
#if DISTRHO_PLUGIN_WANT_STATE
        if (fStateChunk != nullptr)
        {
            delete[] fStateChunk;
            fStateChunk = nullptr;
        }

        fStateMap.clear();
#endif
    }

    intptr_t vst_dispatcher(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        intptr_t ret = 0;
#endif

        switch (opcode)
        {
        case effGetProgram:
            return 0;

        case effSetProgramName:
            if (char* const programName = (char*)ptr)
            {
                DISTRHO_NAMESPACE::strncpy(fProgramName, programName, 32);
                return 1;
            }
            break;

        case effGetProgramName:
            if (char* const programName = (char*)ptr)
            {
                DISTRHO_NAMESPACE::strncpy(programName, fProgramName, 24);
                return 1;
            }
            break;

        case effGetProgramNameIndexed:
            if (char* const programName = (char*)ptr)
            {
                DISTRHO_NAMESPACE::strncpy(programName, fProgramName, 24);
                return 1;
            }
            break;

        case effGetParamDisplay:
            if (ptr != nullptr && index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                const uint32_t hints = fPlugin.getParameterHints(index);
                float value = fPlugin.getParameterValue(index);

                if (hints & kParameterIsBoolean)
                {
                    const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
                    const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;

                    value = value > midRange ? ranges.max : ranges.min;
                }
                else if (hints & kParameterIsInteger)
                {
                    value = std::round(value);
                }

                const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(index));

                for (uint8_t i = 0; i < enumValues.count; ++i)
                {
                    if (d_isNotEqual(value, enumValues.values[i].value))
                        continue;

                    DISTRHO_NAMESPACE::strncpy((char*)ptr, enumValues.values[i].label.buffer(), 24);
                    return 1;
                }

                if (hints & kParameterIsInteger)
                    DISTRHO_NAMESPACE::snprintf_iparam((char*)ptr, (int32_t)value, 24);
                else
                    DISTRHO_NAMESPACE::snprintf_param((char*)ptr, value, 24);

                return 1;
            }
            break;

        case effSetSampleRate:
            fPlugin.setSampleRate(opt, true);

#if DISTRHO_PLUGIN_HAS_UI
            if (fVstUI != nullptr)
                fVstUI->setSampleRate(opt);
#endif
            break;

        case effSetBlockSize:
            fPlugin.setBufferSize(value, true);
            break;

        case effMainsChanged:
            if (value != 0)
            {
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                fMidiEventCount = 0;

                // tell host we want MIDI events
                hostCallback(audioMasterWantMidi);
#endif

                // deactivate for possible changes
                fPlugin.deactivateIfNeeded();

                // check if something changed
                const uint32_t bufferSize = static_cast<uint32_t>(hostCallback(audioMasterGetBlockSize));
                const double   sampleRate = static_cast<double>(hostCallback(audioMasterGetSampleRate));

                if (bufferSize != 0)
                    fPlugin.setBufferSize(bufferSize, true);

                if (sampleRate != 0.0)
                    fPlugin.setSampleRate(sampleRate, true);

                fPlugin.activate();
            }
            else
            {
                fPlugin.deactivate();
            }
            break;

#if DISTRHO_PLUGIN_HAS_UI
        case effEditGetRect:
            if (fVstUI != nullptr)
            {
                fVstRect.right  = fVstUI->getWidth();
                fVstRect.bottom = fVstUI->getHeight();
            }
            else
            {
                d_lastUiSampleRate = fPlugin.getSampleRate();

                // TODO
                const float scaleFactor = 1.0f;

                UIExporter tmpUI(nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr, scaleFactor, fPlugin.getInstancePointer());
                fVstRect.right  = tmpUI.getWidth();
                fVstRect.bottom = tmpUI.getHeight();
                tmpUI.quit();
            }
            *(ERect**)ptr = &fVstRect;
            return 1;

        case effEditOpen:
            if (fVstUI == nullptr)
            {
# if DISTRHO_OS_MAC
                if (! fUsingNsView)
                {
                    d_stderr("Host doesn't support hasCockosViewAsConfig, cannot use UI");
                    return 0;
                }
# endif
                d_lastUiSampleRate = fPlugin.getSampleRate();

                // TODO
                const float scaleFactor = 1.0f;

                fVstUI = new UIVst(fAudioMaster, fEffect, this, &fPlugin, (intptr_t)ptr, scaleFactor);

# if DISTRHO_PLUGIN_WANT_FULL_STATE
                // Update current state from plugin side
                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key = cit->first;
                    fStateMap[key] = fPlugin.getState(key);
                }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
                // Set state
                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key   = cit->first;
                    const String& value = cit->second;

                    fVstUI->setStateFromPlugin(key, value);
                }
# endif
                for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
                    setParameterValueFromPlugin(i, fPlugin.getParameterValue(i));

                fVstUI->idle();
                return 1;
            }
            break;

        case effEditClose:
            if (fVstUI != nullptr)
            {
                delete fVstUI;
                fVstUI = nullptr;
                return 1;
            }
            break;

        //case effIdle:
        case effEditIdle:
            if (fVstUI != nullptr)
                fVstUI->idle();
            break;

        case effEditKeyDown:
            if (fVstUI != nullptr)
                return fVstUI->handlePluginKeyEvent(true, index, value);
            break;

        case effEditKeyUp:
            if (fVstUI != nullptr)
                return fVstUI->handlePluginKeyEvent(false, index, value);
            break;
#endif // DISTRHO_PLUGIN_HAS_UI

#if DISTRHO_PLUGIN_WANT_STATE
        case effGetChunk:
        {
            if (ptr == nullptr)
                return 0;

            if (fStateChunk != nullptr)
            {
                delete[] fStateChunk;
                fStateChunk = nullptr;
            }

            const uint32_t paramCount = fPlugin.getParameterCount();

            if (fPlugin.getStateCount() == 0 && paramCount == 0)
            {
                fStateChunk    = new char[1];
                fStateChunk[0] = '\0';
                ret = 1;
            }
            else
            {
# if DISTRHO_PLUGIN_WANT_FULL_STATE
                // Update current state
                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key = cit->first;
                    fStateMap[key] = fPlugin.getState(key);
                }
# endif

                String chunkStr;

                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key   = cit->first;
                    const String& value = cit->second;

                    // join key and value
                    String tmpStr;
                    tmpStr  = key;
                    tmpStr += "\xff";
                    tmpStr += value;
                    tmpStr += "\xff";

                    chunkStr += tmpStr;
                }

                if (paramCount != 0)
                {
                    // add another separator
                    chunkStr += "\xff";

                    // temporarily set locale to "C" while converting floats
                    const ScopedSafeLocale ssl;

                    for (uint32_t i=0; i<paramCount; ++i)
                    {
                        if (fPlugin.isParameterOutputOrTrigger(i))
                            continue;

                        // join key and value
                        String tmpStr;
                        tmpStr  = fPlugin.getParameterSymbol(i);
                        tmpStr += "\xff";
                        tmpStr += String(fPlugin.getParameterValue(i));
                        tmpStr += "\xff";

                        chunkStr += tmpStr;
                    }
                }

                const std::size_t chunkSize(chunkStr.length()+1);

                fStateChunk = new char[chunkSize];
                std::memcpy(fStateChunk, chunkStr.buffer(), chunkStr.length());
                fStateChunk[chunkSize-1] = '\0';

                for (std::size_t i=0; i<chunkSize; ++i)
                {
                    if (fStateChunk[i] == '\xff')
                        fStateChunk[i] = '\0';
                }

                ret = chunkSize;
            }

            *(void**)ptr = fStateChunk;
            return ret;
        }

        case effSetChunk:
        {
            if (value <= 1 || ptr == nullptr)
                return 0;

            const size_t chunkSize = static_cast<size_t>(value);

            const char* key   = (const char*)ptr;
            const char* value = nullptr;
            size_t size, bytesRead = 0;

            while (bytesRead < chunkSize)
            {
                if (key[0] == '\0')
                    break;

                size  = std::strlen(key)+1;
                value = key + size;
                bytesRead += size;

                setStateFromUI(key, value);

# if DISTRHO_PLUGIN_HAS_UI
                if (fVstUI != nullptr)
                    fVstUI->setStateFromPlugin(key, value);
# endif

                // get next key
                size = std::strlen(value)+1;
                key  = value + size;
                bytesRead += size;
            }

            const uint32_t paramCount = fPlugin.getParameterCount();

            if (bytesRead+4 < chunkSize && paramCount != 0)
            {
                ++key;
                float fvalue;

                // temporarily set locale to "C" while converting floats
                const ScopedSafeLocale ssl;

                while (bytesRead < chunkSize)
                {
                    if (key[0] == '\0')
                        break;

                    size  = std::strlen(key)+1;
                    value = key + size;
                    bytesRead += size;

                    // find parameter with this symbol, and set its value
                    for (uint32_t i=0; i<paramCount; ++i)
                    {
                        if (fPlugin.isParameterOutputOrTrigger(i))
                            continue;
                        if (fPlugin.getParameterSymbol(i) != key)
                            continue;

                        fvalue = std::atof(value);
                        fPlugin.setParameterValue(i, fvalue);
# if DISTRHO_PLUGIN_HAS_UI
                        if (fVstUI != nullptr)
                            setParameterValueFromPlugin(i, fvalue);
# endif
                        break;
                    }

                    // get next key
                    size = std::strlen(value)+1;
                    key  = value + size;
                    bytesRead += size;
                }
            }

            return 1;
        }
#endif // DISTRHO_PLUGIN_WANT_STATE

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        case effProcessEvents:
            if (! fPlugin.isActive())
            {
                // host has not activated the plugin yet, nasty!
                vst_dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
            }

            if (const VstEvents* const events = (const VstEvents*)ptr)
            {
                if (events->numEvents == 0)
                    break;

                for (int i=0, count=events->numEvents; i < count; ++i)
                {
                    const VstMidiEvent* const vstMidiEvent((const VstMidiEvent*)events->events[i]);

                    if (vstMidiEvent == nullptr)
                        break;
                    if (vstMidiEvent->type != kVstMidiType)
                        continue;
                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;

                    MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
                    midiEvent.frame  = vstMidiEvent->deltaFrames;
                    midiEvent.size   = 3;
                    std::memcpy(midiEvent.data, vstMidiEvent->midiData, sizeof(uint8_t)*3);
                }
            }
            break;
#endif

        case effCanBeAutomated:
            if (index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                const uint32_t hints(fPlugin.getParameterHints(index));

                // must be automable, and not output
                if ((hints & kParameterIsAutomable) != 0 && (hints & kParameterIsOutput) == 0)
                    return 1;
            }
            break;

        case effCanDo:
            if (const char* const canDo = (const char*)ptr)
            {
#if DISTRHO_OS_MAC && DISTRHO_PLUGIN_HAS_UI
                if (std::strcmp(canDo, "hasCockosViewAsConfig") == 0)
                {
                    fUsingNsView = true;
                    return 0xbeef0000;
                }
#endif
                if (std::strcmp(canDo, "receiveVstEvents") == 0 ||
                    std::strcmp(canDo, "receiveVstMidiEvent") == 0)
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                    return 1;
#else
                    return -1;
#endif
                if (std::strcmp(canDo, "sendVstEvents") == 0 ||
                    std::strcmp(canDo, "sendVstMidiEvent") == 0)
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
                    return 1;
#else
                    return -1;
#endif
                if (std::strcmp(canDo, "receiveVstTimeInfo") == 0)
#if DISTRHO_PLUGIN_WANT_TIMEPOS
                    return 1;
#else
                    return -1;
#endif
            }
            break;

        //case effStartProcess:
        //case effStopProcess:
        // unused
        //    break;
        }

        return 0;
    }

    float vst_getParameter(const int32_t index)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        return ranges.getNormalizedValue(fPlugin.getParameterValue(index));
    }

    void vst_setParameter(const int32_t index, float value)
    {
        const uint32_t hints(fPlugin.getParameterHints(index));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        value = ranges.getUnnormalizedValue(value);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            value = value > midRange ? ranges.max : ranges.min;
        }

        if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        const float realValue(value);
        fPlugin.setParameterValue(index, realValue);

#if DISTRHO_PLUGIN_HAS_UI
        if (fVstUI != nullptr)
            setParameterValueFromPlugin(index, realValue);
#endif
    }

    void vst_processReplacing(const float** const inputs, float** const outputs, const int32_t sampleFrames)
    {
        if (sampleFrames <= 0)
        {
            updateParameterOutputsAndTriggers();
            return;
        }

        if (! fPlugin.isActive())
        {
            // host has not activated the plugin yet, nasty!
            vst_dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
        }

#if DISTRHO_PLUGIN_WANT_TIMEPOS
        static const int kWantVstTimeFlags(kVstTransportPlaying|kVstPpqPosValid|kVstTempoValid|kVstTimeSigValid);

        if (const VstTimeInfo* const vstTimeInfo = (const VstTimeInfo*)hostCallback(audioMasterGetTime, 0, kWantVstTimeFlags))
        {
            fTimePosition.frame     =   vstTimeInfo->samplePos;
            fTimePosition.playing   =  (vstTimeInfo->flags & kVstTransportPlaying);
            fTimePosition.bbt.valid = ((vstTimeInfo->flags & kVstTempoValid) != 0 || (vstTimeInfo->flags & kVstTimeSigValid) != 0);

            // ticksPerBeat is not possible with VST
            fTimePosition.bbt.ticksPerBeat = 960.0;

            if (vstTimeInfo->flags & kVstTempoValid)
                fTimePosition.bbt.beatsPerMinute = vstTimeInfo->tempo;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            if (vstTimeInfo->flags & (kVstPpqPosValid|kVstTimeSigValid))
            {
                const double ppqPos    = std::abs(vstTimeInfo->ppqPos);
                const int    ppqPerBar = vstTimeInfo->timeSigNumerator * 4 / vstTimeInfo->timeSigDenominator;
                const double barBeats  = (std::fmod(ppqPos, ppqPerBar) / ppqPerBar) * vstTimeInfo->timeSigNumerator;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimePosition.bbt.bar         = static_cast<int32_t>(ppqPos) / ppqPerBar + 1;
                fTimePosition.bbt.beat        = static_cast<int32_t>(barBeats - rest + 0.5) + 1;
                fTimePosition.bbt.tick        = static_cast<int32_t>(rest * fTimePosition.bbt.ticksPerBeat + 0.5);
                fTimePosition.bbt.beatsPerBar = vstTimeInfo->timeSigNumerator;
                fTimePosition.bbt.beatType    = vstTimeInfo->timeSigDenominator;

                if (vstTimeInfo->ppqPos < 0.0)
                {
                    --fTimePosition.bbt.bar;
                    fTimePosition.bbt.beat = vstTimeInfo->timeSigNumerator - fTimePosition.bbt.beat + 1;
                    fTimePosition.bbt.tick = int(fTimePosition.bbt.ticksPerBeat) - fTimePosition.bbt.tick - 1;
                }
            }
            else
            {
                fTimePosition.bbt.bar         = 1;
                fTimePosition.bbt.beat        = 1;
                fTimePosition.bbt.tick        = 0;
                fTimePosition.bbt.beatsPerBar = 4.0f;
                fTimePosition.bbt.beatType    = 4.0f;
            }

            fTimePosition.bbt.barStartTick = fTimePosition.bbt.ticksPerBeat*fTimePosition.bbt.beatsPerBar*(fTimePosition.bbt.bar-1);

            fPlugin.setTimePosition(fTimePosition);
        }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fPlugin.run(inputs, outputs, sampleFrames, fMidiEvents, fMidiEventCount);
        fMidiEventCount = 0;
#else
        fPlugin.run(inputs, outputs, sampleFrames);
#endif

        updateParameterOutputsAndTriggers();
    }

    // -------------------------------------------------------------------

    friend class UIVst;

private:
    // Plugin
    PluginExporter fPlugin;

    // VST stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;

    // Temporary data
    char fProgramName[32+1];

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
#endif

    // UI stuff
#if DISTRHO_PLUGIN_HAS_UI
    UIVst* fVstUI;
    ERect  fVstRect;
# if DISTRHO_OS_MAC
    bool fUsingNsView;
# endif
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    char*     fStateChunk;
    StringMap fStateMap;
#endif

    // -------------------------------------------------------------------
    // host callback

    intptr_t hostCallback(const int32_t opcode,
                          const int32_t index = 0,
                          const intptr_t value = 0,
                          void* const ptr = nullptr,
                          const float opt = 0.0f)
    {
        return fAudioMaster(fEffect, opcode, index, value, ptr, opt);
    }

    // -------------------------------------------------------------------
    // functions called from the plugin side, RT no block

    void updateParameterOutputsAndTriggers()
    {
        float curValue;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
            {
                // NOTE: no output parameter support in VST, simulate it here
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, parameterValues[i]))
                    continue;

#if DISTRHO_PLUGIN_HAS_UI
                if (fVstUI != nullptr)
                    setParameterValueFromPlugin(i, curValue);
                else
#endif
                parameterValues[i] = curValue;

#ifndef DPF_VST_SHOW_PARAMETER_OUTPUTS
                // skip automating parameter outputs from plugin if we disable them on VST
                continue;
#endif
            }
            else if ((fPlugin.getParameterHints(i) & kParameterIsTrigger) == kParameterIsTrigger)
            {
                // NOTE: no trigger support in VST parameters, simulate it here
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, fPlugin.getParameterRanges(i).def))
                    continue;

#if DISTRHO_PLUGIN_HAS_UI
                if (fVstUI != nullptr)
                    setParameterValueFromPlugin(i, curValue);
#endif
                fPlugin.setParameterValue(i, curValue);
            }
            else
            {
                continue;
            }

            const ParameterRanges& ranges(fPlugin.getParameterRanges(i));
            hostCallback(audioMasterAutomate, i, 0, nullptr, ranges.getNormalizedValue(curValue));
        }
    }

#if DISTRHO_PLUGIN_HAS_UI
    void setParameterValueFromPlugin(const uint32_t index, const float realValue)
    {
        parameterValues[index] = realValue;
        parameterChecks[index] = true;
    }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        if (midiEvent.size > 4)
            return true;

        VstEvents vstEvents;
        std::memset(&vstEvents, 0, sizeof(VstEvents));

        VstMidiEvent vstMidiEvent;
        std::memset(&vstMidiEvent, 0, sizeof(VstMidiEvent));

        vstEvents.numEvents = 1;
        vstEvents.events[0] = (VstEvent*)&vstMidiEvent;

        vstMidiEvent.type = kVstMidiType;
        vstMidiEvent.byteSize    = kVstMidiEventSize;
        vstMidiEvent.deltaFrames = midiEvent.frame;

        for (uint8_t i=0; i<midiEvent.size; ++i)
            vstMidiEvent.midiData[i] = midiEvent.data[i];

        return hostCallback(audioMasterProcessEvents, 0, 0, &vstEvents) == 1;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return ((PluginVst*)ptr)->writeMidi(midiEvent);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    // -------------------------------------------------------------------
    // functions called from the UI side, may block

# if DISTRHO_PLUGIN_HAS_UI
    void setStateFromUI(const char* const key, const char* const newValue) override
# else
    void setStateFromUI(const char* const key, const char* const newValue)
# endif
    {
        fPlugin.setState(key, newValue);

        // check if we want to save this key
        if (! fPlugin.wantStateKey(key))
            return;

        // check if key already exists
        for (StringMap::iterator it=fStateMap.begin(), ite=fStateMap.end(); it != ite; ++it)
        {
            const String& dkey(it->first);

            if (dkey == key)
            {
                it->second = newValue;
                return;
            }
        }

        d_stderr("Failed to find plugin state with key \"%s\"", key);
    }
#endif
};

// -----------------------------------------------------------------------

struct VstObject {
    audioMasterCallback audioMaster;
    PluginVst* plugin;
};

#define validObject  effect != nullptr && effect->object != nullptr
#define validPlugin  effect != nullptr && effect->object != nullptr && ((VstObject*)effect->object)->plugin != nullptr
#define vstObjectPtr (VstObject*)effect->object
#define pluginPtr    (vstObjectPtr)->plugin

static intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    // first internal init
    const bool doInternalInit = (opcode == -1729 && index == 0xdead && value == 0xf00d);

    if (doInternalInit)
    {
        // set valid but dummy values
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
    }

    // Create dummy plugin to get data from
    static PluginExporter plugin(nullptr, nullptr);

    if (doInternalInit)
    {
        // unset
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        *(PluginExporter**)ptr = &plugin;
        return 0;
    }

    // handle base opcodes
    switch (opcode)
    {
    case effOpen:
        if (VstObject* const obj = vstObjectPtr)
        {
            // this must always be valid
            DISTRHO_SAFE_ASSERT_RETURN(obj->audioMaster != nullptr, 0);

            // some hosts call effOpen twice
            DISTRHO_SAFE_ASSERT_RETURN(obj->plugin == nullptr, 1);

            audioMasterCallback audioMaster = (audioMasterCallback)obj->audioMaster;

            d_lastBufferSize = audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);
            d_lastSampleRate = audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);

            // some hosts are not ready at this point or return 0 buffersize/samplerate
            if (d_lastBufferSize == 0)
                d_lastBufferSize = 2048;
            if (d_lastSampleRate <= 0.0)
                d_lastSampleRate = 44100.0;

            obj->plugin = new PluginVst(audioMaster, effect);
            return 1;
        }
        return 0;

    case effClose:
        if (VstObject* const obj = vstObjectPtr)
        {
            if (obj->plugin != nullptr)
            {
                delete obj->plugin;
                obj->plugin = nullptr;
            }

#if 0
            /* This code invalidates the object created in VSTPluginMain
             * Probably not safe against all hosts */
            obj->audioMaster = nullptr;
            effect->object = nullptr;
            delete obj;
#endif

            return 1;
        }
        //delete effect;
        return 0;

    case effGetParamLabel:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            DISTRHO_NAMESPACE::strncpy((char*)ptr, plugin.getParameterUnit(index), 8);
            return 1;
        }
        return 0;

    case effGetParamName:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            const String& shortName(plugin.getParameterShortName(index));
            if (shortName.isNotEmpty())
                DISTRHO_NAMESPACE::strncpy((char*)ptr, shortName, 16);
            else
                DISTRHO_NAMESPACE::strncpy((char*)ptr, plugin.getParameterName(index), 16);
            return 1;
        }
        return 0;

    case effGetParameterProperties:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            if (VstParameterProperties* const properties = (VstParameterProperties*)ptr)
            {
                memset(properties, 0, sizeof(VstParameterProperties));

                const uint32_t hints = plugin.getParameterHints(index);

                if (hints & kParameterIsOutput)
                    return 1;

                if (hints & kParameterIsBoolean)
                {
                    properties->flags |= kVstParameterIsSwitch;
                }

                if (hints & kParameterIsInteger)
                {
                    properties->flags |= kVstParameterUsesIntegerMinMax;
                    const ParameterRanges& ranges(plugin.getParameterRanges(index));

                    properties->minInteger = static_cast<int32_t>(ranges.min);
                    properties->maxInteger = static_cast<int32_t>(ranges.max);
                }

                if (hints & kParameterIsLogarithmic)
                {
                    properties->flags |= kVstParameterCanRamp;
                }

                return 1;
            }
        }
        return 0;

    case effGetPlugCategory:
#if DISTRHO_PLUGIN_IS_SYNTH
        return kPlugCategSynth;
#else
        return kPlugCategEffect;
#endif

    case effGetEffectName:
        if (char* const cptr = (char*)ptr)
        {
            DISTRHO_NAMESPACE::strncpy(cptr, plugin.getName(), 32);
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (char* const cptr = (char*)ptr)
        {
            DISTRHO_NAMESPACE::strncpy(cptr, plugin.getMaker(), 32);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (char* const cptr = (char*)ptr)
        {
            DISTRHO_NAMESPACE::strncpy(cptr, plugin.getLabel(), 32);
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return plugin.getVersion();

    case effGetVstVersion:
        return kVstVersion;
    };

    // handle advanced opcodes
    if (validPlugin)
        return pluginPtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    if (validPlugin)
        return pluginPtr->vst_getParameter(index);
    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    if (validPlugin)
        pluginPtr->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validPlugin)
        pluginPtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validPlugin)
        pluginPtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

#undef pluginPtr
#undef validObject
#undef validPlugin
#undef vstObjectPtr

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
#if DISTRHO_OS_WINDOWS || DISTRHO_OS_MAC
const AEffect* VSTPluginMain(audioMasterCallback audioMaster);
#else
const AEffect* VSTPluginMain(audioMasterCallback audioMaster) asm ("main");
#endif

DISTRHO_PLUGIN_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    USE_NAMESPACE_DISTRHO

    // old version
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    // first internal init
    PluginExporter* plugin = nullptr;
    vst_dispatcherCallback(nullptr, -1729, 0xdead, 0xf00d, &plugin, 0.0f);
    DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr, nullptr);

    AEffect* const effect(new AEffect);
    std::memset(effect, 0, sizeof(AEffect));

    // vst fields
    effect->magic    = kEffectMagic;
    effect->uniqueID = plugin->getUniqueId();
    effect->version  = plugin->getVersion();

    // VST doesn't support parameter outputs. we can fake them, but it is a hack. Disabled by default.
#ifdef DPF_VST_SHOW_PARAMETER_OUTPUTS
    const int numParams = plugin->getParameterCount();
#else
    int numParams = 0;
    bool outputsReached = false;

    for (uint32_t i=0, count=plugin->getParameterCount(); i < count; ++i)
    {
        if (plugin->isParameterInput(i))
        {
            // parameter outputs must be all at the end
            DISTRHO_SAFE_ASSERT_BREAK(! outputsReached);
            ++numParams;
            continue;
        }
        outputsReached = true;
    }
#endif

    // plugin fields
    effect->numParams   = numParams;
    effect->numPrograms = 1;
    effect->numInputs   = DISTRHO_PLUGIN_NUM_INPUTS;
    effect->numOutputs  = DISTRHO_PLUGIN_NUM_OUTPUTS;

    // plugin flags
    effect->flags |= effFlagsCanReplacing;
#if DISTRHO_PLUGIN_IS_SYNTH
    effect->flags |= effFlagsIsSynth;
#endif
#if DISTRHO_PLUGIN_HAS_UI
    effect->flags |= effFlagsHasEditor;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    effect->flags |= effFlagsProgramChunks;
#endif

    // static calls
    effect->dispatcher   = vst_dispatcherCallback;
    effect->process      = vst_processCallback;
    effect->getParameter = vst_getParameterCallback;
    effect->setParameter = vst_setParameterCallback;
    effect->processReplacing = vst_processReplacingCallback;

    // pointers
    VstObject* const obj(new VstObject());
    obj->audioMaster = audioMaster;
    obj->plugin      = nullptr;

    // done
    effect->object = obj;

    return effect;
}

// -----------------------------------------------------------------------
