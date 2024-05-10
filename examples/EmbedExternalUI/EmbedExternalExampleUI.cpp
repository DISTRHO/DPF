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

#include "DistrhoUI.hpp"
#include "Window.hpp"
#include "extra/WebView.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class EmbedExternalExampleUI : public UI
{
    WebViewHandle webview;

public:
    EmbedExternalExampleUI()
        : UI(),
          webview(nullptr)
    {
        const double scaleFactor = getScaleFactor();

        const uint width = DISTRHO_UI_DEFAULT_WIDTH * scaleFactor;
        const uint height = DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor;

        setGeometryConstraints(width, height);

        if (d_isNotEqual(scaleFactor, 1.0))
            setSize(width, height);

        webview = webViewCreate("https://distrho.github.io/DPF/",
                                getWindow().getNativeWindowHandle(),
                                width, height, scaleFactor);
    }

    ~EmbedExternalExampleUI()
    {
        if (webview != nullptr)
            webViewDestroy(webview);
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
        d_stdout("parameterChanged %u %f", index, value);

        switch (index)
        {
        case kParameterWidth:
            setWidth(static_cast<int>(value + 0.5f));
            break;
        case kParameterHeight:
            setHeight(static_cast<int>(value + 0.5f));
            break;
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * UI overrides */

    void onResize(const ResizeEvent& ev) override
    {
        UI::onResize(ev);

        if (webview != nullptr)
            webViewResize(webview, ev.size.getWidth(), ev.size.getHeight(), getScaleFactor());
    }

    void uiIdle() override
    {
        if (webview != nullptr)
            webViewIdle(webview);
    }

    // -------------------------------------------------------------------------------------------------------

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmbedExternalExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new EmbedExternalExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
