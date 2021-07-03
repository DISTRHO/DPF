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
// ScopedGraphicsContext

Window::ScopedGraphicsContext::ScopedGraphicsContext(Window& win)
    : window(win)
{
    puglBackendEnter(window.pData->view);
}

Window::ScopedGraphicsContext::~ScopedGraphicsContext()
{
    puglBackendLeave(window.pData->view);
}

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app, this))
{
    pData->initPost();
}

Window::Window(Application& app, Window& parent)
    : pData(new PrivateData(app, this, parent.pData))
{
    pData->initPost();
}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const double scaleFactor,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, scaleFactor, resizable))
{
    pData->initPost();
}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const uint width,
               const uint height,
               const double scaleFactor,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, width, height, scaleFactor, resizable))
{
    pData->initPost();
}

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
    const double width = puglGetFrame(pData->view).width;
    DISTRHO_SAFE_ASSERT_RETURN(width >= 0.0, 0);
    return static_cast<uint>(width + 0.5);
}

uint Window::getHeight() const noexcept
{
    const double height = puglGetFrame(pData->view).height;
    DISTRHO_SAFE_ASSERT_RETURN(height >= 0.0, 0);
    return static_cast<uint>(height + 0.5);
}

Size<uint> Window::getSize() const noexcept
{
    const PuglRect rect = puglGetFrame(pData->view);
    DISTRHO_SAFE_ASSERT_RETURN(rect.width >= 0.0, Size<uint>());
    DISTRHO_SAFE_ASSERT_RETURN(rect.height >= 0.0, Size<uint>());
    return Size<uint>(static_cast<uint>(rect.width + 0.5),
                      static_cast<uint>(rect.height + 0.5));
}

void Window::setWidth(const uint width)
{
    setSize(width, getHeight());
}

void Window::setHeight(const uint height)
{
    setSize(getWidth(), height);
}

void Window::setSize(uint width, uint height)
{
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(width > 1 && height > 1, width, height,);

    if (pData->isEmbed)
    {
        // handle geometry constraints here
        if (width < pData->minWidth)
            width = pData->minWidth;

        if (height < pData->minHeight)
            height = pData->minHeight;

        if (pData->keepAspectRatio)
        {
            const double ratio = static_cast<double>(pData->minWidth)
                               / static_cast<double>(pData->minHeight);
            const double reqRatio = static_cast<double>(width)
                                  / static_cast<double>(height);

            if (d_isNotEqual(ratio, reqRatio))
            {
                // fix width
                if (reqRatio > ratio)
                    width = height * ratio;
                // fix height
                else
                    height = width / ratio;
            }
        }
    }

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

bool Window::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr, false)

    return pData->addIdleCallback(callback, timerFrequencyInMs);
}

bool Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr, false)

    return pData->removeIdleCallback(callback);
}

Application& Window::getApp() const noexcept
{
    return pData->app;
}

#ifndef DPF_TEST_WINDOW_CPP
const GraphicsContext& Window::getGraphicsContext() const noexcept
{
    return pData->getGraphicsContext();
}
#endif

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
    pData->focus();
}

#ifndef DGL_FILE_BROWSER_DISABLED
bool Window::openFileBrowser(const FileBrowserOptions& options)
{
    return pData->openFileBrowser(options);
}
#endif

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

void Window::runAsModal(bool blockWait)
{
    pData->runAsModal(blockWait);
}

void Window::setGeometryConstraints(const uint minimumWidth,
                                    const uint minimumHeight,
                                    const bool keepAspectRatio,
                                    const bool automaticallyScale)
{
    DISTRHO_SAFE_ASSERT_RETURN(minimumWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(minimumHeight > 0,);

    if (pData->isEmbed) {
        // nothing to do here
    } else if (! isResizable()) {
        setResizable(true);
    }

    pData->minWidth = minimumWidth;
    pData->minHeight = minimumHeight;
    pData->autoScaling = automaticallyScale;
    pData->keepAspectRatio = keepAspectRatio;

    const double scaleFactor = pData->scaleFactor;

    puglSetGeometryConstraints(pData->view,
                               static_cast<uint>(minimumWidth * scaleFactor + 0.5),
                               static_cast<uint>(minimumHeight * scaleFactor + 0.5),
                               keepAspectRatio);

    if (scaleFactor != 1.0)
    {
        const Size<uint> size(getSize());

        setSize(static_cast<uint>(size.getWidth() * scaleFactor + 0.5),
                static_cast<uint>(size.getHeight() * scaleFactor + 0.5));
    }
}

bool Window::onClose()
{
    return true;
}

void Window::onFocus(bool, CrossingMode)
{
}

void Window::onReshape(uint, uint)
{
    puglFallbackOnResize(pData->view);
}

void Window::onScaleFactorChanged(double)
{
}

#ifndef DGL_FILE_BROWSER_DISABLED
void Window::onFileSelected(const char*)
{
}
#endif

#if 0
void Window::setTransientWinId(const uintptr_t winId)
{
    puglSetTransientFor(pData->view, winId);
}

// -----------------------------------------------------------------------

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
