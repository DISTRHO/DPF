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

#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app.pData, this)) {}

Window::Window(Window& transientParentWindow)
    : pData(new PrivateData(transientParentWindow.pData->fAppData, this, transientParentWindow)) {}

Window::Window(Application& app, const uintptr_t parentWindowHandle, const double scaling, const bool resizable)
    : pData(new PrivateData(app.pData, this, parentWindowHandle, scaling, resizable)) {}

Window::~Window()
{
    delete pData;
}

void Window::show()
{
    pData->setVisible(true);
}

void Window::hide()
{
    pData->setVisible(false);
}

void Window::close()
{
    pData->close();
}

#if 0
void Window::exec(bool lockWait)
{
    pData->exec(lockWait);
}
#endif

void Window::focus()
{
    if (! pData->fUsingEmbed)
        puglRaiseWindow(pData->fView);

    puglGrabFocus(pData->fView);
}

void Window::repaint() noexcept
{
    puglPostRedisplay(pData->fView);
}

void Window::repaint(const Rectangle<uint>& rect) noexcept
{
    const PuglRect prect = {
        static_cast<double>(rect.getX()),
        static_cast<double>(rect.getY()),
        static_cast<double>(rect.getWidth()),
        static_cast<double>(rect.getHeight()),
    };
    puglPostRedisplayRect(pData->fView, prect);
}

bool Window::isEmbed() const noexcept
{
    return pData->fUsingEmbed;
}

bool Window::isVisible() const noexcept
{
    return pData->fVisible;
}

void Window::setVisible(const bool visible)
{
    pData->setVisible(visible);
}

bool Window::isResizable() const noexcept
{
    return puglGetViewHint(pData->fView, PUGL_RESIZABLE) == PUGL_TRUE;
}

void Window::setResizable(const bool resizable)
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->fUsingEmbed,);
    if (pData->fUsingEmbed)
    {
        DGL_DBG("Window setResizable cannot be called when embedded\n");
        return;
    }

    DGL_DBG("Window setResizable called\n");

    puglSetViewHint(pData->fView, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
#ifdef DISTRHO_OS_WINDOWS
    puglWin32SetWindowResizable(pData->fView, resizable);
#endif
}

bool Window::getIgnoringKeyRepeat() const noexcept
{
    return puglGetViewHint(pData->fView, PUGL_IGNORE_KEY_REPEAT) == PUGL_TRUE;
}

void Window::setIgnoringKeyRepeat(const bool ignore) noexcept
{
    puglSetViewHint(pData->fView, PUGL_IGNORE_KEY_REPEAT, ignore);
}

void Window::setGeometryConstraints(const uint width, const uint height, bool aspect)
{
    // Did you forget to set DISTRHO_UI_USER_RESIZABLE ?
    DISTRHO_SAFE_ASSERT_RETURN(isResizable(),);

    puglUpdateGeometryConstraints(pData->fView, width, height, aspect);
}

uint Window::getWidth() const noexcept
{
    return puglGetFrame(pData->fView).width;
}

uint Window::getHeight() const noexcept
{
    return puglGetFrame(pData->fView).height;
}

Size<uint> Window::getSize() const noexcept
{
    const PuglRect rect = puglGetFrame(pData->fView);
    return Size<uint>(rect.width, rect.height);
}

void Window::setSize(const uint width, const uint height)
{
    DISTRHO_SAFE_ASSERT_INT2_RETURN(width > 1 && height > 1, width, height,);

    puglSetWindowSize(pData->fView, width, height);
}

void Window::setSize(const Size<uint> size)
{
    setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return puglGetWindowTitle(pData->fView);
}

void Window::setTitle(const char* const title)
{
    puglSetWindowTitle(pData->fView, title);
}

void Window::setTransientWinId(const uintptr_t winId)
{
    puglSetTransientFor(pData->fView, winId);
}

double Window::getScaling() const noexcept
{
    return pData->fScaling;
}

#if 0
Application& Window::getApp() const noexcept
{
    return pData->fApp;
}
#endif

uintptr_t Window::getNativeWindowHandle() const noexcept
{
    return puglGetNativeWindow(pData->fView);
}

#if 0
const GraphicsContext& Window::getGraphicsContext() const noexcept
{
    GraphicsContext& context = pData->fContext;
#ifdef DGL_CAIRO
    context.cairo = (cairo_t*)puglGetContext(pData->fView);
#endif
    return context;
}
#endif

void Window::_setAutoScaling(double scaling) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(scaling > 0.0,);

    pData->fAutoScaling = scaling;
}

void Window::_addWidget(Widget* const widget)
{
    pData->addWidget(widget);
}

void Window::_removeWidget(Widget* const widget)
{
    pData->removeWidget(widget);
}

void Window::_idle()
{
    pData->windowSpecificIdle();
}

// -----------------------------------------------------------------------

void Window::addIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fAppData->idleCallbacks.push_back(callback);
}

void Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fAppData->idleCallbacks.remove(callback);
}

// -----------------------------------------------------------------------

void Window::onDisplayBefore()
{
    PrivateData::Fallback::onDisplayBefore();
}

void Window::onDisplayAfter()
{
    PrivateData::Fallback::onDisplayAfter();
}

void Window::onReshape(const uint width, const uint height)
{
    PrivateData::Fallback::onReshape(width, height);
}

void Window::onClose()
{
}

#ifndef DGL_FILE_BROWSER_DISABLED
void Window::fileBrowserSelected(const char*)
{
}
#endif

bool Window::handlePluginKeyboard(const bool press, const uint key)
{
    // TODO
    return false;
    // return pData->handlePluginKeyboard(press, key);
}

bool Window::handlePluginSpecial(const bool press, const Key key)
{
    // TODO
    return false;
    // return pData->handlePluginSpecial(press, key);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
