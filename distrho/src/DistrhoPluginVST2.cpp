/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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
#include "DistrhoPluginVST.hpp"
#include "../DistrhoPluginUtils.hpp"
#include "../extra/ScopedSafeLocale.hpp"
#include "../extra/ScopedPointer.hpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
# include "../extra/RingBuffer.hpp"
#endif

#include <clocale>
#include <map>
#include <string>
#include <vector>

#ifndef __cdecl
# define __cdecl
#endif

#include "xaymar-vst2/vst.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

extern "C" {

// define the midi stuff ourselves
typedef struct _VstMidiEvent {
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t _ignore1[3];
    char midiData[4];
    char _ignore2[4];
} VstMidiEvent;

typedef union _VstEvent {
    int32_t type;
    VstMidiEvent midi; // type 1
} VstEvent;

typedef struct _HostVstEvents {
    int32_t numEvents;
    void* reserved;
    const VstEvent* events[];
} HostVstEvents;

typedef struct _PluginVstEvents {
    int32_t numEvents;
    void* reserved;
    VstEvent* events[1];
} PluginVstEvents;

// info from online documentation of VST provided by Steinberg
typedef struct _VstTimeInfo {
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    int32_t timeSigNumerator;
    int32_t timeSigDenominator;
    int32_t smpteOffset;
    int32_t smpteFrameRate;
    int32_t samplesToNextClock;
    int32_t flags;
} VstTimeInfo;

} // extern "C"

// --------------------------------------------------------------------------------------------------------------------

typedef std::map<const String, String> StringMap;

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------

struct ParameterAndNotesHelper
{
    float* parameterValues;
  #if DISTRHO_PLUGIN_HAS_UI
    bool* parameterChecks;
   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    SmallStackBuffer notesRingBuffer;
   #endif
  #endif

    ParameterAndNotesHelper()
        : parameterValues(nullptr)
      #if DISTRHO_PLUGIN_HAS_UI
        , parameterChecks(nullptr)
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        , notesRingBuffer(StackBuffer_INIT)
       #endif
      #endif
    {
       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT && ! defined(DISTRHO_PROPER_CPP11_SUPPORT)
        std::memset(&notesRingBuffer, 0, sizeof(notesRingBuffer));
       #endif
    }

    virtual ~ParameterAndNotesHelper()
    {
        if (parameterValues != nullptr)
        {
            delete[] parameterValues;
            parameterValues = nullptr;
        }
       #if DISTRHO_PLUGIN_HAS_UI
        if (parameterChecks != nullptr)
        {
            delete[] parameterChecks;
            parameterChecks = nullptr;
        }
       #endif
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    virtual void setStateFromUI(const char* key, const char* value) = 0;
   #endif
};

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static const sendNoteFunc sendNoteCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif

class UIVst
{
public:
    UIVst(const vst_host_callback audioMaster,
          vst_effect* const effect,
          ParameterAndNotesHelper* const uiHelper,
          PluginExporter* const plugin,
          const intptr_t winId, const float scaleFactor)
        : fAudioMaster(audioMaster),
          fEffect(effect),
          fUiHelper(uiHelper),
          fPlugin(plugin),
          fUI(this, winId, plugin->getSampleRate(),
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              d_nextBundlePath,
              plugin->getInstancePointer(),
              scaleFactor),
          fKeyboardModifiers(0)
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        , fNotesRingBuffer()
       #endif
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fNotesRingBuffer.setRingBuffer(&uiHelper->notesRingBuffer, false);
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------

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

        fUI.plugin_idle();
    }

    int16_t getWidth() const
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const
    {
        return fUI.getHeight();
    }

    double getScaleFactor() const
    {
        return fUI.getScaleFactor();
    }

    void setSampleRate(const double newSampleRate)
    {
        fUI.setSampleRate(newSampleRate, true);
    }

    void notifyScaleFactorChanged(const double scaleFactor)
    {
        fUI.notifyScaleFactorChanged(scaleFactor);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // functions called from the plugin side, may block

   #if DISTRHO_PLUGIN_WANT_STATE
    void setStateFromPlugin(const char* const key, const char* const value)
    {
        fUI.stateChanged(key, value);
    }
   #endif

    int handlePluginKeyEvent(const bool down, const int32_t index, const intptr_t value)
    {
        d_stdout("handlePluginKeyEvent %i %i %li\n", down, index, (long int)value);

        using namespace DGL_NAMESPACE;

        bool special;
        const uint key = translateVstKeyCode(special, index, static_cast<int32_t>(value));

        switch (key)
        {
        case kKeyShiftL:
        case kKeyShiftR:
            if (down)
                fKeyboardModifiers |= kModifierShift;
            else
                fKeyboardModifiers &= ~kModifierShift;
            break;
        case kKeyControlL:
        case kKeyControlR:
            if (down)
                fKeyboardModifiers |= kModifierControl;
            else
                fKeyboardModifiers &= ~kModifierControl;
            break;
        case kKeyAltL:
        case kKeyAltR:
            if (down)
                fKeyboardModifiers |= kModifierAlt;
            else
                fKeyboardModifiers &= ~kModifierAlt;
            break;
        }

        return fUI.handlePluginKeyboardVST(down, special, key,
                                           value >= 0 ? static_cast<uint>(value) : 0,
                                           fKeyboardModifiers) ? 1 : 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    inline intptr_t hostCallback(const VST_HOST_OPCODE opcode,
                                 const int32_t index = 0,
                                 const intptr_t value = 0,
                                 void* const ptr = nullptr,
                                 const float opt = 0.0f) const
    {
        return fAudioMaster(fEffect, opcode, index, value, ptr, opt);
    }

    void editParameter(const uint32_t index, const bool started) const
    {
        hostCallback(started ? VST_HOST_OPCODE_2B : VST_HOST_OPCODE_2C, index);
    }

    void setParameterValue(const uint32_t index, const float realValue)
    {
        const ParameterRanges& ranges(fPlugin->getParameterRanges(index));
        const float perValue = ranges.getNormalizedValue(realValue);

        fPlugin->setParameterValue(index, realValue);
        hostCallback(VST_HOST_OPCODE_00, index, 0, nullptr, perValue);
    }

    void setSize(uint width, uint height)
    {
       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        width /= scaleFactor;
        height /= scaleFactor;
       #endif
        hostCallback(VST_HOST_OPCODE_0F, width, height);
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        uint8_t midiData[3];
        midiData[0] = (velocity != 0 ? 0x90 : 0x80) | channel;
        midiData[1] = note;
        midiData[2] = velocity;
        fNotesRingBuffer.writeCustomData(midiData, 3);
        fNotesRingBuffer.commitWrite();
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        fUiHelper->setStateFromUI(key, value);
    }
   #endif

private:
    // Vst stuff
    const vst_host_callback fAudioMaster;
    vst_effect* const fEffect;
    ParameterAndNotesHelper* const fUiHelper;
    PluginExporter* const fPlugin;

    // Plugin UI
    UIExporter fUI;
    uint16_t fKeyboardModifiers;
   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    RingBufferControl<SmallStackBuffer> fNotesRingBuffer;
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks

    static void editParameterCallback(void* const ptr, const uint32_t index, const bool started)
    {
        static_cast<UIVst*>(ptr)->editParameter(index, started);
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<UIVst*>(ptr)->setParameterValue(rindex, value);
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<UIVst*>(ptr)->setSize(width, height);
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<UIVst*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<UIVst*>(ptr)->setState(key, value);
    }
   #endif
};
#endif // DISTRHO_PLUGIN_HAS_UI

// --------------------------------------------------------------------------------------------------------------------

class PluginVst : public ParameterAndNotesHelper
{
public:
    PluginVst(const vst_host_callback audioMaster, vst_effect* const effect)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, nullptr),
          fAudioMaster(audioMaster),
          fEffect(effect)
    {
        std::memset(fProgramName, 0, sizeof(fProgramName));
        std::strcpy(fProgramName, "Default");

        const uint32_t parameterCount = fPlugin.getParameterCount();

        if (parameterCount != 0)
        {
            parameterValues = new float[parameterCount];

            for (uint32_t i=0; i < parameterCount; ++i)
                parameterValues[i] = NAN;
        }

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fMidiEventCount = 0;
       #endif

      #if DISTRHO_PLUGIN_HAS_UI
        fVstUI           = nullptr;
        fVstRect.top     = 0;
        fVstRect.left    = 0;
        fVstRect.bottom  = 0;
        fVstRect.right   = 0;
        fLastScaleFactor = 0.0f;

        if (parameterCount != 0)
        {
            parameterChecks = new bool[parameterCount];
            memset(parameterChecks, 0, sizeof(bool)*parameterCount);
        }

      #ifdef DISTRHO_OS_MAC
       #ifdef __LP64__
        fUsingNsView = true;
       #else
        #ifndef DISTRHO_NO_WARNINGS
         #warning 32bit VST UIs on macOS only work if the host supports "hasCockosViewAsConfig"
        #endif
        fUsingNsView = false;
       #endif
      #endif // DISTRHO_OS_MAC

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fNotesRingBuffer.setRingBuffer(&notesRingBuffer, true);
       #endif
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
        case VST_EFFECT_OPCODE_03: // get program
            return 0;

        case VST_EFFECT_OPCODE_04: // set program name
            if (char* const programName = (char*)ptr)
            {
                d_strncpy(fProgramName, programName, 32);
                return 1;
            }
            break;

        case VST_EFFECT_OPCODE_05: // get program name
            if (char* const programName = (char*)ptr)
            {
                d_strncpy(programName, fProgramName, 24);
                return 1;
            }
            break;

        case VST_EFFECT_OPCODE_1D: // get program name indexed
            if (char* const programName = (char*)ptr)
            {
                d_strncpy(programName, fProgramName, 24);
                return 1;
            }
            break;

        case VST_EFFECT_OPCODE_PARAM_GETVALUE:
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

                    strncpy((char*)ptr, enumValues.values[i].label.buffer(), 24);
                    return 1;
                }

                if (hints & kParameterIsInteger)
                    snprintf_i32((char*)ptr, (int32_t)value, 24);
                else
                    snprintf_f32((char*)ptr, value, 24);

                return 1;
            }
            break;

        case VST_EFFECT_OPCODE_SET_SAMPLE_RATE:
            fPlugin.setSampleRate(opt, true);

           #if DISTRHO_PLUGIN_HAS_UI
            if (fVstUI != nullptr)
                fVstUI->setSampleRate(opt);
           #endif
            break;

        case VST_EFFECT_OPCODE_SET_BLOCK_SIZE:
            fPlugin.setBufferSize(value, true);
            break;

        case VST_EFFECT_OPCODE_SUSPEND:
            if (value != 0)
            {
               #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                fMidiEventCount = 0;

                // tell host we want MIDI events
                hostCallback(VST_HOST_OPCODE_06);
               #endif

                // deactivate for possible changes
                fPlugin.deactivateIfNeeded();

                // check if something changed
                const uint32_t bufferSize = static_cast<uint32_t>(hostCallback(VST_HOST_OPCODE_11));
                const double   sampleRate = static_cast<double>(hostCallback(VST_HOST_OPCODE_10));

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
        case VST_EFFECT_OPCODE_WINDOW_GETRECT:
            if (fVstUI != nullptr)
            {
                fVstRect.right  = fVstUI->getWidth();
                fVstRect.bottom = fVstUI->getHeight();
               #ifdef DISTRHO_OS_MAC
                const double scaleFactor = fVstUI->getScaleFactor();
                fVstRect.right /= scaleFactor;
                fVstRect.bottom /= scaleFactor;
               #endif
            }
            else
            {
                double scaleFactor = fLastScaleFactor;
               #if defined(DISTRHO_UI_DEFAULT_WIDTH) && defined(DISTRHO_UI_DEFAULT_HEIGHT)
                if (d_isZero(scaleFactor))
                    scaleFactor = 1.0;
                fVstRect.right = DISTRHO_UI_DEFAULT_WIDTH * scaleFactor;
                fVstRect.bottom = DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor;
               #else
                UIExporter tmpUI(nullptr, 0, fPlugin.getSampleRate(),
                                 nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, d_nextBundlePath,
                                 fPlugin.getInstancePointer(), scaleFactor);
                fVstRect.right = tmpUI.getWidth();
                fVstRect.bottom = tmpUI.getHeight();
                scaleFactor = tmpUI.getScaleFactor();
                tmpUI.quit();
               #endif
               #ifdef DISTRHO_OS_MAC
                fVstRect.right /= scaleFactor;
                fVstRect.bottom /= scaleFactor;
               #endif
            }
            *(vst_rect**)ptr = &fVstRect;
            return 1;

        case VST_EFFECT_OPCODE_WINDOW_CREATE:
            delete fVstUI; // for hosts which don't pair create/destroy calls (Minihost Modular)
            fVstUI = nullptr;

           #ifdef DISTRHO_OS_MAC
            if (! fUsingNsView)
            {
                d_stderr("Host doesn't support hasCockosViewAsConfig, cannot use UI");
                return 0;
            }
           #endif
            fVstUI = new UIVst(fAudioMaster, fEffect, this, &fPlugin, (intptr_t)ptr, fLastScaleFactor);

           #if DISTRHO_PLUGIN_WANT_FULL_STATE
            // Update current state from plugin side
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key = cit->first;
                fStateMap[key] = fPlugin.getStateValue(key);
            }
           #endif

           #if DISTRHO_PLUGIN_WANT_STATE
            // Set state
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key(cit->first);
                const String& value(cit->second);

                // TODO skip DSP only states

                fVstUI->setStateFromPlugin(key, value);
            }
           #endif

            for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
                setParameterValueFromPlugin(i, fPlugin.getParameterValue(i));

            fVstUI->idle();
            return 1;

        case VST_EFFECT_OPCODE_WINDOW_DESTROY:
            if (fVstUI != nullptr)
            {
                delete fVstUI;
                fVstUI = nullptr;
                return 1;
            }
            break;

        case VST_EFFECT_OPCODE_13: // window idle
            if (fVstUI != nullptr)
                fVstUI->idle();
            break;

        case VST_EFFECT_OPCODE_3B: // key down
            if (fVstUI != nullptr)
                return fVstUI->handlePluginKeyEvent(true, index, value);
            break;

        case VST_EFFECT_OPCODE_3C: // key up
            if (fVstUI != nullptr)
                return fVstUI->handlePluginKeyEvent(false, index, value);
            break;
      #endif // DISTRHO_PLUGIN_HAS_UI

       #if DISTRHO_PLUGIN_WANT_STATE
        case VST_EFFECT_OPCODE_17: // get chunk
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
               #if DISTRHO_PLUGIN_WANT_FULL_STATE
                // Update current state
                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key = cit->first;
                    fStateMap[key] = fPlugin.getStateValue(key);
                }
               #endif

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

                const std::size_t chunkSize = chunkStr.length()+1;

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

        case VST_EFFECT_OPCODE_18: // set chunk
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

               #if DISTRHO_PLUGIN_HAS_UI
                if (fVstUI != nullptr)
                {
                    // TODO skip DSP only states
                    fVstUI->setStateFromPlugin(key, value);
                }
               #endif

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

                        if (fPlugin.getParameterHints(i) & kParameterIsInteger)
                        {
                            fvalue = std::atoi(value);
                        }
                        else
                        {
                            const ScopedSafeLocale ssl;
                            fvalue = std::atof(value);
                        }

                        fPlugin.setParameterValue(i, fvalue);
                       #if DISTRHO_PLUGIN_HAS_UI
                        if (fVstUI != nullptr)
                            setParameterValueFromPlugin(i, fvalue);
                       #endif
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
        case VST_EFFECT_OPCODE_19: // process events
            if (! fPlugin.isActive())
            {
                // host has not activated the plugin yet, nasty!
                vst_dispatcher(VST_EFFECT_OPCODE_SUSPEND, 0, 1, nullptr, 0.0f);
            }

            if (const HostVstEvents* const events = (const HostVstEvents*)ptr)
            {
                if (events->numEvents == 0)
                    break;

                for (int i=0, count=events->numEvents; i < count; ++i)
                {
                    const VstEvent* const vstEvent = events->events[i];

                    if (vstEvent == nullptr)
                        break;
                    if (vstEvent->type != 1)
                        continue;
                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;

                    const VstMidiEvent& vstMidiEvent(events->events[i]->midi);

                    MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
                    midiEvent.frame  = vstMidiEvent.deltaFrames;
                    midiEvent.size   = 3;
                    std::memcpy(midiEvent.data, vstMidiEvent.midiData, sizeof(uint8_t)*3);
                }
            }
            break;
       #endif

        case VST_EFFECT_OPCODE_PARAM_ISAUTOMATABLE:
            if (index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                const uint32_t hints(fPlugin.getParameterHints(index));

                // must be automatable, and not output
                if ((hints & kParameterIsAutomatable) != 0 && (hints & kParameterIsOutput) == 0)
                    return 1;
            }
            break;

        case VST_EFFECT_OPCODE_SUPPORTS:
            if (const char* const canDo = (const char*)ptr)
            {
               #if defined(DISTRHO_OS_MAC) && DISTRHO_PLUGIN_HAS_UI
                if (std::strcmp(canDo, "hasCockosViewAsConfig") == 0)
                {
                    fUsingNsView = true;
                    return 0xbeef0000;
                }
               #endif
               #ifndef DISTRHO_OS_MAC
                if (std::strcmp(canDo, "supportsViewDpiScaling") == 0)
                    return 1;
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
                if (std::strcmp(canDo, "offline") == 0)
                    return -1;
            }
            break;

        case VST_EFFECT_OPCODE_CUSTOM:
           #if DISTRHO_PLUGIN_HAS_UI && !defined(DISTRHO_OS_MAC)
            if (index == d_cconst('P', 'r', 'e', 'S') && value == d_cconst('A', 'e', 'C', 's'))
            {
                if (d_isEqual(fLastScaleFactor, opt))
                    break;

                fLastScaleFactor = opt;

                if (fVstUI != nullptr)
                    fVstUI->notifyScaleFactorChanged(opt);
            }
           #endif
            break;

        //case effStartProcess:
        //case effStopProcess:
        // unused
        //    break;
        }

        return 0;
    }

    float vst_getParameter(const uint32_t index)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        return ranges.getNormalizedValue(fPlugin.getParameterValue(index));
    }

    void vst_setParameter(const uint32_t index, const float value)
    {
        const uint32_t hints = fPlugin.getParameterHints(index);
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        // TODO figure out how to detect kVstParameterUsesIntegerMinMax host support, and skip normalization
        float realValue = ranges.getUnnormalizedValue(value);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            realValue = realValue > midRange ? ranges.max : ranges.min;
        }

        if (hints & kParameterIsInteger)
        {
            realValue = std::round(realValue);
        }

        fPlugin.setParameterValue(index, realValue);

       #if DISTRHO_PLUGIN_HAS_UI
        if (fVstUI != nullptr)
            setParameterValueFromPlugin(index, realValue);
       #endif
    }

    void vst_processReplacing(const float** const inputs, float** const outputs, const int32_t sampleFrames)
    {
        if (! fPlugin.isActive())
        {
            // host has not activated the plugin yet, nasty!
            vst_dispatcher(VST_EFFECT_OPCODE_SUSPEND, 0, 1, nullptr, 0.0f);
        }

        if (sampleFrames <= 0)
        {
            updateParameterOutputsAndTriggers();
            return;
        }

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        static constexpr const int kWantVstTimeFlags = 0x2602;

        if (const VstTimeInfo* const vstTimeInfo = (const VstTimeInfo*)hostCallback(VST_HOST_OPCODE_07, 0, kWantVstTimeFlags))
        {
            fTimePosition.frame   = vstTimeInfo->samplePos;
            fTimePosition.playing = vstTimeInfo->flags & 0x2;

            // ticksPerBeat is not possible with VST2
            fTimePosition.bbt.ticksPerBeat = 1920.0;

            if (vstTimeInfo->flags & 0x400)
                fTimePosition.bbt.beatsPerMinute = vstTimeInfo->tempo;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            if ((vstTimeInfo->flags & 0x2200) == 0x2200)
            {
                const double ppqPos    = std::abs(vstTimeInfo->ppqPos);
                const int    ppqPerBar = vstTimeInfo->timeSigNumerator * 4 / vstTimeInfo->timeSigDenominator;
                const double barBeats  = (std::fmod(ppqPos, ppqPerBar) / ppqPerBar) * vstTimeInfo->timeSigNumerator;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimePosition.bbt.valid       = true;
                fTimePosition.bbt.bar         = static_cast<int32_t>(ppqPos) / ppqPerBar + 1;
                fTimePosition.bbt.beat        = static_cast<int32_t>(barBeats - rest + 0.5) + 1;
                fTimePosition.bbt.tick        = rest * fTimePosition.bbt.ticksPerBeat;
                fTimePosition.bbt.beatsPerBar = vstTimeInfo->timeSigNumerator;
                fTimePosition.bbt.beatType    = vstTimeInfo->timeSigDenominator;

                if (vstTimeInfo->ppqPos < 0.0)
                {
                    --fTimePosition.bbt.bar;
                    fTimePosition.bbt.beat = vstTimeInfo->timeSigNumerator - fTimePosition.bbt.beat + 1;
                    fTimePosition.bbt.tick = fTimePosition.bbt.ticksPerBeat - fTimePosition.bbt.tick - 1;
                }
            }
            else
            {
                fTimePosition.bbt.valid       = false;
                fTimePosition.bbt.bar         = 1;
                fTimePosition.bbt.beat        = 1;
                fTimePosition.bbt.tick        = 0.0;
                fTimePosition.bbt.beatsPerBar = 4.0f;
                fTimePosition.bbt.beatType    = 4.0f;
            }

            fTimePosition.bbt.barStartTick = fTimePosition.bbt.ticksPerBeat*
                                             fTimePosition.bbt.beatsPerBar*
                                             (fTimePosition.bbt.bar-1);

            fPlugin.setTimePosition(fTimePosition);
        }
       #endif

      #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
       #if DISTRHO_PLUGIN_HAS_UI
        if (fMidiEventCount != kMaxMidiEvents && fNotesRingBuffer.isDataAvailableForReading())
        {
            uint8_t midiData[3];
            const uint32_t frame = fMidiEventCount != 0 ? fMidiEvents[fMidiEventCount - 1].frame : 0;

            while (fNotesRingBuffer.isDataAvailableForReading())
            {
                if (! fNotesRingBuffer.readCustomData(midiData, 3))
                    break;

                MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
                midiEvent.frame = frame;
                midiEvent.size  = 3;
                std::memcpy(midiEvent.data, midiData, 3);

                if (fMidiEventCount == kMaxMidiEvents)
                    break;
            }
        }
       #endif

        fPlugin.run(inputs, outputs, sampleFrames, fMidiEvents, fMidiEventCount);
        fMidiEventCount = 0;
      #else
        fPlugin.run(inputs, outputs, sampleFrames);
      #endif

        updateParameterOutputsAndTriggers();
    }

    // ----------------------------------------------------------------------------------------------------------------

    friend class UIVst;

private:
    // Plugin
    PluginExporter fPlugin;

    // VST stuff
    const vst_host_callback fAudioMaster;
    vst_effect* const fEffect;

    // Temporary data
    char fProgramName[32];

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
   #endif

   #if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
   #endif

    // UI stuff
  #if DISTRHO_PLUGIN_HAS_UI
    UIVst*   fVstUI;
    vst_rect fVstRect;
    float    fLastScaleFactor;
   #ifdef DISTRHO_OS_MAC
    bool fUsingNsView;
   #endif
   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    RingBufferControl<SmallStackBuffer> fNotesRingBuffer;
   #endif
  #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    char*     fStateChunk;
    StringMap fStateMap;
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // host callback

    intptr_t hostCallback(const VST_HOST_OPCODE opcode,
                          const int32_t index = 0,
                          const intptr_t value = 0,
                          void* const ptr = nullptr,
                          const float opt = 0.0f)
    {
        return fAudioMaster(fEffect, opcode, index, value, ptr, opt);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // functions called from the plugin side, RT no block

    void updateParameterOutputsAndTriggers()
    {
        float curValue, defValue;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
            {
                // NOTE: no output parameter support in VST2, simulate it here
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
                // NOTE: no trigger parameter support in VST2, simulate it here
                defValue = fPlugin.getParameterDefault(i);
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, defValue))
                    continue;

               #if DISTRHO_PLUGIN_HAS_UI
                if (fVstUI != nullptr)
                    setParameterValueFromPlugin(i, defValue);
               #endif
                fPlugin.setParameterValue(i, defValue);
            }
            else
            {
                continue;
            }

            const ParameterRanges& ranges(fPlugin.getParameterRanges(i));
            hostCallback(VST_HOST_OPCODE_00, i, 0, nullptr, ranges.getNormalizedValue(curValue));
        }

       #if DISTRHO_PLUGIN_WANT_LATENCY
        fEffect->delay = fPlugin.getLatency();
       #endif
    }

   #if DISTRHO_PLUGIN_HAS_UI
    void setParameterValueFromPlugin(const uint32_t index, const float realValue)
    {
        parameterValues[index] = realValue;
        parameterChecks[index] = true;
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(const uint32_t index, const float value)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        hostCallback(VST_HOST_OPCODE_00, index, 0, nullptr, ranges.getNormalizedValue(value));
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return ((PluginVst*)ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        if (midiEvent.size > 4)
            return true;

        PluginVstEvents vstEvents = {};
        VstMidiEvent vstMidiEvent = {};

        vstEvents.numEvents = 1;
        vstEvents.events[0] = (VstEvent*)&vstMidiEvent;

        vstMidiEvent.type = 1;
        vstMidiEvent.byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
        vstMidiEvent.deltaFrames = midiEvent.frame;

        for (uint8_t i=0; i<midiEvent.size; ++i)
            vstMidiEvent.midiData[i] = midiEvent.data[i];

        return hostCallback(VST_HOST_OPCODE_08, 0, 0, &vstEvents) == 1;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginVst*>(ptr)->writeMidi(midiEvent);
    }
   #endif

  #if DISTRHO_PLUGIN_WANT_STATE
    // ----------------------------------------------------------------------------------------------------------------
    // functions called from the UI side, may block

   #if DISTRHO_PLUGIN_HAS_UI
    void setStateFromUI(const char* const key, const char* const value) override
   #else
    void setStateFromUI(const char* const key, const char* const value)
   #endif
    {
        fPlugin.setState(key, value);

        // check if we want to save this key
        if (fPlugin.wantStateKey(key))
        {
            const String dkey(key);
            fStateMap[dkey] = value;
        }
    }
  #endif
};

// --------------------------------------------------------------------------------------------------------------------

struct ExtendedAEffect : vst_effect {
    char _padding[63];
    char valid;
    vst_host_callback audioMaster;
    PluginVst* pluginPtr;
};

static ScopedPointer<PluginExporter> sPlugin;

static struct Cleanup {
    std::vector<ExtendedAEffect*> effects;

    ~Cleanup()
    {
        for (std::vector<ExtendedAEffect*>::iterator it = effects.begin(), end = effects.end(); it != end; ++it)
        {
            ExtendedAEffect* const exteffect = *it;
            delete exteffect->pluginPtr;
            delete exteffect;
        }

        sPlugin = nullptr;
    }
} sCleanup;

// --------------------------------------------------------------------------------------------------------------------

static inline
ExtendedAEffect* getExtendedEffect(vst_effect* const effect)
{
    if (effect == nullptr)
        return nullptr;

    ExtendedAEffect* const exteffect = static_cast<ExtendedAEffect*>(effect);
    DISTRHO_SAFE_ASSERT_RETURN(exteffect->valid == 101, nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(exteffect->audioMaster != nullptr, nullptr);

    return exteffect;
}

static inline
PluginVst* getEffectPlugin(vst_effect* const effect)
{
    if (effect == nullptr)
        return nullptr;

    ExtendedAEffect* const exteffect = static_cast<ExtendedAEffect*>(effect);
    DISTRHO_SAFE_ASSERT_RETURN(exteffect->valid == 101, nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(exteffect->audioMaster != nullptr, nullptr);

    return exteffect->pluginPtr;
}

// --------------------------------------------------------------------------------------------------------------------

static intptr_t VST_FUNCTION_INTERFACE vst_dispatcherCallback(vst_effect* const effect,
                                                              const VST_EFFECT_OPCODE opcode,
                                                              const int32_t index,
                                                              const intptr_t value,
                                                              void* const ptr,
                                                              const float opt)
{
    // handle base opcodes
    switch (opcode)
    {
    case VST_EFFECT_OPCODE_CREATE:
        if (ExtendedAEffect* const exteffect = getExtendedEffect(effect))
        {
            // some hosts call open/create twice
            if (exteffect->pluginPtr != nullptr)
                return 1;

            const vst_host_callback audioMaster = exteffect->audioMaster;

            d_nextBufferSize = audioMaster(effect, VST_HOST_OPCODE_11, 0, 0, nullptr, 0.0f);
            d_nextSampleRate = audioMaster(effect, VST_HOST_OPCODE_10, 0, 0, nullptr, 0.0f);
            d_nextCanRequestParameterValueChanges = true;

            // some hosts are not ready at this point or return 0 buffersize/samplerate
            if (d_nextBufferSize == 0)
                d_nextBufferSize = 2048;
            if (d_nextSampleRate <= 0.0)
                d_nextSampleRate = 44100.0;

            exteffect->pluginPtr = new PluginVst(audioMaster, effect);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_DESTROY:
        if (ExtendedAEffect* const exteffect = getExtendedEffect(effect))
        {
            // delete plugin object
            if (exteffect->pluginPtr != nullptr)
            {
                delete exteffect->pluginPtr;
                exteffect->pluginPtr = nullptr;
            }

            // delete effect too, if it comes from us
            const std::vector<ExtendedAEffect*>::iterator it = std::find(sCleanup.effects.begin(), sCleanup.effects.end(), exteffect);
            if (it != sCleanup.effects.end())
            {
                delete exteffect;
                sCleanup.effects.erase(it);
            }

            // delete global plugin instance too if this is the last loaded effect
            if (sCleanup.effects.empty())
                sPlugin = nullptr;
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_PARAM_GETLABEL:
        if (ptr != nullptr && index < static_cast<int32_t>(sPlugin->getParameterCount()))
        {
            d_strncpy((char*)ptr, sPlugin->getParameterUnit(index), 8);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_PARAM_GETNAME:
        if (ptr != nullptr && index < static_cast<int32_t>(sPlugin->getParameterCount()))
        {
            const String& shortName(sPlugin->getParameterShortName(index));
            if (shortName.isNotEmpty())
                d_strncpy((char*)ptr, shortName, 16);
            else
                d_strncpy((char*)ptr, sPlugin->getParameterName(index), 16);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_38: // FIXME VST_EFFECT_OPCODE_GET_PARAMETER_PROPERTIES is wrong by 1
        if (ptr != nullptr && index < static_cast<int32_t>(sPlugin->getParameterCount()))
        {
            if (vst_parameter_properties* const properties = (vst_parameter_properties*)ptr)
            {
                memset(properties, 0, sizeof(vst_parameter_properties));

                // full name
                d_strncpy(properties->name,
                          sPlugin->getParameterName(index),
                          sizeof(properties->name));

                // short name
                const String& shortName(sPlugin->getParameterShortName(index));

                if (shortName.isNotEmpty())
                    d_strncpy(properties->label,
                              sPlugin->getParameterShortName(index),
                              sizeof(properties->label));

                // parameter hints
                const uint32_t hints = sPlugin->getParameterHints(index);

                if (hints & kParameterIsOutput)
                    return 1;

                if (hints & kParameterIsBoolean)
                {
                    properties->flags |= VST_PARAMETER_FLAGS_SWITCH;
                }

                if (hints & kParameterIsInteger)
                {
                    const ParameterRanges& ranges(sPlugin->getParameterRanges(index));
                    properties->flags |= VST_PARAMETER_FLAGS_INTEGER_LIMITS;
                    properties->min_value_i32 = static_cast<int32_t>(ranges.min);
                    properties->max_value_i32 = static_cast<int32_t>(ranges.max);
                }

                if (hints & kParameterIsLogarithmic)
                {
                    properties->flags |= VST_PARAMETER_FLAGS_UNKNOWN6; // can ramp
                }

                // parameter group (category in vst)
                const uint32_t groupId = sPlugin->getParameterGroupId(index);

                if (groupId != kPortGroupNone)
                {
                    // we can't use groupId directly, so use the index array where this group is stored in
                    for (uint32_t i=0, count=sPlugin->getPortGroupCount(); i < count; ++i)
                    {
                        const PortGroupWithId& portGroup(sPlugin->getPortGroupByIndex(i));

                        if (portGroup.groupId == groupId)
                        {
                            properties->flags |= VST_PARAMETER_FLAGS_CATEGORY;
                            properties->category = i + 1;
                            d_strncpy(properties->category_label,
                                      portGroup.name.buffer(),
                                      sizeof(properties->category_label));
                            break;
                        }
                    }

                    if (properties->category != 0)
                    {
                        for (uint32_t i=0, count=sPlugin->getParameterCount(); i < count; ++i)
                            if (sPlugin->getParameterGroupId(i) == groupId)
                                ++properties->num_parameters_in_category;
                    }
                }

                return 1;
            }
        }
        return 0;

    case VST_EFFECT_OPCODE_EFFECT_CATEGORY:
       #if DISTRHO_PLUGIN_IS_SYNTH
        return VST_CATEGORY_02;
       #else
        return VST_CATEGORY_01;
       #endif

    case VST_EFFECT_OPCODE_EFFECT_NAME:
        if (char* const cptr = (char*)ptr)
        {
            d_strncpy(cptr, sPlugin->getName(), 32);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_VENDOR_NAME:
        if (char* const cptr = (char*)ptr)
        {
            d_strncpy(cptr, sPlugin->getMaker(), 32);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_PRODUCT_NAME:
        if (char* const cptr = (char*)ptr)
        {
            d_strncpy(cptr, sPlugin->getLabel(), 32);
            return 1;
        }
        return 0;

    case VST_EFFECT_OPCODE_VENDOR_VERSION:
        return sPlugin->getVersion();

    case VST_EFFECT_OPCODE_VST_VERSION:
        return VST_VERSION_2_4_0_0;

    default:
        break;
    }

    // handle advanced opcodes
    if (PluginVst* const pluginPtr = getEffectPlugin(effect))
        return pluginPtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float VST_FUNCTION_INTERFACE vst_getParameterCallback(vst_effect* const effect,
                                                             const uint32_t index)
{
    if (PluginVst* const pluginPtr = getEffectPlugin(effect))
        return pluginPtr->vst_getParameter(index);
    return 0.0f;
}

static void VST_FUNCTION_INTERFACE vst_setParameterCallback(vst_effect* const effect,
                                                            const uint32_t index,
                                                            const float value)
{
    if (PluginVst* const pluginPtr = getEffectPlugin(effect))
        pluginPtr->vst_setParameter(index, value);
}

static void VST_FUNCTION_INTERFACE vst_processCallback(vst_effect* const effect,
                                                       const float* const* const inputs,
                                                       float** const outputs,
                                                       const int32_t sampleFrames)
{
    if (PluginVst* const pluginPtr = getEffectPlugin(effect))
        pluginPtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

static void VST_FUNCTION_INTERFACE vst_processReplacingCallback(vst_effect* const effect,
                                                                const float* const* const inputs,
                                                                float** const outputs,
                                                                const int32_t sampleFrames)
{
    if (PluginVst* const pluginPtr = getEffectPlugin(effect))
        pluginPtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const vst_effect* VSTPluginMain(vst_host_callback);

DISTRHO_PLUGIN_EXPORT
const vst_effect* VSTPluginMain(const vst_host_callback audioMaster)
{
    USE_NAMESPACE_DISTRHO

    // old version
    if (audioMaster(nullptr, VST_HOST_OPCODE_01 /* version */, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    // find plugin bundle
    static String bundlePath;
    if (bundlePath.isEmpty())
    {
        String tmpPath(getBinaryFilename());
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
       #ifdef DISTRHO_OS_MAC
        if (tmpPath.endsWith("/MacOS"))
        {
            tmpPath.truncate(tmpPath.rfind('/'));
            if (tmpPath.endsWith("/Contents"))
            {
                tmpPath.truncate(tmpPath.rfind('/'));
                bundlePath = tmpPath;
                d_nextBundlePath = bundlePath.buffer();
            }
        }
       #else
        if (tmpPath.endsWith(".vst"))
        {
            bundlePath = tmpPath;
            d_nextBundlePath = bundlePath.buffer();
        }
       #endif
    }

    // first internal init
    if (sPlugin == nullptr)
    {
        // set valid but dummy values
        d_nextBufferSize = 512;
        d_nextSampleRate = 44100.0;
        d_nextPluginIsDummy = true;
        d_nextCanRequestParameterValueChanges = true;

        // Create dummy plugin to get data from
        sPlugin = new PluginExporter(nullptr, nullptr, nullptr, nullptr);

        // unset
        d_nextBufferSize = 0;
        d_nextSampleRate = 0.0;
        d_nextPluginIsDummy = false;
        d_nextCanRequestParameterValueChanges = false;
    }

    ExtendedAEffect* const effect = new ExtendedAEffect;
    std::memset(effect, 0, sizeof(ExtendedAEffect));

    // vst fields
   #ifdef WORDS_BIGENDIAN
    effect->magic_number = 0x50747356;
   #else
    effect->magic_number = 0x56737450;
   #endif
    effect->unique_id    = sPlugin->getUniqueId();
    effect->version      = sPlugin->getVersion();

    // VST doesn't support parameter outputs. we can fake them, but it is a hack. Disabled by default.
   #ifdef DPF_VST_SHOW_PARAMETER_OUTPUTS
    const int numParams = sPlugin->getParameterCount();
   #else
    int numParams = 0;
    bool outputsReached = false;

    for (uint32_t i=0, count=sPlugin->getParameterCount(); i < count; ++i)
    {
        if (sPlugin->isParameterInput(i))
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
    effect->num_params   = numParams;
    effect->num_programs = 1;
    effect->num_inputs   = DISTRHO_PLUGIN_NUM_INPUTS;
    effect->num_outputs  = DISTRHO_PLUGIN_NUM_OUTPUTS;

    // plugin flags
    effect->flags |= 1 << 4; // uses process_float
   #if DISTRHO_PLUGIN_IS_SYNTH
    effect->flags |= 1 << 8;
   #endif
   #if DISTRHO_PLUGIN_HAS_UI
    effect->flags |= 1 << 0;
   #endif
   #if DISTRHO_PLUGIN_WANT_STATE
    effect->flags |= 1 << 5;
   #endif

    // static calls
    effect->control       = vst_dispatcherCallback;
    effect->process       = vst_processCallback;
    effect->get_parameter = vst_getParameterCallback;
    effect->set_parameter = vst_setParameterCallback;
    effect->process_float = vst_processReplacingCallback;

    // special values
    effect->valid       = 101;
    effect->audioMaster = audioMaster;
    effect->pluginPtr   = nullptr;

    // done
    sCleanup.effects.push_back(effect);

    return effect;
}

#if !(defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WASM) || defined(DISTRHO_OS_WINDOWS) || DISTRHO_UI_WEB_VIEW)
DISTRHO_PLUGIN_EXPORT
const vst_effect* VSTPluginMainCompat(vst_host_callback) asm ("main");

DISTRHO_PLUGIN_EXPORT
const vst_effect* VSTPluginMainCompat(const vst_host_callback audioMaster)
{
    // protect main symbol against running as executable
    if (reinterpret_cast<uintptr_t>(audioMaster) < 0xff)
        return nullptr;

    return VSTPluginMain(audioMaster);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
