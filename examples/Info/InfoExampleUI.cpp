/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUI.hpp"

#include "ResizeHandle.hpp"

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::ResizeHandle;

// -----------------------------------------------------------------------------------------------------------

class InfoExampleUI : public UI
{
public:
    InfoExampleUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          fSampleRate(getSampleRate()),
          fResizable(isResizable()),
          fScale(1.0f),
          fScaleFactor(getScaleFactor()),
          fResizeHandle(this)
    {
        std::memset(fParameters, 0, sizeof(float)*kParameterCount);
        std::memset(fStrBuf, 0, sizeof(char)*(0xff+1));

#ifdef DGL_NO_SHARED_RESOURCES
        createFontFromFile("sans", "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf");
#else
        loadSharedResources();
#endif

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // no need to show resize handle if window is user-resizable
        if (fResizable)
            fResizeHandle.hide();
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
        // some hosts send parameter change events for output parameters even when nothing changed
        // we catch that here in order to prevent excessive repaints
        if (d_isEqual(fParameters[index], value))
            return;

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
        const float lineHeight = 20 * fScale;

        fontSize(15.0f * fScale);
        textLineHeight(lineHeight);

        float x = 0.0f * fScale;
        float y = 15.0f * fScale;

        // buffer size
        drawLeft(x, y, "Buffer Size:");
        drawRight(x, y, getTextBufInt(fParameters[kParameterBufferSize]));
        y+=lineHeight;

        // sample rate
        drawLeft(x, y, "Sample Rate:");
        drawRight(x, y, getTextBufFloat(fSampleRate));
        y+=lineHeight;

        // separator
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

        // separator
        y+=lineHeight;

        // param changes
        drawLeft(x, y, "Param Changes:", 20);
        drawRight(x, y, (fParameters[kParameterCanRequestParameterValueChanges] > 0.5f) ? "Yes" : "No", 40);
        y+=lineHeight;

        // resizable
        drawLeft(x, y, "Host resizable:", 20);
        drawRight(x, y, fResizable ? "Yes" : "No", 40);
        y+=lineHeight;

        // host scale factor
        drawLeft(x, y, "Host scale factor:", 20);
        drawRight(x, y, getTextBufFloat(fScaleFactor), 40);
        y+=lineHeight;

        // BBT
        x = 200.0f * fScale;
        y = 15.0f * fScale;

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
        drawRight(x, y, getTextBufFloatExtra(fParameters[kParameterTimeTick]));
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

    void onResize(const ResizeEvent& ev) override
    {
        fScale = static_cast<float>(ev.size.getHeight())/static_cast<float>(DISTRHO_UI_DEFAULT_HEIGHT);

        UI::onResize(ev);
    }

    void uiScaleFactorChanged(const double scaleFactor) override
    {
        fScaleFactor = scaleFactor;
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Parameters
    float  fParameters[kParameterCount];
    double fSampleRate;

    // UI stuff
    bool fResizable;
    float fScale; // our internal scaling
    double fScaleFactor; // host reported scale factor
    ResizeHandle fResizeHandle;

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

    const char* getTextBufFloatExtra(const float value)
    {
        std::snprintf(fStrBuf, 0xff, "%.2f", value + 0.001f);
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
    void drawLeft(float x, const float y, const char* const text, const int offset = 0)
    {
        const float width = (100.0f + offset) * fScale;
        x += offset * fScale;
        beginPath();
        fillColor(200, 200, 200);
        textAlign(ALIGN_RIGHT|ALIGN_TOP);
        textBox(x, y, width, text);
        closePath();
    }

    void drawRight(float x, const float y, const char* const text, const int offset = 0)
    {
        const float width = (100.0f + offset) * fScale;
        x += offset * fScale;
        beginPath();
        fillColor(255, 255, 255);
        textAlign(ALIGN_LEFT|ALIGN_TOP);
        textBox(x + (105 * fScale), y, width, text);
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
