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

// we need this for now
//#define PUGL_GRAB_FOCUS 1

// #include "../Base.hpp"
#include "WindowPrivateData.hpp"

// #include "../../distrho/extra/String.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app, this)) {}

Window::Window(Application& app, Window& parent)
    : pData(new PrivateData(app, this, parent)) {}

Window::Window(Application& app, intptr_t parentId, double scaling, bool resizable)
    : pData(new PrivateData(app, this, parentId, scaling, resizable)) {}

Window::~Window()
{
    delete pData;
}

#if 0
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

void Window::exec(bool lockWait)
{
    pData->exec(lockWait);
}

void Window::focus()
{
    pData->focus();
}

void Window::repaint() noexcept
{
    puglPostRedisplay(pData->fView);
}

bool Window::isEmbed() const noexcept
{
    return pData->fUsingEmbed;
}

bool Window::isVisible() const noexcept
{
    return pData->fVisible;
}

void Window::setVisible(bool yesNo)
{
    pData->setVisible(yesNo);
}

bool Window::isResizable() const noexcept
{
    return pData->fResizable;
}

void Window::setResizable(bool yesNo)
{
    pData->setResizable(yesNo);
}

void Window::setGeometryConstraints(uint width, uint height, bool aspect)
{
    pData->setGeometryConstraints(width, height, aspect);
}

uint Window::getWidth() const noexcept
{
    return pData->fWidth;
}

uint Window::getHeight() const noexcept
{
    return pData->fHeight;
}

Size<uint> Window::getSize() const noexcept
{
    return Size<uint>(pData->fWidth, pData->fHeight);
}

void Window::setSize(uint width, uint height)
{
    pData->setSize(width, height);
}

void Window::setSize(Size<uint> size)
{
    pData->setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return pData->getTitle();
}

void Window::setTitle(const char* title)
{
    pData->setTitle(title);
}

void Window::setTransientWinId(uintptr_t winId)
{
    pData->setTransientWinId(winId);
}

double Window::getScaling() const noexcept
{
    return pData->getScaling();
}

bool Window::getIgnoringKeyRepeat() const noexcept
{
    return pData->getIgnoringKeyRepeat();
}

void Window::setIgnoringKeyRepeat(bool ignore) noexcept
{
    pData->setIgnoringKeyRepeat(ignore);
}
#endif

Application& Window::getApp() const noexcept
{
    return pData->fApp;
}

uintptr_t Window::getWindowId() const noexcept
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

void Window::_setAutoScaling(double scaling) noexcept
{
    pData->setAutoScaling(scaling);
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
    pData->idle();
}
#endif

// -----------------------------------------------------------------------

void Window::addIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.push_back(callback);
}

void Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.remove(callback);
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

#if 0
bool Window::handlePluginKeyboard(const bool press, const uint key)
{
    return pData->handlePluginKeyboard(press, key);
}

bool Window::handlePluginSpecial(const bool press, const Key key)
{
    return pData->handlePluginSpecial(press, key);
}
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#undef DBG
#undef DBGF
