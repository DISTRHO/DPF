/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginInfo.h"

#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class InfoExampleUI : public UI
{
public:
    InfoExampleUI()
        : UI(405, 256)
    {
        std::memset(fParameters, 0, sizeof(float)*kParameterCount);
        std::memset(fStrBuf, 0, sizeof(char)*(0xff+1));

        fSampleRate = getSampleRate();
        fFont       = createFontFromFile("sans", "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf");
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        fParameters[index] = value;
        repaint();
    }

   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks (optional) */

   /**
      Optional callback to inform the UI about a sample rate change on the plugin side.
    */
    void sampleRateChanged(double newSampleRate) override
    {
        fSampleRate = newSampleRate;
        repaint();
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The NanoVG drawing function.
    */
    void onNanoDisplay() override
    {
        static const float lineHeight = 20;

        fontSize(15.0f);
        textLineHeight(lineHeight);

        float x = 0;
        float y = 15;

        // buffer size
        drawLeft(x, y, "Buffer Size:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterBufferSize]));
        y+=lineHeight;

        // sample rate
        drawLeft(x, y, "Sample Rate:");
        drawRight(x, y, getTextBufFloat(fSampleRate));
        y+=lineHeight;

        // nothing
        y+=lineHeight;

        // time stuff
        drawLeft(x, y, "Playing:");
        drawRight(x, y, (fParameters[kParameterTimePlaying] > 0.5f) ? "Yes" : "No");
        y+=lineHeight;

        drawLeft(x, y, "Frame:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterTimeFrame]));
        y+=lineHeight;

        drawLeft(x, y, "Time:");
        drawRight(x, y, getTextBufTime(fParameters[kParameterTimeFrame]));
        y+=lineHeight;

        // BBT
        x = 200;
        y = 15;

        const bool validBBT(fParameters[kParameterTimeValidBBT] > 0.5f);
        drawLeft(x, y, "BBT Valid:");
        drawRight(x, y, validBBT ? "Yes" : "No");
        y+=lineHeight;

        if (! validBBT)
            return;

        drawLeft(x, y, "Bar:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterTimeBar]));
        y+=lineHeight;

        drawLeft(x, y, "Beat:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterTimeBeat]));
        y+=lineHeight;

        drawLeft(x, y, "Tick:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterTimeTick]));
        y+=lineHeight;

        drawLeft(x, y, "Bar Start Tick:");
        drawRight(x, y, getTextBufFloat(fParameters[kParameterTimeBarStartTick]));
        y+=lineHeight;

        drawLeft(x, y, "Beats Per Bar:");
        drawRight(x, y, getTextBufFloat(fParameters[kParameterTimeBeatsPerBar]));
        y+=lineHeight;

        drawLeft(x, y, "Beat Type:");
        drawRight(x, y, getTextBufFloat(fParameters[kParameterTimeBeatType]));
        y+=lineHeight;

        drawLeft(x, y, "Ticks Per Beat:");
        drawRight(x, y, getTextBufFloat(fParameters[kParameterTimeTicksPerBeat]));
        y+=lineHeight;

        drawLeft(x, y, "BPM:");
        drawRight(x, y, getTextBufFloat(fParameters[kParameterTimeBeatsPerMinute]));
        y+=lineHeight;
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Parameters
    float  fParameters[kParameterCount];
    double fSampleRate;

    // font
    FontId fFont;

    // temp buf for text
    char fStrBuf[0xff+1];

    // helpers for putting text into fStrBuf and returning it
    const char* getTextBufInt(const int value)
    {
        std::snprintf(fStrBuf, 0xff, "%i", value);
        return fStrBuf;
    }

    const char* getTextBufFloat(const float value)
    {
        std::snprintf(fStrBuf, 0xff, "%.1f", value);
        return fStrBuf;
    }

    const char* getTextBufTime(const uint64_t frame)
    {
        const uint32_t time = frame / uint64_t(fSampleRate);
        const uint32_t secs =  time % 60;
        const uint32_t mins = (time / 60) % 60;
        const uint32_t hrs  = (time / 3600) % 60;
        std::snprintf(fStrBuf, 0xff, "%02i:%02i:%02i", hrs, mins, secs);
        return fStrBuf;
    }

    // helpers for drawing text
    void drawLeft(const float x, const float y, const char* const text)
    {
        beginPath();
        fillColor(200, 200, 200);
        textAlign(ALIGN_RIGHT|ALIGN_TOP);
        textBox(x, y, 100, text);
        closePath();
    }

    void drawRight(const float x, const float y, const char* const text)
    {
        beginPath();
        fillColor(255, 255, 255);
        textAlign(ALIGN_LEFT|ALIGN_TOP);
        textBox(x+105, y, 100, text);
        closePath();
    }

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfoExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new InfoExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
