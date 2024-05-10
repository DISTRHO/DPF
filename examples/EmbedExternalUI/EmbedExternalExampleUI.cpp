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
#include "NativeWindow.hpp"
#include "extra/WebView.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class EmbedExternalExampleUI : public UI,
                               public NativeWindow::Callbacks
{
    ScopedPointer<NativeWindow> window;
    WebViewHandle webview;

public:
    EmbedExternalExampleUI()
        : UI(),
          webview(nullptr)
    {
        const bool standalone = isStandalone();
        const double scaleFactor = getScaleFactor();
        d_stdout("isStandalone %d", (int)standalone);

        const uint width = DISTRHO_UI_DEFAULT_WIDTH * scaleFactor;
        const uint height = DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor;

        window = new NativeWindow(this, getTitle(), getParentWindowHandle(), width, height, standalone);
        webview = webViewCreate("https://distrho.github.io/DPF/",
                                window->getNativeWindowHandle(),
                                width, height, scaleFactor);

        setGeometryConstraints(width, height);

        if (d_isNotEqual(scaleFactor, 1.0))
            setSize(width, height);

        d_stdout("created external window with size %u %u", getWidth(), getHeight());
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
    * External Window callbacks */

    void nativeHide() override
    {
        d_stdout("nativeHide");
        UI::hide();
    }

    void nativeResize(const uint width, const uint height) override
    {
        d_stdout("nativeResize");
        setSize(width, height);
    }

   /* --------------------------------------------------------------------------------------------------------
    * External Window overrides */

    void focus() override
    {
        d_stdout("focus");
        window->focus();
    }

    uintptr_t getNativeWindowHandle() const noexcept override
    {
        return window->getNativeWindowHandle();
    }

    void sizeChanged(const uint width, const uint height) override
    {
        d_stdout("sizeChanged %u %u", width, height);
        UI::sizeChanged(width, height);

        window->setSize(width, height);

        if (webview != nullptr)
            webViewResize(webview, width, height, getScaleFactor());
    }

    void titleChanged(const char* const title) override
    {
        d_stdout("titleChanged %s", title);
        window->setTitle(title);
    }

    void transientParentWindowChanged(const uintptr_t winId) override
    {
        d_stdout("transientParentWindowChanged %lu", winId);
        window->setTransientParentWindow(winId);
    }

    void visibilityChanged(const bool visible) override
    {
        d_stdout("visibilityChanged %d", visible);
        window->setVisible(visible);
    }

    void uiIdle() override
    {
        // d_stdout("uiIdle");
        window->idle();

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
