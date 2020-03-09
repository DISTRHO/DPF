/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#include "extra/String.hpp"

#include "DistrhoPluginInfo.h"
#include "NanoButton.hpp"

START_NAMESPACE_DISTRHO

const char* kStateKeys[kStateCount] = {
    "file1",
    "file2",
    "file3",
};

// -----------------------------------------------------------------------------------------------------------

static void setupButton(Button& btn, int y)
{
    btn.setAbsolutePos(5, y);
    btn.setLabel("Open...");
    btn.setSize(100, 30);
}

class FileHandlingExampleUI : public UI,
                              public Button::Callback
{
public:
    static const uint kInitialWidth  = 600;
    static const uint kInitialHeight = 350;

    FileHandlingExampleUI()
        : UI(kInitialWidth, kInitialHeight),
          fButton1(this, this),
          fButton2(this, this),
          fButton3(this, this),
          fScale(1.0f)
    {
        std::memset(fParameters, 0, sizeof(fParameters));
        std::memset(fStrBuf, 0, sizeof(fStrBuf));

        setupButton(fButton1, 5);
        setupButton(fButton2, 105);
        setupButton(fButton3, 205);

#ifdef DGL_NO_SHARED_RESOURCES
        createFontFromFile("sans", "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf");
#else
        loadSharedResources();
#endif

        setGeometryConstraints(kInitialWidth, kInitialHeight, true);
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        fParameters[index] = value;
        repaint();
    }

   /**
      A state has changed on the plugin side.@n
      This is called by the host to inform the UI about state changes.
    */
    void stateChanged(const char* key, const char* value) override
    {
        States stateId = kStateCount;

        /**/ if (std::strcmp(key, "file1") == 0)
            stateId = kStateFile1;
        else if (std::strcmp(key, "file2") == 0)
            stateId = kStateFile2;
        else if (std::strcmp(key, "file3") == 0)
            stateId = kStateFile3;

        if (stateId == kStateCount)
            return;

        fState[stateId] = value;
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
        float y;

        fontSize(15.0f * fScale);
        textLineHeight(lineHeight);

        // ---------------------------------------------------------------------------------------
        // File 1

        y = 45.0f * fScale;

        if (fState[kStateFile1].isNotEmpty())
        {
            drawLeft(0.0f, y, "Name:");
            drawRight(0.0f, y, fState[kStateFile1]);
            y += lineHeight;

            drawLeft(0.0f, y, "Size:");
            drawRight(0.0f, y, getTextBufFileSize(fParameters[kParameterFileSize1]));
            y += lineHeight;
        }
        else
        {
            drawLeft(0.0f, y, "No file loaded");
        }

        // ---------------------------------------------------------------------------------------
        // File 2

        y = 145.0f * fScale;

        if (fState[kStateFile2].isNotEmpty())
        {
            drawLeft(0.0f, y, "Name:");
            drawRight(0.0f, y, fState[kStateFile2]);
            y += lineHeight;

            drawLeft(0.0f, y, "Size:");
            drawRight(0.0f, y, getTextBufFileSize(fParameters[kParameterFileSize2]));
            y += lineHeight;
        }
        else
        {
            drawLeft(0.0f, y, "No file loaded");
        }

        // ---------------------------------------------------------------------------------------
        // File 3

        y = 245.0f * fScale;

        if (fState[kStateFile3].isNotEmpty())
        {
            drawLeft(0.0f, y, "Name:");
            drawRight(0.0f, y, fState[kStateFile3]);
            y += lineHeight;

            drawLeft(0.0f, y, "Size:");
            drawRight(0.0f, y, getTextBufFileSize(fParameters[kParameterFileSize3]));
            y += lineHeight;
        }
        else
        {
            drawLeft(0.0f, y, "No file loaded");
        }
    }

    void onResize(const ResizeEvent& ev) override
    {
        fScale = static_cast<float>(ev.size.getHeight())/kInitialHeight;

        UI::onResize(ev);
    }

    void buttonClicked(Button* const button, bool) override
    {
        States stateId = kStateCount;

        /**/ if (button == &fButton1)
            stateId = kStateFile1;
        else if (button == &fButton2)
            stateId = kStateFile2;
        else if (button == &fButton3)
            stateId = kStateFile3;

        if (stateId == kStateCount)
            return;

        requestStateFile(kStateKeys[stateId]);
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Parameters
    float fParameters[kParameterCount];

    // State (files)
    String fState[kStateCount];
    Button fButton1, fButton2, fButton3;

    // UI stuff
    float fScale;

    // temp buf for text
    char fStrBuf[0xff];

    // helpers for putting text into fStrBuf and returning it
    const char* getTextBufFileSize(const float value)
    {
        /**/ if (value > 1024*1024)
            std::snprintf(fStrBuf, 0xfe, "%.2f GiB", value/(1024*1024));
        else if (value > 1024)
            std::snprintf(fStrBuf, 0xfe, "%.2f MiB", value/1024);
        else
            std::snprintf(fStrBuf, 0xfe, "%.2f KiB", value);
        return fStrBuf;
    }

    // helpers for drawing text
    void drawLeft(const float x, const float y, const char* const text)
    {
        beginPath();
        fillColor(200, 200, 200);
        textAlign(ALIGN_RIGHT|ALIGN_TOP);
        textBox(x, y, 100 * fScale, text);
        closePath();
    }

    void drawRight(const float x, const float y, const char* const text)
    {
        beginPath();
        fillColor(255, 255, 255);
        textAlign(ALIGN_LEFT|ALIGN_TOP);
        textBox(x + (105 * fScale), y, (kInitialWidth - x) * fScale, text);
        closePath();
    }

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileHandlingExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new FileHandlingExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
