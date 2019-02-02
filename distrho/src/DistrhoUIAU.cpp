/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUIInternal.hpp"

#include "../extra/Sleep.hpp"

START_NAMESPACE_DISTRHO

//#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static const sendNoteFunc sendNoteCallback = nullptr;
//#endif

// -----------------------------------------------------------------------

class UIAu
{
public:
    UIAu(const char* const uiTitle)
        : fUI(this, 0, nullptr, setParameterCallback, setStateCallback, sendNoteCallback, setSizeCallback),
          fHostClosed(false)
    {
        fUI.setWindowTitle(uiTitle);
    }

    ~UIAu()
    {
    }

    void exec()
    {
        for (;;)
        {
            if (fHostClosed || ! fUI.idle())
                break;

            d_msleep(30);
        }
    }

    // -------------------------------------------------------------------

#if 0
#if DISTRHO_PLUGIN_WANT_STATE
    void auui_configure(const char* key, const char* value)
    {
        fUI.stateChanged(key, value);
    }
#endif
#endif

    void auui_control(ulong index, float value)
    {
        fUI.parameterChanged(index, value);
    }

#if 0
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void auui_program(ulong bank, ulong program)
    {
        fUI.programLoaded(bank * 128 + program);
    }
#endif
#endif

    void auui_samplerate(const double sampleRate)
    {
        fUI.setSampleRate(sampleRate, true);
    }

    void auui_show()
    {
        fUI.setWindowVisible(true);
    }

    void auui_hide()
    {
        fUI.setWindowVisible(false);
    }

    void auui_quit()
    {
        fHostClosed = true;
        fUI.quit();
    }

    // -------------------------------------------------------------------

protected:
    void setParameterValue(const uint32_t rindex, const float value)
    {
        //FIXME fUI.setParameterValue(rindex, value);
    }

    void setState(const char* const key, const char* const value)
    {
    }

#if 0
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (fOscData.server == nullptr)
            return;
        if (channel > 0xF)
            return;

        uint8_t mdata[4] = {
            0,
            static_cast<uint8_t>(channel + (velocity != 0 ? 0x90 : 0x80)),
            note,
            velocity
        };
        fOscData.send_midi(mdata);
    }
#endif
#endif

    void setSize(const uint width, const uint height)
    {
        fUI.setWindowSize(width, height);
    }

private:
    UIExporter fUI;
    bool fHostClosed;

    // -------------------------------------------------------------------
    // Callbacks

    #define uiPtr ((UIAu*)ptr)

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        uiPtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        uiPtr->setState(key, value);
    }

#if 0
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uiPtr->sendNote(channel, note, velocity);
    }
#endif
#endif

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        uiPtr->setSize(width, height);
    }

    #undef uiPtr
};

// -----------------------------------------------------------------------

static const char* gUiTitle = nullptr;
static UIAu*     globalUI = nullptr;

static void initUiIfNeeded()
{
    if (globalUI != nullptr)
        return;

    if (d_lastUiSampleRate == 0.0)
        d_lastUiSampleRate = 44100.0;

    globalUI = new UIAu(gUiTitle);
}

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    USE_NAMESPACE_DISTRHO

    // dummy test mode
    if (argc == 1)
    {
        gUiTitle = "AU UI Test";

        initUiIfNeeded();
        globalUI->auui_show();
        globalUI->exec();

        delete globalUI;
        globalUI = nullptr;

        return 0;
    }
}
