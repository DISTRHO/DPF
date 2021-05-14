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

#include "WindowPrivateData.hpp"

#include "pugl.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Window

// Window::Window(Window& transientParentWindow)
//     : pData(new PrivateData(transientParentWindow.pData->fAppData, this, transientParentWindow)) {}

Window::Window(Application& app)
    : pData(new PrivateData(app, this)) {}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const double scaling,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, scaling, resizable)) {}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const uint width,
               const uint height,
               const double scaling,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, width, height, scaling, resizable)) {}

Window::~Window()
{
    delete pData;
}

bool Window::isEmbed() const noexcept
{
    return pData->isEmbed;
}

bool Window::isVisible() const noexcept
{
    return pData->isVisible;
}

void Window::setVisible(const bool visible)
{
    if (visible)
        pData->show();
    else
        pData->hide();
}

void Window::show()
{
    pData->show();
}

void Window::hide()
{
    pData->hide();
}

void Window::close()
{
    pData->close();
}

bool Window::isResizable() const noexcept
{
    return puglGetViewHint(pData->view, PUGL_RESIZABLE) == PUGL_TRUE;
}

void Window::setResizable(const bool resizable)
{
    pData->setResizable(resizable);
}

uint Window::getWidth() const noexcept
{
    return puglGetFrame(pData->view).width;
}

uint Window::getHeight() const noexcept
{
    return puglGetFrame(pData->view).height;
}

Size<uint> Window::getSize() const noexcept
{
    const PuglRect rect = puglGetFrame(pData->view);
    return Size<uint>(rect.width, rect.height);
}

void Window::setWidth(const uint width)
{
    setSize(width, getHeight());
}

void Window::setHeight(const uint height)
{
    setSize(getWidth(), height);
}

void Window::setSize(const uint width, const uint height)
{
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(width > 1 && height > 1, width, height,);

    puglSetWindowSize(pData->view, width, height);
}

void Window::setSize(const Size<uint>& size)
{
    setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return puglGetWindowTitle(pData->view);
}

void Window::setTitle(const char* const title)
{
    puglSetWindowTitle(pData->view, title);
}

bool Window::isIgnoringKeyRepeat() const noexcept
{
    return puglGetViewHint(pData->view, PUGL_IGNORE_KEY_REPEAT) == PUGL_TRUE;
}

void Window::setIgnoringKeyRepeat(const bool ignore) noexcept
{
    puglSetViewHint(pData->view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

Application& Window::getApp() const noexcept
{
    return pData->app;
}

uintptr_t Window::getNativeWindowHandle() const noexcept
{
    return puglGetNativeWindow(pData->view);
}

double Window::getScaleFactor() const noexcept
{
    return pData->scaleFactor;
}

void Window::focus()
{
    if (! pData->isEmbed)
        puglRaiseWindow(pData->view);

    puglGrabFocus(pData->view);
}

void Window::repaint() noexcept
{
    puglPostRedisplay(pData->view);
}

void Window::repaint(const Rectangle<uint>& rect) noexcept
{
    const PuglRect prect = {
        static_cast<double>(rect.getX()),
        static_cast<double>(rect.getY()),
        static_cast<double>(rect.getWidth()),
        static_cast<double>(rect.getHeight()),
    };
    puglPostRedisplayRect(pData->view, prect);
}

void Window::setGeometryConstraints(const uint minimumWidth,
                                    const uint minimumHeight,
                                    const bool keepAspectRatio,
                                    const bool automaticallyScale)
{
    DISTRHO_SAFE_ASSERT_RETURN(minimumWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(minimumHeight > 0,);

    if (pData->isEmbed) {
        // Did you forget to set DISTRHO_UI_USER_RESIZABLE ?
        DISTRHO_SAFE_ASSERT_RETURN(isResizable(),);
    } else if (! isResizable()) {
        setResizable(true);
    }

    pData->minWidth = minimumWidth;
    pData->minHeight = minimumHeight;
    pData->autoScaling = automaticallyScale;

    const double scaleFactor = pData->scaleFactor;

    puglSetGeometryConstraints(pData->view,
                               minimumWidth * scaleFactor,
                               minimumHeight * scaleFactor,
                               keepAspectRatio);

    if (scaleFactor != 1.0)
    {
        const Size<uint> size(getSize());

        setSize(size.getWidth() * scaleFactor,
                size.getHeight() * scaleFactor);
    }
}

void Window::onReshape(uint, uint)
{
    puglFallbackOnResize(pData->view);
}

bool Window::onClose()
{
    return true;
}

#if 0
#if 0
void Window::exec(bool lockWait)
{
    pData->exec(lockWait);
}
#endif

void Window::setTransientWinId(const uintptr_t winId)
{
    puglSetTransientFor(pData->fView, winId);
}

#if 0
Application& Window::getApp() const noexcept
{
    return pData->fApp;
}
#endif

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
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
