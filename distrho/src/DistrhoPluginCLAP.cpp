/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

/* TODO items:
 * CV: write a specification
 * INFO: define url, manual url, support url and string version
 * PARAMETERS: test parameter triggers
 * States: skip DSP/UI only states as appropriate
 * UI: expose external-only UIs
 */

#include "DistrhoPluginInternal.hpp"
#include "extra/ScopedPointer.hpp"

#ifndef DISTRHO_PLUGIN_CLAP_ID
# error DISTRHO_PLUGIN_CLAP_ID undefined!
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
# include "../extra/Mutex.hpp"
#endif

#if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT
# include "../extra/RingBuffer.hpp"
#endif

#include <map>
#include <vector>

#include "clap/entry.h"
#include "clap/plugin-factory.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/latency.h"
#include "clap/ext/gui.h"
#include "clap/ext/note-ports.h"
#include "clap/ext/params.h"
#include "clap/ext/state.h"
#include "clap/ext/thread-check.h"
#include "clap/ext/timer-support.h"

#if (defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WINDOWS)) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# define DPF_CLAP_USING_HOST_TIMER 0
#else
# define DPF_CLAP_USING_HOST_TIMER 1
#endif

#ifndef DPF_CLAP_TIMER_INTERVAL
# define DPF_CLAP_TIMER_INTERVAL 16 /* ~60 fps */
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

typedef std::map<const String, String> StringMap;

struct ClapEventQueue
{
  #if DISTRHO_PLUGIN_HAS_UI
    enum EventType {
        kEventGestureBegin,
        kEventGestureEnd,
        kEventParamSet
    };

    struct Event {
        EventType type;
        uint32_t index;
        float value;
    };

    struct Queue {
        RecursiveMutex lock;
        uint allocated;
        uint used;
        Event* events;

        Queue()
            : allocated(0),
              used(0),
              events(nullptr) {}

        ~Queue()
        {
            delete[] events;
        }

        void addEventFromUI(const Event& event)
        {
            const RecursiveMutexLocker crml(lock);

            if (events == nullptr)
            {
                events = static_cast<Event*>(std::malloc(sizeof(Event) * 8));
                allocated = 8;
            }
            else if (used + 1 > allocated)
            {
                allocated = used * 2;
                events = static_cast<Event*>(std::realloc(events, sizeof(Event) * allocated));
            }

            std::memcpy(&events[used++], &event, sizeof(Event));
        }
    } fEventQueue;

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    SmallStackBuffer fNotesBuffer;
   #endif
  #endif

   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t fCurrentProgram;
   #endif

  #if DISTRHO_PLUGIN_WANT_STATE
    StringMap fStateMap;
   #if DISTRHO_PLUGIN_HAS_UI
    virtual void setStateFromUI(const char* key, const char* value) = 0;
   #endif
  #endif

    struct CachedParameters {
        uint numParams;
        bool* changed;
        float* values;

        CachedParameters()
            : numParams(0),
              changed(nullptr),
              values(nullptr) {}

        ~CachedParameters()
        {
            delete[] changed;
            delete[] values;
        }

        void setup(const uint numParameters)
        {
            if (numParameters == 0)
                return;

            numParams = numParameters;
            changed = new bool[numParameters];
            values = new float[numParameters];

            std::memset(changed, 0, sizeof(bool)*numParameters);
            std::memset(values, 0, sizeof(float)*numParameters);
        }
    } fCachedParameters;

    ClapEventQueue()
       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        : fNotesBuffer(StackBuffer_INIT)
       #endif
    {
       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT && ! defined(DISTRHO_PROPER_CPP11_SUPPORT)
        std::memset(&fNotesBuffer, 0, sizeof(fNotesBuffer));
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        fCurrentProgram = 0;
       #endif
    }

    virtual ~ClapEventQueue() {}
};

// --------------------------------------------------------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

/**
 * CLAP UI class.
 */
class ClapUI : public DGL_NAMESPACE::IdleCallback
{
public:
    ClapUI(PluginExporter& plugin,
           ClapEventQueue* const eventQueue,
           const clap_host_t* const host,
           const clap_host_gui_t* const hostGui,
          #if DPF_CLAP_USING_HOST_TIMER
           const clap_host_timer_support_t* const hostTimer,
          #endif
           const bool isFloating)
        : fPlugin(plugin),
          fPluinEventQueue(eventQueue),
          fEventQueue(eventQueue->fEventQueue),
          fCachedParameters(eventQueue->fCachedParameters),
         #if DISTRHO_PLUGIN_WANT_PROGRAMS
          fCurrentProgram(eventQueue->fCurrentProgram),
         #endif
         #if DISTRHO_PLUGIN_WANT_STATE
          fStateMap(eventQueue->fStateMap),
         #endif
          fHost(host),
          fHostGui(hostGui),
         #if DPF_CLAP_USING_HOST_TIMER
          fTimerId(0),
          fHostTimer(hostTimer),
         #else
          fCallbackRegistered(false),
         #endif
          fIsFloating(isFloating),
          fScaleFactor(0.0),
          fParentWindow(0),
          fTransientWindow(0)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fNotesRingBuffer.setRingBuffer(&eventQueue->fNotesBuffer, false);
       #endif
    }

    ~ClapUI() override
    {
       #if DPF_CLAP_USING_HOST_TIMER
        if (fTimerId != 0)
            fHostTimer->unregister_timer(fHost, fTimerId);
       #else
        if (fCallbackRegistered && fUI != nullptr)
            fUI->removeIdleCallbackForNativeIdle(this);
       #endif
    }

   #ifndef DISTRHO_OS_MAC
    bool setScaleFactor(const double scaleFactor)
    {
        if (d_isEqual(fScaleFactor, scaleFactor))
            return true;

        fScaleFactor = scaleFactor;

        if (UIExporter* const ui = fUI.get())
            ui->notifyScaleFactorChanged(scaleFactor);

        return true;
    }
   #endif

    bool getSize(uint32_t* const width, uint32_t* const height) const
    {
        if (UIExporter* const ui = fUI.get())
        {
            *width = ui->getWidth();
            *height = ui->getHeight();
           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = ui->getScaleFactor();
            *width /= scaleFactor;
            *height /= scaleFactor;
           #endif
            return true;
        }

        double scaleFactor = fScaleFactor;
       #if defined(DISTRHO_UI_DEFAULT_WIDTH) && defined(DISTRHO_UI_DEFAULT_HEIGHT)
        *width = DISTRHO_UI_DEFAULT_WIDTH;
        *height = DISTRHO_UI_DEFAULT_HEIGHT;
        if (d_isZero(scaleFactor))
            scaleFactor = 1.0;
       #else
        UIExporter tmpUI(nullptr, 0, fPlugin.getSampleRate(),
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, d_nextBundlePath,
                         fPlugin.getInstancePointer(), scaleFactor);
        *width = tmpUI.getWidth();
        *height = tmpUI.getHeight();
        scaleFactor = tmpUI.getScaleFactor();
        tmpUI.quit();
       #endif

       #ifdef DISTRHO_OS_MAC
        *width /= scaleFactor;
        *height /= scaleFactor;
       #endif

        return true;
    }

    bool canResize() const noexcept
    {
       #if DISTRHO_UI_USER_RESIZABLE
        if (UIExporter* const ui = fUI.get())
            return ui->isResizable();
       #endif
        return false;
    }

    bool getResizeHints(clap_gui_resize_hints_t* const hints) const
    {
        if (canResize())
        {
            uint minimumWidth, minimumHeight;
            bool keepAspectRatio;
            fUI->getGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio);

           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = fUI->getScaleFactor();
            minimumWidth /= scaleFactor;
            minimumHeight /= scaleFactor;
           #endif

            hints->can_resize_horizontally = true;
            hints->can_resize_vertically = true;
            hints->preserve_aspect_ratio = keepAspectRatio;
            hints->aspect_ratio_width = minimumWidth;
            hints->aspect_ratio_height = minimumHeight;

            return true;
        }

        hints->can_resize_horizontally = false;
        hints->can_resize_vertically = false;
        hints->preserve_aspect_ratio = false;
        hints->aspect_ratio_width = 0;
        hints->aspect_ratio_height = 0;

        return false;
    }

    bool adjustSize(uint32_t* const width, uint32_t* const height) const
    {
        if (canResize())
        {
            uint minimumWidth, minimumHeight;
            bool keepAspectRatio;
            fUI->getGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio);

           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = fUI->getScaleFactor();
            minimumWidth /= scaleFactor;
            minimumHeight /= scaleFactor;
           #endif

            if (keepAspectRatio)
            {
                if (*width < 1)
                    *width = 1;
                if (*height < 1)
                    *height = 1;

                const double ratio = static_cast<double>(minimumWidth) / static_cast<double>(minimumHeight);
                const double reqRatio = static_cast<double>(*width) / static_cast<double>(*height);

                if (d_isNotEqual(ratio, reqRatio))
                {
                    // fix width
                    if (reqRatio > ratio)
                        *width = static_cast<int32_t>(*height * ratio + 0.5);
                    // fix height
                    else
                        *height = static_cast<int32_t>(static_cast<double>(*width) / ratio + 0.5);
                }
            }

            if (minimumWidth > *width)
                *width = minimumWidth;
            if (minimumHeight > *height)
                *height = minimumHeight;
            
            return true;
        }

        return false;
    }

    bool setSizeFromHost(uint32_t width, uint32_t height)
    {
        if (UIExporter* const ui = fUI.get())
        {
           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = ui->getScaleFactor();
            width *= scaleFactor;
            height *= scaleFactor;
           #endif
            ui->setWindowSizeFromHost(width, height);
            return true;
        }

        return false;
    }

    bool setParent(const clap_window_t* const window)
    {
        if (fIsFloating)
            return false;

        fParentWindow = window->uptr;

        if (fUI == nullptr)
        {
            createUI();
            fHostGui->resize_hints_changed(fHost);
        }

        return true;
    }

    bool setTransient(const clap_window_t* const window)
    {
        if (! fIsFloating)
            return false;

        fTransientWindow = window->uptr;

        if (UIExporter* const ui = fUI.get())
            ui->setWindowTransientWinId(window->uptr);

        return true;
    }

    void suggestTitle(const char* const title)
    {
        if (! fIsFloating)
            return;

        fWindowTitle = title;

        if (UIExporter* const ui = fUI.get())
            ui->setWindowTitle(title);
    }

    bool show()
    {
        if (fUI == nullptr)
        {
            createUI();
            fHostGui->resize_hints_changed(fHost);
        }

        if (fIsFloating)
            fUI->setWindowVisible(true);

       #if DPF_CLAP_USING_HOST_TIMER
        fHostTimer->register_timer(fHost, DPF_CLAP_TIMER_INTERVAL, &fTimerId);
       #else
        fCallbackRegistered = true;
        fUI->addIdleCallbackForNativeIdle(this, DPF_CLAP_TIMER_INTERVAL);
       #endif
        return true;
    }

    bool hide()
    {
        if (UIExporter* const ui = fUI.get())
        {
            ui->setWindowVisible(false);
           #if DPF_CLAP_USING_HOST_TIMER
            fHostTimer->unregister_timer(fHost, fTimerId);
            fTimerId = 0;
           #else
            ui->removeIdleCallbackForNativeIdle(this);
            fCallbackRegistered = false;
           #endif
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void idleCallback() override
    {
        if (UIExporter* const ui = fUI.get())
        {
           #if DPF_CLAP_USING_HOST_TIMER
            ui->plugin_idle();
           #else
            ui->idleFromNativeIdle();
           #endif

            for (uint i=0; i<fCachedParameters.numParams; ++i)
            {
                if (fCachedParameters.changed[i])
                {
                    fCachedParameters.changed[i] = false;
                    ui->parameterChanged(i, fCachedParameters.values[i]);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    void setParameterValueFromPlugin(const uint index, const float value)
    {
        if (UIExporter* const ui = fUI.get())
            ui->parameterChanged(index, value);
    }

   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setProgramFromPlugin(const uint index)
    {
        if (UIExporter* const ui = fUI.get())
            ui->programLoaded(index);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    void setStateFromPlugin(const char* const key, const char* const value)
    {
        if (UIExporter* const ui = fUI.get())
            ui->stateChanged(key, value);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin and UI
    PluginExporter& fPlugin;
    ClapEventQueue* const fPluinEventQueue;
    ClapEventQueue::Queue& fEventQueue;
    ClapEventQueue::CachedParameters& fCachedParameters;
   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t& fCurrentProgram;
   #endif
   #if DISTRHO_PLUGIN_WANT_STATE
    StringMap& fStateMap;
   #endif
    const clap_host_t* const fHost;
    const clap_host_gui_t* const fHostGui;
   #if DPF_CLAP_USING_HOST_TIMER
    clap_id fTimerId;
    const clap_host_timer_support_t* const fHostTimer;
   #else
    bool fCallbackRegistered;
   #endif
   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    RingBufferControl<SmallStackBuffer> fNotesRingBuffer;
   #endif
    ScopedPointer<UIExporter> fUI;

    const bool fIsFloating;

    // Temporary data
    double fScaleFactor;
    uintptr_t fParentWindow;
    uintptr_t fTransientWindow;
    String fWindowTitle;

    // ----------------------------------------------------------------------------------------------------------------

    void createUI()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI == nullptr,);

        fUI = new UIExporter(this,
                             fParentWindow,
                             fPlugin.getSampleRate(),
                             editParameterCallback,
                             setParameterCallback,
                             setStateCallback,
                             sendNoteCallback,
                             setSizeCallback,
                             fileRequestCallback,
                             d_nextBundlePath,
                             fPlugin.getInstancePointer(),
                             fScaleFactor);

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        fUI->programLoaded(fCurrentProgram);
       #endif

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
            const String& key   = cit->first;
            const String& value = cit->second;

            // TODO skip DSP only states

            fUI->stateChanged(key, value);
        }
       #endif

        for (uint32_t i=0; i<fCachedParameters.numParams; ++i)
        {
            const float value = fCachedParameters.values[i] = fPlugin.getParameterValue(i);
            fCachedParameters.changed[i] = false;
            fUI->parameterChanged(i, value);
        }

        if (fIsFloating)
        {
            if (fWindowTitle.isNotEmpty())
                fUI->setWindowTitle(fWindowTitle);

            if (fTransientWindow != 0)
                fUI->setWindowTransientWinId(fTransientWindow);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(const uint32_t rindex, const bool started) const
    {
        const ClapEventQueue::Event ev = {
            started ? ClapEventQueue::kEventGestureBegin : ClapEventQueue::kEventGestureEnd,
            rindex, 0.f
        };
        fEventQueue.addEventFromUI(ev);
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<ClapUI*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(const uint32_t rindex, const float value)
    {
        const ClapEventQueue::Event ev = {
            ClapEventQueue::kEventParamSet,
            rindex, value
        };
        fEventQueue.addEventFromUI(ev);
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<ClapUI*>(ptr)->setParameterValue(rindex, value);
    }

    void setSizeFromPlugin(const uint width, const uint height)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI->getScaleFactor();
        const uint hostWidth = width / scaleFactor;
        const uint hostHeight = height / scaleFactor;
       #else
        const uint hostWidth = width;
        const uint hostHeight = height;
       #endif

        if (fHostGui->request_resize(fHost, hostWidth, hostHeight))
            fUI->setWindowSizeFromHost(width, height);
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<ClapUI*>(ptr)->setSizeFromPlugin(width, height);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        fPluinEventQueue->setStateFromUI(key, value);
    }

    static void setStateCallback(void* const ptr, const char* key, const char* value)
    {
        static_cast<ClapUI*>(ptr)->setState(key, value);
    }
   #endif

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

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<ClapUI*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    bool fileRequest(const char*)
    {
        return true;
    }

    static bool fileRequestCallback(void* const ptr, const char* const key)
    {
        return static_cast<ClapUI*>(ptr)->fileRequest(key);
    }
};

// --------------------------------------------------------------------------------------------------------------------

#endif // DISTRHO_PLUGIN_HAS_UI

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const updateStateValueFunc updateStateValueCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------

/**
 * CLAP plugin class.
 */
class PluginCLAP : ClapEventQueue
{
public:
    PluginCLAP(const clap_host_t* const host)
        : fPlugin(this,
                  writeMidiCallback,
                  requestParameterValueChangeCallback,
                  updateStateValueCallback),
          fHost(host),
          fOutputEvents(nullptr),
         #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
          fUsingCV(false),
         #endif
         #if DISTRHO_PLUGIN_WANT_LATENCY
          fLatencyChanged(false),
          fLastKnownLatency(0),
         #endif
         #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
          fMidiEventCount(0),
         #endif
          fHostExtensions(host)
    {
        fCachedParameters.setup(fPlugin.getParameterCount());

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fNotesRingBuffer.setRingBuffer(&fNotesBuffer, true);
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=fPlugin.getStateCount(); i<count; ++i)
        {
            const String& dkey(fPlugin.getStateKey(i));
            fStateMap[dkey] = fPlugin.getStateDefaultValue(i);
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        fillInBusInfoDetails<true>();
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        fillInBusInfoDetails<false>();
       #endif
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        fillInBusInfoPairs();
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // core

    template <class T>
    const T* getHostExtension(const char* const extensionId) const
    {
        return static_cast<const T*>(fHost->get_extension(fHost, extensionId));
    }

    bool init()
    {
        if (!clap_version_is_compatible(fHost->clap_version))
            return false;

        return fHostExtensions.init();
    }

    void activate(const double sampleRate, const uint32_t maxFramesCount)
    {
        fPlugin.setSampleRate(sampleRate, true);
        fPlugin.setBufferSize(maxFramesCount, true);
        fPlugin.activate();
    }

    void deactivate()
    {
        fPlugin.deactivate();
       #if DISTRHO_PLUGIN_WANT_LATENCY
        checkForLatencyChanges(false, true);
        reportLatencyChangeIfNeeded();
       #endif
    }

    bool process(const clap_process_t* const process)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fMidiEventCount = 0;
       #endif

       #if DISTRHO_PLUGIN_HAS_UI
        if (const clap_output_events_t* const outputEvents = process->out_events)
        {
            const RecursiveMutexTryLocker crmtl(fEventQueue.lock);

            if (crmtl.wasLocked())
            {
                // reuse the same struct for gesture and parameters, they are compatible up to where it matters
                clap_event_param_value_t clapEvent = {
                    { 0, 0, 0, 0, CLAP_EVENT_IS_LIVE },
                    0, nullptr, 0, 0, 0, 0, 0.0
                };

                for (uint32_t i=0; i<fEventQueue.used; ++i)
                {
                    const Event& event(fEventQueue.events[i]);

                    switch (event.type)
                    {
                    case kEventGestureBegin:
                        clapEvent.header.size = sizeof(clap_event_param_gesture_t);
                        clapEvent.header.type = CLAP_EVENT_PARAM_GESTURE_BEGIN;
                        clapEvent.param_id = event.index;
                        break;
                    case kEventGestureEnd:
                        clapEvent.header.size = sizeof(clap_event_param_gesture_t);
                        clapEvent.header.type = CLAP_EVENT_PARAM_GESTURE_END;
                        clapEvent.param_id = event.index;
                        break;
                    case kEventParamSet:
                        clapEvent.header.size = sizeof(clap_event_param_value_t);
                        clapEvent.header.type = CLAP_EVENT_PARAM_VALUE;
                        clapEvent.param_id = event.index;
                        clapEvent.value = event.value;
                        fPlugin.setParameterValue(event.index, event.value);
                        break;
                    default:
                        continue;
                    }

                    outputEvents->try_push(outputEvents, &clapEvent.header);
                }

                fEventQueue.used = 0;
            }
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (const clap_event_transport_t* const transport = process->transport)
        {
            fTimePosition.playing = (transport->flags & CLAP_TRANSPORT_IS_PLAYING) != 0 &&
                                    (transport->flags & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL) == 0;

            fTimePosition.frame = process->steady_time >= 0 ? process->steady_time : 0;

            if (transport->flags & CLAP_TRANSPORT_HAS_TEMPO)
                fTimePosition.bbt.beatsPerMinute = transport->tempo;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            // ticksPerBeat is not possible with CLAP
            fTimePosition.bbt.ticksPerBeat = 1920.0;

            if ((transport->flags & (CLAP_TRANSPORT_HAS_BEATS_TIMELINE|CLAP_TRANSPORT_HAS_TIME_SIGNATURE)) == (CLAP_TRANSPORT_HAS_BEATS_TIMELINE|CLAP_TRANSPORT_HAS_TIME_SIGNATURE))
            {
                if (transport->song_pos_beats >= 0)
                {
                    const int64_t clapPos   = std::abs(transport->song_pos_beats);
                    const int64_t clapBeats = clapPos >> 31;
                    const double  clapRest  = static_cast<double>(clapPos & 0x7fffffff) / CLAP_BEATTIME_FACTOR;

                    fTimePosition.bbt.bar  = static_cast<int32_t>(clapBeats) / transport->tsig_num + 1;
                    fTimePosition.bbt.beat = static_cast<int32_t>(clapBeats % transport->tsig_num) + 1;
                    fTimePosition.bbt.tick = clapRest * fTimePosition.bbt.ticksPerBeat;
                }
                else
                {
                    fTimePosition.bbt.bar  = 1;
                    fTimePosition.bbt.beat = 1;
                    fTimePosition.bbt.tick = 0.0;
                }

                fTimePosition.bbt.valid       = true;
                fTimePosition.bbt.beatsPerBar = transport->tsig_num;
                fTimePosition.bbt.beatType    = transport->tsig_denom;
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
        }
        else
        {
            fTimePosition.playing = false;
            fTimePosition.frame = 0;
            fTimePosition.bbt.valid          = false;
            fTimePosition.bbt.beatsPerMinute = 120.0;
            fTimePosition.bbt.bar            = 1;
            fTimePosition.bbt.beat           = 1;
            fTimePosition.bbt.tick           = 0.0;
            fTimePosition.bbt.beatsPerBar    = 4.f;
            fTimePosition.bbt.beatType       = 4.f;
            fTimePosition.bbt.barStartTick   = 0;
        }

        fPlugin.setTimePosition(fTimePosition);
       #endif

        if (const clap_input_events_t* const inputEvents = process->in_events)
        {
            if (const uint32_t len = inputEvents->size(inputEvents))
            {
                for (uint32_t i=0; i<len; ++i)
                {
                    const clap_event_header_t* const event = inputEvents->get(inputEvents, i);

                    switch (event->type)
                    {
                    case CLAP_EVENT_NOTE_ON:
                       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                        // BUG: even though we only report CLAP_NOTE_DIALECT_MIDI as supported, Bitwig sends us this anyway
                        addNoteEvent(static_cast<const clap_event_note_t*>(static_cast<const void*>(event)), true);
                       #endif
                        break;
                    case CLAP_EVENT_NOTE_OFF:
                       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                        // BUG: even though we only report CLAP_NOTE_DIALECT_MIDI as supported, Bitwig sends us this anyway
                        addNoteEvent(static_cast<const clap_event_note_t*>(static_cast<const void*>(event)), false);
                       #endif
                        break;
                    case CLAP_EVENT_NOTE_CHOKE:
                    case CLAP_EVENT_NOTE_END:
                    case CLAP_EVENT_NOTE_EXPRESSION:
                        break;
                    case CLAP_EVENT_PARAM_VALUE:
                        DISTRHO_SAFE_ASSERT_UINT2_BREAK(event->size == sizeof(clap_event_param_value),
                                                        event->size, sizeof(clap_event_param_value));
                        setParameterValueFromEvent(static_cast<const clap_event_param_value*>(static_cast<const void*>(event)));
                        break;
                    case CLAP_EVENT_PARAM_MOD:
                    case CLAP_EVENT_PARAM_GESTURE_BEGIN:
                    case CLAP_EVENT_PARAM_GESTURE_END:
                    case CLAP_EVENT_TRANSPORT:
                    case CLAP_EVENT_MIDI:
                        DISTRHO_SAFE_ASSERT_UINT2_BREAK(event->size == sizeof(clap_event_midi_t),
                                                        event->size, sizeof(clap_event_midi_t));
                       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                        addMidiEvent(static_cast<const clap_event_midi_t*>(static_cast<const void*>(event)));
                       #endif
                        break;
                    case CLAP_EVENT_MIDI_SYSEX:
                    case CLAP_EVENT_MIDI2:
                        break;
                    }
                }
            }
        }

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (fMidiEventCount != kMaxMidiEvents && fNotesRingBuffer.isDataAvailableForReading())
        {
            uint8_t midiData[3];
            const uint32_t frame = fMidiEventCount != 0 ? fMidiEvents[fMidiEventCount-1].frame : 0;

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

        if (const uint32_t frames = process->frames_count)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            const float** const audioInputs = fAudioInputs;

            uint32_t in=0;
            for (uint32_t i=0; i<process->audio_inputs_count; ++i)
            {
                const clap_audio_buffer_t& inputs(process->audio_inputs[i]);
                DISTRHO_SAFE_ASSERT_CONTINUE(inputs.channel_count != 0);

                for (uint32_t j=0; j<inputs.channel_count; ++j, ++in)
                    audioInputs[in] = const_cast<const float*>(inputs.data32[j]);
            }

            if (fUsingCV)
            {
                for (; in<DISTRHO_PLUGIN_NUM_INPUTS; ++in)
                    audioInputs[in] = nullptr;
            }
            else
            {
                DISTRHO_SAFE_ASSERT_UINT2_RETURN(in == DISTRHO_PLUGIN_NUM_INPUTS,
                                                 in, process->audio_inputs_count, false);
            }
           #else
            constexpr const float** const audioInputs = nullptr;
           #endif

           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            float** const audioOutputs = fAudioOutputs;

            uint32_t out=0;
            for (uint32_t i=0; i<process->audio_outputs_count; ++i)
            {
                const clap_audio_buffer_t& outputs(process->audio_outputs[i]);
                DISTRHO_SAFE_ASSERT_CONTINUE(outputs.channel_count != 0);

                for (uint32_t j=0; j<outputs.channel_count; ++j, ++out)
                    audioOutputs[out] = outputs.data32[j];
            }

            if (fUsingCV)
            {
                for (; out<DISTRHO_PLUGIN_NUM_OUTPUTS; ++out)
                    audioOutputs[out] = nullptr;
            }
            else
            {
                DISTRHO_SAFE_ASSERT_UINT2_RETURN(out == DISTRHO_PLUGIN_NUM_OUTPUTS,
                                                 out, DISTRHO_PLUGIN_NUM_OUTPUTS, false);
            }
           #else
            constexpr float** const audioOutputs = nullptr;
           #endif

            fOutputEvents = process->out_events;

           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            fPlugin.run(audioInputs, audioOutputs, frames, fMidiEvents, fMidiEventCount);
           #else
            fPlugin.run(audioInputs, audioOutputs, frames);
           #endif

            flushParameters(nullptr, process->out_events, frames - 1);

            fOutputEvents = nullptr;
        }

       #if DISTRHO_PLUGIN_WANT_LATENCY
        checkForLatencyChanges(true, false);
       #endif

        return true;
    }

    void onMainThread()
    {
       #if DISTRHO_PLUGIN_WANT_LATENCY
        reportLatencyChangeIfNeeded();
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // parameters

    uint32_t getParameterCount() const
    {
        return fPlugin.getParameterCount();
    }

    bool getParameterInfo(const uint32_t index, clap_param_info_t* const info) const
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        if (fPlugin.getParameterDesignation(index) == kParameterDesignationBypass)
        {
            info->flags = CLAP_PARAM_IS_STEPPED|CLAP_PARAM_IS_BYPASS|CLAP_PARAM_IS_AUTOMATABLE;
            std::strcpy(info->name, "Bypass");
            std::strcpy(info->module, "dpf_bypass");
        }
        else
        {
            const uint32_t hints = fPlugin.getParameterHints(index);
            const uint32_t groupId = fPlugin.getParameterGroupId(index);

            info->flags = 0;
            if (hints & kParameterIsOutput)
                info->flags |= CLAP_PARAM_IS_READONLY;
            else if (hints & kParameterIsAutomatable)
                info->flags |= CLAP_PARAM_IS_AUTOMATABLE;

            if (hints & (kParameterIsBoolean|kParameterIsInteger))
                info->flags |= CLAP_PARAM_IS_STEPPED;

            d_strncpy(info->name, fPlugin.getParameterName(index), CLAP_NAME_SIZE);

            uint wrtn;
            if (groupId != kPortGroupNone)
            {
                const PortGroupWithId& portGroup(fPlugin.getPortGroupById(groupId));
                strncpy(info->module, portGroup.symbol, CLAP_PATH_SIZE / 2);
                info->module[CLAP_PATH_SIZE / 2] = '\0';
                wrtn = std::strlen(info->module);
                info->module[wrtn++] = '/';
            }
            else
            {
                wrtn = 0;
            }

            d_strncpy(info->module + wrtn, fPlugin.getParameterSymbol(index), CLAP_PATH_SIZE - wrtn);
        }

        info->id = index;
        info->cookie = nullptr;
        info->min_value = ranges.min;
        info->max_value = ranges.max;
        info->default_value = ranges.def;
        return true;
    }

    bool getParameterValue(const clap_id param_id, double* const value) const
    {
        *value = fPlugin.getParameterValue(param_id);
        return true;
    }

    bool getParameterStringForValue(const clap_id param_id, double value, char* const display, const uint32_t size) const
    {
        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(param_id));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(param_id));
        const uint32_t hints = fPlugin.getParameterHints(param_id);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) * 0.5f;
            value = value > midRange ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (d_isEqual(static_cast<double>(enumValues.values[i].value), value))
            {
                d_strncpy(display, enumValues.values[i].label, size);
                return true;
            }
        }

        if (hints & kParameterIsInteger)
            snprintf_i32(display, value, size);
        else
            snprintf_f32(display, value, size);

        return true;
    }

    bool getParameterValueForString(const clap_id param_id, const char* const display, double* const value) const
    {
        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(param_id));
        const bool isInteger = fPlugin.isParameterInteger(param_id);

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (std::strcmp(display, enumValues.values[i].label) == 0)
            {
                *value = enumValues.values[i].value;
                return true;
            }
        }

        if (isInteger)
            *value = std::atoi(display);
        else
            *value = std::atof(display);

        return true;
    }

    void flushParameters(const clap_input_events_t* const in,
                         const clap_output_events_t* const out,
                         const uint32_t frameOffset)
    {
        if (const uint32_t len = in != nullptr ? in->size(in) : 0)
        {
            for (uint32_t i=0; i<len; ++i)
            {
                const clap_event_header_t* const event = in->get(in, i);

                if (event->type != CLAP_EVENT_PARAM_VALUE)
                    continue;

                DISTRHO_SAFE_ASSERT_UINT2_BREAK(event->size == sizeof(clap_event_param_value),
                                                event->size, sizeof(clap_event_param_value));
                
                setParameterValueFromEvent(static_cast<const clap_event_param_value*>(static_cast<const void*>(event)));
            }
        }

        if (out != nullptr)
        {
            clap_event_param_value_t clapEvent = {
                { sizeof(clap_event_param_value_t), frameOffset, 0, CLAP_EVENT_PARAM_VALUE, CLAP_EVENT_IS_LIVE },
                0, nullptr, 0, 0, 0, 0, 0.0
            };

            float value;
            for (uint i=0; i<fCachedParameters.numParams; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                {
                    value = fPlugin.getParameterValue(i);

                    if (d_isEqual(fCachedParameters.values[i], value))
                        continue;

                    fCachedParameters.values[i] = value;
                    fCachedParameters.changed[i] = true;

                    clapEvent.param_id = i;
                    clapEvent.value = value;
                    out->try_push(out, &clapEvent.header);
                }
            }
        }

       #if DISTRHO_PLUGIN_WANT_LATENCY
        const bool active = fPlugin.isActive();
        checkForLatencyChanges(active, !active);
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void addNoteEvent(const clap_event_note_t* const event, const bool isOn) noexcept
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(event->port_index == 0, event->port_index,);

        if (fMidiEventCount == kMaxMidiEvents)
            return;

        MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
        midiEvent.frame = event->header.time;
        midiEvent.size  = 3;
        midiEvent.data[0] = (isOn ? 0x90 : 0x80) | (event->channel & 0x0F);
        midiEvent.data[1] = std::max(0, std::min(127, static_cast<int>(event->key)));
        midiEvent.data[2] = std::max(0, std::min(127, static_cast<int>(event->velocity * 127 + 0.5)));
    }

    void addMidiEvent(const clap_event_midi_t* const event) noexcept
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(event->port_index == 0, event->port_index,);

        if (fMidiEventCount == kMaxMidiEvents)
            return;

        MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
        midiEvent.frame = event->header.time;
        midiEvent.size  = 3;
        std::memcpy(midiEvent.data, event->data, 3);
    }
   #endif

    void setParameterValueFromEvent(const clap_event_param_value* const event)
    {
        fCachedParameters.values[event->param_id] = event->value;
        fCachedParameters.changed[event->param_id] = true;
        fPlugin.setParameterValue(event->param_id, event->value);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // audio ports

   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    template<bool isInput>
    uint32_t getAudioPortCount() const noexcept
    {
        return (isInput ? fAudioInputBuses : fAudioOutputBuses).size();
    }

    template<bool isInput>
    bool getAudioPortInfo(const uint32_t index, clap_audio_port_info_t* const info) const noexcept
    {
        const std::vector<BusInfo>& busInfos(isInput ? fAudioInputBuses : fAudioOutputBuses);
        DISTRHO_SAFE_ASSERT_RETURN(index < busInfos.size(), false);

        const BusInfo& busInfo(busInfos[index]);

        info->id = busInfo.groupId;
        d_strncpy(info->name, busInfo.name, CLAP_NAME_SIZE);

        info->flags = busInfo.isMain ? CLAP_AUDIO_PORT_IS_MAIN : 0x0;
        info->channel_count = busInfo.numChannels;

        switch (busInfo.groupId)
        {
        case kPortGroupMono:
            info->port_type = CLAP_PORT_MONO;
            break;
        case kPortGroupStereo:
            info->port_type = CLAP_PORT_STEREO;
            break;
        default:
            info->port_type = nullptr;
            break;
        }

        info->in_place_pair = busInfo.hasPair ? busInfo.groupId : CLAP_INVALID_ID;
        return true;
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // latency

   #if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatency() const noexcept
    {
        return fPlugin.getLatency();
    }

    void checkForLatencyChanges(const bool isActive, const bool fromMainThread)
    {
        const uint32_t latency = fPlugin.getLatency();

        if (fLastKnownLatency == latency)
            return;

        fLastKnownLatency = latency;

        if (isActive)
        {
            fLatencyChanged = true;
            fHost->request_restart(fHost);
        }
        else
        {
            // if this is main-thread we can report latency change directly
            if (fromMainThread || (fHostExtensions.threadCheck != nullptr && fHostExtensions.threadCheck->is_main_thread(fHost)))
            {
                fLatencyChanged = false;
                fHostExtensions.latency->changed(fHost);
            }
            // otherwise we need to request a main-thread callback
            else
            {
                fLatencyChanged = true;
                fHost->request_callback(fHost);
            }
        }
    }

    // called from main thread
    void reportLatencyChangeIfNeeded()
    {
        if (fLatencyChanged)
        {
            fLatencyChanged = false;
            fHostExtensions.latency->changed(fHost);
        }
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // state

    bool stateSave(const clap_ostream_t* const stream)
    {
        const uint32_t paramCount = fPlugin.getParameterCount();
       #if DISTRHO_PLUGIN_WANT_STATE
        const uint32_t stateCount = fPlugin.getStateCount();
       #else
        const uint32_t stateCount = 0;
       #endif

        if (stateCount == 0 && paramCount == 0)
        {
            char buffer = '\0';
            return stream->write(stream, &buffer, 1) == 1;
        }

       #if DISTRHO_PLUGIN_WANT_FULL_STATE
        // Update current state
        for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
        {
            const String& key = cit->first;
            fStateMap[key] = fPlugin.getStateValue(key);
        }
       #endif

        String state;

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        {
            String tmpStr("__dpf_program__\xff");
            tmpStr += String(fCurrentProgram);
            tmpStr += "\xff";

            state += tmpStr;
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        if (stateCount != 0)
        {
            state += "__dpf_state_begin__\xff";

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

                state += tmpStr;
            }

            state += "__dpf_state_end__\xff";
        }
       #endif

        if (paramCount != 0)
        {
            state += "__dpf_parameters_begin__\xff";

            for (uint32_t i=0; i<paramCount; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                    continue;

                // join key and value
                String tmpStr;
                tmpStr  = fPlugin.getParameterSymbol(i);
                tmpStr += "\xff";
                if (fPlugin.getParameterHints(i) & kParameterIsInteger)
                    tmpStr += String(static_cast<int>(std::round(fPlugin.getParameterValue(i))));
                else
                    tmpStr += String(fPlugin.getParameterValue(i));
                tmpStr += "\xff";

                state += tmpStr;
            }

            state += "__dpf_parameters_end__\xff";
        }

        // terminator
        state += "\xfe";

        state.replace('\xff', '\0');

        // now saving state, carefully until host written bytes matches full state size
        const char* buffer = state.buffer();
        const int32_t size = static_cast<int32_t>(state.length())+1;

        for (int32_t wrtntotal = 0, wrtn; wrtntotal < size; wrtntotal += wrtn)
        {
            wrtn = stream->write(stream, buffer, size - wrtntotal);
            DISTRHO_SAFE_ASSERT_INT_RETURN(wrtn > 0, wrtn, false);
        }

        return true;
    }

    bool stateLoad(const clap_istream_t* const stream)
    {
       #if DISTRHO_PLUGIN_HAS_UI
        ClapUI* const ui = fUI.get();
       #endif
        String key, value;
        bool hasValue = false;
        bool fillingKey = true; // if filling key or value
        char queryingType = 'i'; // can be 'n', 's' or 'p' (none, states, parameters)

        char buffer[512], orig;
        buffer[sizeof(buffer)-1] = '\xff';

        for (int32_t terminated = 0, read; terminated == 0;)
        {
            read = stream->read(stream, buffer, sizeof(buffer)-1);
            DISTRHO_SAFE_ASSERT_INT_RETURN(read >= 0, read, false);

            if (read == 0)
                return true;

            for (int32_t i = 0; i < read; ++i)
            {
                // found terminator, stop here
                if (buffer[i] == '\xfe')
                {
                    terminated = 1;
                    break;
                }

                // store character at read position
                orig = buffer[read];

                // place null character to create valid string
                buffer[read] = '\0';

                // append to temporary vars
                if (fillingKey)
                {
                    key += buffer + i;
                }
                else
                {
                    value += buffer + i;
                    hasValue = true;
                }

                // increase buffer offset by length of string
                i += std::strlen(buffer + i);

                // restore read character
                buffer[read] = orig;

                // if buffer offset points to null, we found the end of a string, lets check
                if (buffer[i] == '\0')
                {
                    // special keys
                    if (key == "__dpf_state_begin__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n',
                                                       queryingType, false);
                        queryingType = 's';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_state_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 's', queryingType, false);
                        queryingType = 'n';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_parameters_begin__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n',
                                                       queryingType, false);
                        queryingType = 'p';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_parameters_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'p', queryingType, false);
                        queryingType = 'x';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }

                    // no special key, swap between reading real key and value
                    fillingKey = !fillingKey;

                    // if there is no value yet keep reading until we have one
                    if (! hasValue)
                        continue;

                    if (key == "__dpf_program__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i', queryingType, false);
                        queryingType = 'n';

                        d_debug("found program '%s'", value.buffer());

                      #if DISTRHO_PLUGIN_WANT_PROGRAMS
                        const int program = std::atoi(value.buffer());
                        DISTRHO_SAFE_ASSERT_CONTINUE(program >= 0);

                        fCurrentProgram = static_cast<uint32_t>(program);
                        fPlugin.loadProgram(fCurrentProgram);

                       #if DISTRHO_PLUGIN_HAS_UI
                        if (ui != nullptr)
                            ui->setProgramFromPlugin(fCurrentProgram);
                       #endif
                      #endif
                    }
                    else if (queryingType == 's')
                    {
                        d_debug("found state '%s' '%s'", key.buffer(), value.buffer());

                       #if DISTRHO_PLUGIN_WANT_STATE
                        if (fPlugin.wantStateKey(key))
                        {
                            fStateMap[key] = value;
                            fPlugin.setState(key, value);

                           #if DISTRHO_PLUGIN_HAS_UI
                            if (ui != nullptr)
                                ui->setStateFromPlugin(key, value);
                           #endif
                        }
                       #endif
                    }
                    else if (queryingType == 'p')
                    {
                        d_debug("found parameter '%s' '%s'", key.buffer(), value.buffer());
                        float fvalue;

                        // find parameter with this symbol, and set its value
                        for (uint32_t j=0; j<fCachedParameters.numParams; ++j)
                        {
                            if (fPlugin.isParameterOutputOrTrigger(j))
                                continue;
                            if (fPlugin.getParameterSymbol(j) != key)
                                continue;

                            if (fPlugin.getParameterHints(j) & kParameterIsInteger)
                                fvalue = std::atoi(value.buffer());
                            else
                                fvalue = std::atof(value.buffer());

                            fCachedParameters.values[j] = fvalue;
                           #if DISTRHO_PLUGIN_HAS_UI
                            if (ui != nullptr)
                            {
                                // UI parameter updates are handled outside the read loop (after host param restart)
                                fCachedParameters.changed[j] = true;
                            }
                           #endif
                            fPlugin.setParameterValue(j, fvalue);
                            break;
                        }
                    }

                    key.clear();
                    value.clear();
                    hasValue = false;
                }
            }
        }

        if (fHostExtensions.params != nullptr)
            fHostExtensions.params->rescan(fHost, CLAP_PARAM_RESCAN_VALUES|CLAP_PARAM_RESCAN_TEXT);

       #if DISTRHO_PLUGIN_WANT_LATENCY
        checkForLatencyChanges(fPlugin.isActive(), true);
        reportLatencyChangeIfNeeded();
       #endif

       #if DISTRHO_PLUGIN_HAS_UI
        if (ui != nullptr)
        {
            for (uint32_t i=0; i<fCachedParameters.numParams; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                    continue;
                fCachedParameters.changed[i] = false;
                ui->setParameterValueFromPlugin(i, fCachedParameters.values[i]);
            }
        }
       #endif

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // gui

   #if DISTRHO_PLUGIN_HAS_UI
    bool createUI(const bool isFloating)
    {
        const clap_host_gui_t* const hostGui = getHostExtension<clap_host_gui_t>(CLAP_EXT_GUI);
        DISTRHO_SAFE_ASSERT_RETURN(hostGui != nullptr, false);

       #if DPF_CLAP_USING_HOST_TIMER
        const clap_host_timer_support_t* const hostTimer = getHostExtension<clap_host_timer_support_t>(CLAP_EXT_TIMER_SUPPORT);
        DISTRHO_SAFE_ASSERT_RETURN(hostTimer != nullptr, false);
       #endif

        fUI = new ClapUI(fPlugin, this, fHost, hostGui,
                        #if DPF_CLAP_USING_HOST_TIMER
                         hostTimer,
                        #endif
                         isFloating);
        return true;
    }

    void destroyUI()
    {
        fUI = nullptr;
    }

    ClapUI* getUI() const noexcept
    {
        return fUI.get();
    }
   #endif

   #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_STATE
    void setStateFromUI(const char* const key, const char* const value) override
    {
        fPlugin.setState(key, value);

        // check if we want to save this key
        if (! fPlugin.wantStateKey(key))
            return;

        // check if key already exists
        for (StringMap::iterator it=fStateMap.begin(), ite=fStateMap.end(); it != ite; ++it)
        {
            const String& dkey(it->first);

            if (dkey == key)
            {
                it->second = value;
                return;
            }
        }

        d_stderr("Failed to find plugin state with key \"%s\"", key);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin and UI
    PluginExporter fPlugin;
   #if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<ClapUI> fUI;
   #endif

    // CLAP stuff
    const clap_host_t* const fHost;
    const clap_output_events_t* fOutputEvents;

   #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    const float* fAudioInputs[DISTRHO_PLUGIN_NUM_INPUTS];
   #endif
   #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    float* fAudioOutputs[DISTRHO_PLUGIN_NUM_OUTPUTS];
   #endif
   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    bool fUsingCV;
   #endif
   #if DISTRHO_PLUGIN_WANT_LATENCY
    bool fLatencyChanged;
    uint32_t fLastKnownLatency;
   #endif
  #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    uint32_t fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
   #if DISTRHO_PLUGIN_HAS_UI
    RingBufferControl<SmallStackBuffer> fNotesRingBuffer;
   #endif
  #endif
   #if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
   #endif

    struct HostExtensions {
        const clap_host_t* const host;
        const clap_host_params_t* params;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        const clap_host_latency_t* latency;
        const clap_host_thread_check_t* threadCheck;
       #endif

        HostExtensions(const clap_host_t* const host)
            : host(host),
              params(nullptr)
           #if DISTRHO_PLUGIN_WANT_LATENCY
            , latency(nullptr)
            , threadCheck(nullptr)
           #endif
        {}

        bool init()
        {
            params = static_cast<const clap_host_params_t*>(host->get_extension(host, CLAP_EXT_PARAMS));
           #if DISTRHO_PLUGIN_WANT_LATENCY
            DISTRHO_SAFE_ASSERT_RETURN(host->request_restart != nullptr, false);
            DISTRHO_SAFE_ASSERT_RETURN(host->request_callback != nullptr, false);
            latency = static_cast<const clap_host_latency_t*>(host->get_extension(host, CLAP_EXT_LATENCY));
            threadCheck = static_cast<const clap_host_thread_check_t*>(host->get_extension(host, CLAP_EXT_THREAD_CHECK));
           #endif
            return true;
        }
    } fHostExtensions;

    // ----------------------------------------------------------------------------------------------------------------
    // helper functions for dealing with buses

   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    struct BusInfo {
        char name[CLAP_NAME_SIZE];
        uint32_t numChannels;
        bool hasPair;
        bool isCV;
        bool isMain;
        uint32_t groupId;
    };
    std::vector<BusInfo> fAudioInputBuses, fAudioOutputBuses;

    template<bool isInput>
    void fillInBusInfoDetails()
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        std::vector<BusInfo>& busInfos(isInput ? fAudioInputBuses : fAudioOutputBuses);

        enum {
            kPortTypeNull,
            kPortTypeAudio,
            kPortTypeSidechain,
            kPortTypeCV,
            kPortTypeGroup
        } lastSeenPortType = kPortTypeNull;
        uint32_t lastSeenGroupId = kPortGroupNone;
        uint32_t nonGroupAudioId = 0;
        uint32_t nonGroupSidechainId = 0x20000000;

        for (uint32_t i=0; i<numPorts; ++i)
        {
            const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

            if (port.groupId != kPortGroupNone)
            {
                if (lastSeenPortType != kPortTypeGroup || lastSeenGroupId != port.groupId)
                {
                    lastSeenPortType = kPortTypeGroup;
                    lastSeenGroupId = port.groupId;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // if this is the first port, set it as main
                        busInfos.empty(),
                        // user given group id with extra safety offset
                        port.groupId + 0x80000000
                    };

                    const PortGroupWithId& group(fPlugin.getPortGroupById(port.groupId));

                    switch (port.groupId)
                    {
                    case kPortGroupStereo:
                    case kPortGroupMono:
                        if (busInfo.isMain)
                        {
                            d_strncpy(busInfo.name, isInput ? "Audio Input" : "Audio Output", CLAP_NAME_SIZE);
                            break;
                        }
                    // fall-through
                    default:
                        if (group.name.isNotEmpty())
                            d_strncpy(busInfo.name, group.name, CLAP_NAME_SIZE);
                        else
                            d_strncpy(busInfo.name, port.name, CLAP_NAME_SIZE);
                        break;
                    }

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
            else if (port.hints & kAudioPortIsCV)
            {
                // TODO
                lastSeenPortType = kPortTypeCV;
                lastSeenGroupId = kPortGroupNone;
                fUsingCV = true;
            }
            else if (port.hints & kAudioPortIsSidechain)
            {
                if (lastSeenPortType != kPortTypeSidechain)
                {
                    lastSeenPortType = kPortTypeSidechain;
                    lastSeenGroupId = kPortGroupNone;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // not main
                        false,
                        // give unique id
                        nonGroupSidechainId++
                    };

                    d_strncpy(busInfo.name, port.name, CLAP_NAME_SIZE);

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
            else
            {
                if (lastSeenPortType != kPortTypeAudio)
                {
                    lastSeenPortType = kPortTypeAudio;
                    lastSeenGroupId = kPortGroupNone;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // if this is the first port, set it as main
                        busInfos.empty(),
                        // give unique id
                        nonGroupAudioId++
                    };

                    if (busInfo.isMain)
                    {
                        d_strncpy(busInfo.name, isInput ? "Audio Input" : "Audio Output", CLAP_NAME_SIZE);
                    }
                    else
                    {
                        d_strncpy(busInfo.name, port.name, CLAP_NAME_SIZE);
                    }

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
        }
    }
   #endif

   #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    void fillInBusInfoPairs()
    {
        const size_t numChannels = std::min(fAudioInputBuses.size(), fAudioOutputBuses.size());

        for (size_t i=0; i<numChannels; ++i)
        {
            if (fAudioInputBuses[i].groupId != fAudioOutputBuses[i].groupId)
                break;
            if (fAudioInputBuses[i].numChannels != fAudioOutputBuses[i].numChannels)
                break;
            if (fAudioInputBuses[i].isMain != fAudioOutputBuses[i].isMain)
                break;

            fAudioInputBuses[i].hasPair = fAudioOutputBuses[i].hasPair = true;
        }
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fOutputEvents != nullptr, false);

        if (midiEvent.size > 3)
            return true;

        const clap_event_midi clapEvent = {
            { sizeof(clap_event_midi), midiEvent.frame, 0, CLAP_EVENT_MIDI, CLAP_EVENT_IS_LIVE },
            0, { midiEvent.data[0],
                 static_cast<uint8_t>(midiEvent.size >= 2 ? midiEvent.data[1] : 0),
                 static_cast<uint8_t>(midiEvent.size >= 3 ? midiEvent.data[2] : 0) }
        };
        return fOutputEvents->try_push(fOutputEvents, &clapEvent.header);
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginCLAP*>(ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginCLAP*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return static_cast<PluginCLAP*>(ptr)->updateState(key, value);
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

static ScopedPointer<PluginExporter> sPlugin;

// --------------------------------------------------------------------------------------------------------------------
// plugin gui

#if DISTRHO_PLUGIN_HAS_UI

static const char* const kSupportedAPIs[] = {
#if defined(DISTRHO_OS_WINDOWS)
    CLAP_WINDOW_API_WIN32,
#elif defined(DISTRHO_OS_MAC)
    CLAP_WINDOW_API_COCOA,
#else
    CLAP_WINDOW_API_X11,
#endif
};

// TODO DPF external UI
static bool CLAP_ABI clap_gui_is_api_supported(const clap_plugin_t*, const char* const api, bool)
{
    for (size_t i=0; i<ARRAY_SIZE(kSupportedAPIs); ++i)
    {
        if (std::strcmp(kSupportedAPIs[i], api) == 0)
            return true;
    }

    return false;
}

// TODO DPF external UI
static bool CLAP_ABI clap_gui_get_preferred_api(const clap_plugin_t*, const char** const api, bool* const is_floating)
{
    *api = kSupportedAPIs[0];
    *is_floating = false;
    return true;
}

static bool CLAP_ABI clap_gui_create(const clap_plugin_t* const plugin, const char* const api, const bool is_floating)
{
    for (size_t i=0; i<ARRAY_SIZE(kSupportedAPIs); ++i)
    {
        if (std::strcmp(kSupportedAPIs[i], api) == 0)
        {
            PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
            return instance->createUI(is_floating);
        }
    }

    return false;
}

static void CLAP_ABI clap_gui_destroy(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->destroyUI();
}

static bool CLAP_ABI clap_gui_set_scale(const clap_plugin_t* const plugin, const double scale)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
   #ifndef DISTRHO_OS_MAC
    return gui->setScaleFactor(scale);
   #else
    return true;
    // unused
    (void)scale;
   #endif
}

static bool CLAP_ABI clap_gui_get_size(const clap_plugin_t* const plugin, uint32_t* const width, uint32_t* const height)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->getSize(width, height);
}

static bool CLAP_ABI clap_gui_can_resize(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->canResize();
}

static bool CLAP_ABI clap_gui_get_resize_hints(const clap_plugin_t* const plugin, clap_gui_resize_hints_t* const hints)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->getResizeHints(hints);
}

static bool CLAP_ABI clap_gui_adjust_size(const clap_plugin_t* const plugin, uint32_t* const width, uint32_t* const height)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->adjustSize(width, height);
}

static bool CLAP_ABI clap_gui_set_size(const clap_plugin_t* const plugin, const uint32_t width, const uint32_t height)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->setSizeFromHost(width, height);
}

static bool CLAP_ABI clap_gui_set_parent(const clap_plugin_t* const plugin, const clap_window_t* const window)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->setParent(window);
}

static bool CLAP_ABI clap_gui_set_transient(const clap_plugin_t* const plugin, const clap_window_t* const window)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->setTransient(window);
}

static void CLAP_ABI clap_gui_suggest_title(const clap_plugin_t* const plugin, const char* const title)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr,);
    return gui->suggestTitle(title);
}

static bool CLAP_ABI clap_gui_show(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->show();
}

static bool CLAP_ABI clap_gui_hide(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr, false);
    return gui->hide();
}

static const clap_plugin_gui_t clap_plugin_gui = {
    clap_gui_is_api_supported,
    clap_gui_get_preferred_api,
    clap_gui_create,
    clap_gui_destroy,
    clap_gui_set_scale,
    clap_gui_get_size,
    clap_gui_can_resize,
    clap_gui_get_resize_hints,
    clap_gui_adjust_size,
    clap_gui_set_size,
    clap_gui_set_parent,
    clap_gui_set_transient,
    clap_gui_suggest_title,
    clap_gui_show,
    clap_gui_hide
};

// --------------------------------------------------------------------------------------------------------------------
// plugin timer

#if DPF_CLAP_USING_HOST_TIMER
static void CLAP_ABI clap_plugin_on_timer(const clap_plugin_t* const plugin, clap_id)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    ClapUI* const gui = instance->getUI();
    DISTRHO_SAFE_ASSERT_RETURN(gui != nullptr,);
    return gui->idleCallback();
}

static const clap_plugin_timer_support_t clap_timer = {
    clap_plugin_on_timer
};
#endif

#endif // DISTRHO_PLUGIN_HAS_UI

// --------------------------------------------------------------------------------------------------------------------
// plugin audio ports

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
static uint32_t CLAP_ABI clap_plugin_audio_ports_count(const clap_plugin_t* const plugin, const bool is_input)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return is_input ? instance->getAudioPortCount<true>()
                    : instance->getAudioPortCount<false>();
}

static bool CLAP_ABI clap_plugin_audio_ports_get(const clap_plugin_t* const plugin,
                                        const uint32_t index,
                                        const bool is_input,
                                        clap_audio_port_info_t* const info)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return is_input ? instance->getAudioPortInfo<true>(index, info)
                    : instance->getAudioPortInfo<false>(index, info);
}

static const clap_plugin_audio_ports_t clap_plugin_audio_ports = {
    clap_plugin_audio_ports_count,
    clap_plugin_audio_ports_get
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// plugin note ports

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT+DISTRHO_PLUGIN_WANT_MIDI_OUTPUT != 0
static uint32_t CLAP_ABI clap_plugin_note_ports_count(const clap_plugin_t*, const bool is_input)
{
    return (is_input ? DISTRHO_PLUGIN_WANT_MIDI_INPUT : DISTRHO_PLUGIN_WANT_MIDI_OUTPUT) != 0 ? 1 : 0;
}

static bool CLAP_ABI clap_plugin_note_ports_get(const clap_plugin_t*, uint32_t,
                                                const bool is_input, clap_note_port_info_t* const info)
{
    if (is_input)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        info->id = 0;
        info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
        info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
        std::strcpy(info->name, "Event/MIDI Input");
        return true;
       #endif
    }
    else
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        info->id = 0;
        info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
        info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
        std::strcpy(info->name, "Event/MIDI Output");
        return true;
       #endif
    }

    return false;
}

static const clap_plugin_note_ports_t clap_plugin_note_ports = {
    clap_plugin_note_ports_count,
    clap_plugin_note_ports_get
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// plugin parameters

static uint32_t CLAP_ABI clap_plugin_params_count(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterCount();
}

static bool CLAP_ABI clap_plugin_params_get_info(const clap_plugin_t* const plugin, const uint32_t index, clap_param_info_t* const info)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterInfo(index, info);
}

static bool CLAP_ABI clap_plugin_params_get_value(const clap_plugin_t* const plugin, const clap_id param_id, double* const value)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterValue(param_id, value);
}

static bool CLAP_ABI clap_plugin_params_value_to_text(const clap_plugin_t* plugin, const clap_id param_id, const double value, char* const display, const uint32_t size)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterStringForValue(param_id, value, display, size);
}

static bool CLAP_ABI clap_plugin_params_text_to_value(const clap_plugin_t* plugin, const clap_id param_id, const char* const display, double* const value)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterValueForString(param_id, display, value);
}

static void CLAP_ABI clap_plugin_params_flush(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->flushParameters(in, out, 0);
}

static const clap_plugin_params_t clap_plugin_params = {
    clap_plugin_params_count,
    clap_plugin_params_get_info,
    clap_plugin_params_get_value,
    clap_plugin_params_value_to_text,
    clap_plugin_params_text_to_value,
    clap_plugin_params_flush
};

#if DISTRHO_PLUGIN_WANT_LATENCY
// --------------------------------------------------------------------------------------------------------------------
// plugin latency

static uint32_t CLAP_ABI clap_plugin_latency_get(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getLatency();
}

static const clap_plugin_latency_t clap_plugin_latency = {
    clap_plugin_latency_get
};
#endif

#if DISTRHO_PLUGIN_WANT_STATE
// --------------------------------------------------------------------------------------------------------------------
// plugin state

static bool CLAP_ABI clap_plugin_state_save(const clap_plugin_t* const plugin, const clap_ostream_t* const stream)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->stateSave(stream);
}

static bool CLAP_ABI clap_plugin_state_load(const clap_plugin_t* const plugin, const clap_istream_t* const stream)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->stateLoad(stream);
}

static const clap_plugin_state_t clap_plugin_state = {
    clap_plugin_state_save,
    clap_plugin_state_load
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// plugin

static bool CLAP_ABI clap_plugin_init(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->init();
}

static void CLAP_ABI clap_plugin_destroy(const clap_plugin_t* const plugin)
{
    delete static_cast<PluginCLAP*>(plugin->plugin_data);
    std::free(const_cast<clap_plugin_t*>(plugin));
}

static bool CLAP_ABI clap_plugin_activate(const clap_plugin_t* const plugin,
                                          const double sample_rate,
                                          uint32_t,
                                          const uint32_t max_frames_count)
{
    d_nextBufferSize = max_frames_count;
    d_nextSampleRate = sample_rate;

    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->activate(sample_rate, max_frames_count);
    return true;
}

static void CLAP_ABI clap_plugin_deactivate(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->deactivate();
}

static bool CLAP_ABI clap_plugin_start_processing(const clap_plugin_t*)
{
    // nothing to do
    return true;
}

static void CLAP_ABI clap_plugin_stop_processing(const clap_plugin_t*)
{
    // nothing to do
}

static void CLAP_ABI clap_plugin_reset(const clap_plugin_t*)
{
    // nothing to do
}

static clap_process_status CLAP_ABI clap_plugin_process(const clap_plugin_t* const plugin, const clap_process_t* const process)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->process(process) ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_ERROR;
}

static const void* CLAP_ABI clap_plugin_get_extension(const clap_plugin_t*, const char* const id)
{
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &clap_plugin_params;
   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &clap_plugin_audio_ports;
   #endif
   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT+DISTRHO_PLUGIN_WANT_MIDI_OUTPUT != 0
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
        return &clap_plugin_note_ports;
   #endif
   #if DISTRHO_PLUGIN_WANT_LATENCY
    if (std::strcmp(id, CLAP_EXT_LATENCY) == 0)
        return &clap_plugin_latency;
   #endif
   #if DISTRHO_PLUGIN_WANT_STATE
    if (std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &clap_plugin_state;
   #endif
  #if DISTRHO_PLUGIN_HAS_UI
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &clap_plugin_gui;
   #if DPF_CLAP_USING_HOST_TIMER
    if (std::strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0)
        return &clap_timer;
   #endif
  #endif
    return nullptr;
}

static void CLAP_ABI clap_plugin_on_main_thread(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->onMainThread();
}

// --------------------------------------------------------------------------------------------------------------------
// plugin factory

static uint32_t CLAP_ABI clap_get_plugin_count(const clap_plugin_factory_t*)
{
    return 1;
}

static const clap_plugin_descriptor_t* CLAP_ABI clap_get_plugin_descriptor(const clap_plugin_factory_t*,
                                                                           const uint32_t index)
{
    DISTRHO_SAFE_ASSERT_UINT_RETURN(index == 0, index, nullptr);

    static const char* features[] = {
       #ifdef DISTRHO_PLUGIN_CLAP_FEATURES
        DISTRHO_PLUGIN_CLAP_FEATURES,
       #elif DISTRHO_PLUGIN_IS_SYNTH
        "instrument",
       #endif
        nullptr
    };

    static const clap_plugin_descriptor_t descriptor = {
        CLAP_VERSION,
        DISTRHO_PLUGIN_CLAP_ID,
        sPlugin->getName(),
        sPlugin->getMaker(),
        // TODO url
        "",
        // TODO manual url
        "",
        // TODO support url
        "",
        // TODO version string
        "",
        sPlugin->getDescription(),
        features
    };

    return &descriptor;
}

static const clap_plugin_t* CLAP_ABI clap_create_plugin(const clap_plugin_factory_t* const factory,
                                                        const clap_host_t* const host,
                                                        const char*)
{
    clap_plugin_t* const pluginptr = static_cast<clap_plugin_t*>(std::malloc(sizeof(clap_plugin_t)));
    DISTRHO_SAFE_ASSERT_RETURN(pluginptr != nullptr, nullptr);

    // default early values
    if (d_nextBufferSize == 0)
        d_nextBufferSize = 1024;
    if (d_nextSampleRate <= 0.0)
        d_nextSampleRate = 44100.0;

    d_nextCanRequestParameterValueChanges = true;

    const clap_plugin_t plugin = {
        clap_get_plugin_descriptor(factory, 0),
        new PluginCLAP(host),
        clap_plugin_init,
        clap_plugin_destroy,
        clap_plugin_activate,
        clap_plugin_deactivate,
        clap_plugin_start_processing,
        clap_plugin_stop_processing,
        clap_plugin_reset,
        clap_plugin_process,
        clap_plugin_get_extension,
        clap_plugin_on_main_thread
    };

    std::memcpy(pluginptr, &plugin, sizeof(clap_plugin_t));
    return pluginptr;
}

static const clap_plugin_factory_t clap_plugin_factory = {
    clap_get_plugin_count,
    clap_get_plugin_descriptor,
    clap_create_plugin
};

// --------------------------------------------------------------------------------------------------------------------
// plugin entry

static bool CLAP_ABI clap_plugin_entry_init(const char* const plugin_path)
{
    static String bundlePath;
    bundlePath = plugin_path;
    d_nextBundlePath = bundlePath.buffer();

    // init dummy plugin
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

    return true;
}

static void CLAP_ABI clap_plugin_entry_deinit(void)
{
    sPlugin = nullptr;
}

static const void* CLAP_ABI clap_plugin_entry_get_factory(const char* const factory_id)
{
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &clap_plugin_factory;
    return nullptr;
}

static const clap_plugin_entry_t clap_plugin_entry = {
    CLAP_VERSION,
    clap_plugin_entry_init,
    clap_plugin_entry_deinit,
    clap_plugin_entry_get_factory
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const clap_plugin_entry_t clap_entry = DISTRHO_NAMESPACE::clap_plugin_entry;

// --------------------------------------------------------------------------------------------------------------------
