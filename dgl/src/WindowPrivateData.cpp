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
#include "TopLevelWidgetPrivateData.hpp"

#include "pugl.hpp"

#define DGL_DEBUG_EVENTS

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
# include <cinttypes>
#endif

START_NAMESPACE_DGL

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
# define DGL_DBG(msg)  std::fprintf(stderr, "%s", msg);
# define DGL_DBGp(...) std::fprintf(stderr, __VA_ARGS__);
# define DGL_DBGF      std::fflush(stderr);
#else
# define DGL_DBG(msg)
# define DGL_DBGp(...)
# define DGL_DBGF
#endif

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

// -----------------------------------------------------------------------

static double getDesktopScaleFactor()
{
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

    return 1.0;
}

// -----------------------------------------------------------------------

Window::PrivateData::PrivateData(Application& a, Window* const s)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      scaleFactor(getDesktopScaleFactor()),
      autoScaling(false),
      autoScaleFactor(1.0),
      minWidth(0),
      minHeight(0),
      modal()
{
    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
}

Window::PrivateData::PrivateData(Application& a, Window* const s, PrivateData* const ppData)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      scaleFactor(ppData->scaleFactor),
      autoScaling(false),
      autoScaleFactor(1.0),
      minWidth(0),
      minHeight(0),
      modal(ppData)
{
    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);

    puglSetTransientFor(view, puglGetNativeWindow(ppData->view));
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const double scale, const bool resizable)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0),
      isEmbed(parentWindowHandle != 0),
      scaleFactor(scale != 0.0 ? scale : getDesktopScaleFactor()),
      autoScaling(false),
      autoScaleFactor(1.0),
      minWidth(0),
      minHeight(0),
      modal()
{
    if (isEmbed)
    {
        // puglSetDefaultSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, height);
        puglSetParentWindow(view, parentWindowHandle);
    }

    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, resizable);

    if (isEmbed)
    {
        appData->oneWindowShown();
        puglShow(view);
    }
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const uint width, const uint height,
                                 const double scale, const bool resizable)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0),
      isEmbed(parentWindowHandle != 0),
      scaleFactor(scale != 0.0 ? scale : getDesktopScaleFactor()),
      autoScaling(false),
      autoScaleFactor(1.0),
      minWidth(0),
      minHeight(0),
      modal()
{
    if (isEmbed)
    {
        puglSetDefaultSize(view, static_cast<int>(width), static_cast<int>(height));
        puglSetParentWindow(view, parentWindowHandle);
    }

    init(width, height, resizable);

    if (isEmbed)
    {
        appData->oneWindowShown();
        puglShow(view);
    }
}

Window::PrivateData::~PrivateData()
{
    if (isEmbed)
    {
        puglHide(view);
        appData->oneWindowClosed();
        isClosed = true;
        isVisible = false;
    }

    appData->idleCallbacks.remove(this);
    appData->windows.remove(self);

    if (view != nullptr)
        puglFreeView(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::init(const uint width, const uint height, const bool resizable)
{
    appData->windows.push_back(self);
    appData->idleCallbacks.push_back(this);
    memset(graphicsContext, 0, sizeof(graphicsContext));

    if (view == nullptr)
    {
        DGL_DBG("Failed to create Pugl view, everything will fail!\n");
        return;
    }

    puglSetMatchingBackendForCurrentBuild(view);

    puglSetHandle(view, this);
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
    puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
    puglSetViewHint(view, PUGL_DEPTH_BITS, 16);
    puglSetViewHint(view, PUGL_STENCIL_BITS, 8);
    // PUGL_SAMPLES ??
    puglSetEventFunc(view, puglEventCallback);
// #ifndef DGL_FILE_BROWSER_DISABLED
//     puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
// #endif

    PuglRect rect = puglGetFrame(view);
    rect.width = width;
    rect.height = height;
    puglSetFrame(view, rect);

    // FIXME this is bad
    puglRealize(view);
    puglBackendEnter(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::show()
{
    if (isVisible)
    {
        DGL_DBG("Window show matches current visible state, ignoring request\n");
        return;
    }
    if (isEmbed)
    {
        DGL_DBG("Window show cannot be called when embedded\n");
        return;
    }

    DGL_DBG("Window show called\n");

#if 0 && defined(DISTRHO_OS_MAC)
//     if (mWindow != nullptr)
//     {
//         if (mParentWindow != nullptr)
//             [mParentWindow addChildWindow:mWindow
//                                   ordered:NSWindowAbove];
//     }
#endif

    if (isClosed)
    {
        isClosed = false;
        appData->oneWindowShown();

        // FIXME
        PuglRect rect = puglGetFrame(view);
        puglSetDefaultSize(view, static_cast<int>(rect.width), static_cast<int>(rect.height));
        puglSetWindowSize(view, static_cast<uint>(rect.width), static_cast<uint>(rect.height));

#ifdef DISTRHO_OS_WINDOWS
        puglWin32ShowWindowCentered(view);
#else
        puglShow(view);
#endif
    }
    else
    {
#ifdef DISTRHO_OS_WINDOWS
        puglWin32RestoreWindow(view);
#else
        puglShow(view);
#endif
    }

    isVisible = true;
}

void Window::PrivateData::hide()
{
    if (isEmbed)
    {
        DGL_DBG("Window hide cannot be called when embedded\n");
        return;
    }
    if (! isVisible)
    {
        DGL_DBG("Window hide matches current visible state, ignoring request\n");
        return;
    }

    DGL_DBG("Window hide called\n");

#if 0 && defined(DISTRHO_OS_MAC)
//     if (mWindow != nullptr)
//     {
//         if (mParentWindow != nullptr)
//             [mParentWindow removeChildWindow:mWindow];
//     }
#endif

    if (modal.enabled)
        stopModal();

    puglHide(view);

    isVisible = false;
}

// -----------------------------------------------------------------------

void Window::PrivateData::close()
{
    DGL_DBG("Window close\n");
    // DGL_DBGp("Window close DBG %i %i %p\n", isEmbed, isClosed, appData);

    if (isEmbed || isClosed)
        return;

    isClosed = true;
    hide();
    appData->oneWindowClosed();
}

// -----------------------------------------------------------------------

void Window::PrivateData::focus()
{
    if (! isEmbed)
        puglRaiseWindow(view);

    puglGrabFocus(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::setResizable(const bool resizable)
{
    DISTRHO_SAFE_ASSERT_RETURN(! isEmbed,);

    DGL_DBG("Window setResizable called\n");

    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
#ifdef DISTRHO_OS_WINDOWS
    puglWin32SetWindowResizable(view, resizable);
#endif
}

// -----------------------------------------------------------------------

void Window::PrivateData::idleCallback()
{
// #if defined(DISTRHO_OS_WINDOWS) && !defined(DGL_FILE_BROWSER_DISABLED)
//     if (fSelectedFile.isNotEmpty())
//     {
//         char* const buffer = fSelectedFile.getAndReleaseBuffer();
//         fView->fileSelectedFunc(fView, buffer);
//         std::free(buffer);
//     }
// #endif

//     if (modal.enabled && modal.parent != nullptr)
//         modal.parent->idleCallback();
}

// -----------------------------------------------------------------------

bool Window::PrivateData::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    if (timerFrequencyInMs == 0)
    {
        appData->idleCallbacks.push_back(callback);
        return true;
    }

    return puglStartTimer(view, (uintptr_t)callback, static_cast<double>(timerFrequencyInMs) / 1000.0) == PUGL_SUCCESS;
}

bool Window::PrivateData::removeIdleCallback(IdleCallback* const callback)
{
    if (std::find(appData->idleCallbacks.begin(),
                  appData->idleCallbacks.end(), callback) != appData->idleCallbacks.end())
    {
        appData->idleCallbacks.remove(callback);
        return true;
    }

    return puglStopTimer(view, (uintptr_t)callback) == PUGL_SUCCESS;
}

// -----------------------------------------------------------------------
// modal handling

void Window::PrivateData::startModal()
{
    DGL_DBG("Window modal loop starting..."); DGL_DBGF;
    DISTRHO_SAFE_ASSERT_RETURN(modal.parent != nullptr, show());

    // activate modal mode for this window
    modal.enabled = true;

    // make parent give focus to us
    modal.parent->modal.child = this;

    // FIXME?
    PuglRect rect = puglGetFrame(view);
    puglSetDefaultSize(view, static_cast<int>(rect.width), static_cast<int>(rect.height));

    // make sure both parent and ourselves are visible
    modal.parent->show();
    show();

    DGL_DBG("Ok\n");
}

void Window::PrivateData::stopModal()
{
    DGL_DBG("Window modal loop stopping..."); DGL_DBGF;

    // deactivate modal mode
    modal.enabled = false;

    // safety checks, make sure we have a parent and we are currently active as the child to give focus to
    if (modal.parent == nullptr)
        return;
    if (modal.parent->modal.child != this)
        return;

    // stop parent from giving focus to us, so it behaves like normal
    modal.parent->modal.child = nullptr;

    // the mouse position probably changed since the modal appeared,
    // so send a mouse motion event to the modal's parent window
#if 0
#if defined(DISTRHO_OS_HAIKU)
    // TODO
#elif defined(DISTRHO_OS_MAC)
    // TODO
#elif defined(DISTRHO_OS_WINDOWS)
    // TODO
#else
    int i, wx, wy;
    uint u;
    ::Window w;
    if (XQueryPointer(fModal.parent->xDisplay, fModal.parent->xWindow, &w, &w, &i, &i, &wx, &wy, &u) == True)
        fModal.parent->onPuglMotion(wx, wy);
#endif
#endif

    DGL_DBG("Ok\n");
}

void Window::PrivateData::runAsModal(const bool blockWait)
{
    DGL_DBGp("Window::PrivateData::runAsModal %i\n", blockWait);
    startModal();

    if (blockWait)
    {
        DISTRHO_SAFE_ASSERT_RETURN(appData->isStandalone,);

        while (isVisible && modal.enabled)
            appData->idle(10);

        stopModal();
    }
    else
    {
        appData->idle(0);
    }
}

// -----------------------------------------------------------------------
// pugl events

void Window::PrivateData::onPuglConfigure(const double width, const double height)
{
    DISTRHO_SAFE_ASSERT_INT2_RETURN(width > 1 && height > 1, width, height,);

    DGL_DBGp("PUGL: onReshape : %i %i\n", width, height);

    if (autoScaling)
    {
        const double scaleHorizontal = width  / static_cast<double>(minWidth);
        const double scaleVertical   = height / static_cast<double>(minHeight);
        autoScaleFactor = scaleHorizontal < scaleVertical ? scaleHorizontal : scaleVertical;
    }

    const uint uwidth = static_cast<uint>(width + 0.5);
    const uint uheight = static_cast<uint>(height + 0.5);
    self->onReshape(uwidth, uheight);

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->setSize(uwidth, uheight);
#endif

    // always repaint after a resize
    puglPostRedisplay(view);
}

void Window::PrivateData::onPuglExpose()
{
    DGL_DBGp("PUGL: onPuglExpose : %p\n", topLevelWidget);

    puglOnDisplayPrepare(view);

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->display();
#endif
}

void Window::PrivateData::onPuglClose()
{
    DGL_DBG("PUGL: onClose\n");

    // if we have a parent or running as standalone we can prevent closing in certain conditions
    if (modal.parent != nullptr || appData->isStandalone)
    {
        // parent gives focus to us as modal, prevent closing
        if (modal.child != nullptr)
            return modal.child->focus();

        // ask window if we should close
        if (! self->onClose())
            return;
    }

    if (modal.enabled)
        stopModal();

    if (PrivateData* const child = modal.child)
    {
        modal.child = nullptr;
        child->close();
    }

    close();
}

void Window::PrivateData::onPuglFocus(const bool focus, const CrossingMode mode)
{
    DGL_DBGp("onPuglFocus : %i %i\n", focus, mode);

    if (isClosed)
        return;

    if (modal.child != nullptr)
        return modal.child->focus();

    self->onFocus(focus, mode);
}

void Window::PrivateData::onPuglKey(const Widget::KeyboardEvent& ev)
{
    DGL_DBGp("onPuglKey : %i %u %u\n", ev.press, ev.key, ev.keycode);

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->keyboardEvent(ev);
#endif
}

void Window::PrivateData::onPuglSpecial(const Widget::SpecialEvent& ev)
{
    DGL_DBGp("onPuglSpecial : %i %u\n", ev.press, ev.key);

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->specialEvent(ev);
#endif
}

void Window::PrivateData::onPuglText(const Widget::CharacterInputEvent& ev)
{
    DGL_DBGp("onPuglText : %u %u %s\n", ev.keycode, ev.character, ev.string);

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->characterInputEvent(ev);
#endif
}

void Window::PrivateData::onPuglMouse(const Widget::MouseEvent& ev)
{
    DGL_DBGp("onPuglMouse : %i %i %f %f\n", ev.button, ev.press, ev.pos.getX(), ev.pos.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->mouseEvent(ev);
#endif
}

void Window::PrivateData::onPuglMotion(const Widget::MotionEvent& ev)
{
    DGL_DBGp("onPuglMotion : %f %f\n", ev.pos.getX(), ev.pos.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->motionEvent(ev);
#endif
}

void Window::PrivateData::onPuglScroll(const Widget::ScrollEvent& ev)
{
    DGL_DBGp("onPuglScroll : %f %f %f %f\n", ev.pos.getX(), ev.pos.getY(), ev.delta.getX(), ev.delta.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->scrollEvent(ev);
#endif
}

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose);
#endif

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
    Window::PrivateData* const pData = (Window::PrivateData*)puglGetHandle(view);
#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
    printEvent(event, "pugl event: ", true);
#endif

    switch (event->type)
    {
    ///< No event
    case PUGL_NOTHING:
        break;

    ///< View created, a #PuglEventCreate
    case PUGL_CREATE:
#ifdef DGL_PUGL_USING_X11
        if (! pData->isEmbed)
            puglExtraSetWindowTypeAndPID(view);
#endif
        break;

    ///< View destroyed, a #PuglEventDestroy
    case PUGL_DESTROY:
        break;

    ///< View moved/resized, a #PuglEventConfigure
    case PUGL_CONFIGURE:
        // unused x, y (double)
        pData->onPuglConfigure(event->configure.width, event->configure.height);
        break;

    ///< View made visible, a #PuglEventMap
    case PUGL_MAP:
        break;

    ///< View made invisible, a #PuglEventUnmap
    case PUGL_UNMAP:
        break;

    ///< View ready to draw, a #PuglEventUpdate
    case PUGL_UPDATE:
        break;

    ///< View must be drawn, a #PuglEventExpose
    case PUGL_EXPOSE:
        // unused x, y, width, height (double)
        pData->onPuglExpose();
        break;

    ///< View will be closed, a #PuglEventClose
    case PUGL_CLOSE:
        pData->onPuglClose();
        break;

    ///< Keyboard focus entered view, a #PuglEventFocus
    case PUGL_FOCUS_IN:
    ///< Keyboard focus left view, a #PuglEventFocus
    case PUGL_FOCUS_OUT:
        pData->onPuglFocus(event->type == PUGL_FOCUS_IN,
                           static_cast<CrossingMode>(event->focus.mode));
        break;

    ///< Key pressed, a #PuglEventKey
    case PUGL_KEY_PRESS:
    ///< Key released, a #PuglEventKey
    case PUGL_KEY_RELEASE:
    {
        // unused x, y, xRoot, yRoot (double)
        // TODO special keys?
        Widget::KeyboardEvent ev;
        ev.mod     = event->key.state;
        ev.flags   = event->key.flags;
        ev.time    = static_cast<uint>(event->key.time * 1000.0 + 0.5);
        ev.press   = event->type == PUGL_KEY_PRESS;
        ev.key     = event->key.key;
        ev.keycode = event->key.keycode;
        if ((ev.mod & kModifierShift) != 0 && ev.key >= 'a' && ev.key <= 'z')
            ev.key -= 'a' - 'A'; // a-z -> A-Z
        pData->onPuglKey(ev);
        break;
    }

    ///< Character entered, a #PuglEventText
    case PUGL_TEXT:
    {
        // unused x, y, xRoot, yRoot (double)
        Widget::CharacterInputEvent ev;
        ev.mod       = event->text.state;
        ev.flags     = event->text.flags;
        ev.time      = static_cast<uint>(event->text.time * 1000.0 + 0.5);
        ev.keycode   = event->text.keycode;
        ev.character = event->text.character;
        std::strncpy(ev.string, event->text.string, sizeof(ev.string));
        pData->onPuglText(ev);
        break;
    }

    ///< Pointer entered view, a #PuglEventCrossing
    case PUGL_POINTER_IN:
        break;
    ///< Pointer left view, a #PuglEventCrossing
    case PUGL_POINTER_OUT:
        break;

    ///< Mouse button pressed, a #PuglEventButton
    case PUGL_BUTTON_PRESS:
    ///< Mouse button released, a #PuglEventButton
    case PUGL_BUTTON_RELEASE:
    {
        Widget::MouseEvent ev;
        ev.mod    = event->button.state;
        ev.flags  = event->button.flags;
        ev.time   = static_cast<uint>(event->button.time * 1000.0 + 0.5);
        ev.button = event->button.button;
        ev.press  = event->type == PUGL_BUTTON_PRESS;
        ev.pos    = Point<double>(event->button.x, event->button.y);
        pData->onPuglMouse(ev);
        break;
    }

    ///< Pointer moved, a #PuglEventMotion
    case PUGL_MOTION:
    {
        Widget::MotionEvent ev;
        ev.mod   = event->motion.state;
        ev.flags = event->motion.flags;
        ev.time  = static_cast<uint>(event->motion.time * 1000.0 + 0.5);
        ev.pos   = Point<double>(event->motion.x, event->motion.y);
        pData->onPuglMotion(ev);
        break;
    }

    ///< Scrolled, a #PuglEventScroll
    case PUGL_SCROLL:
    {
        Widget::ScrollEvent ev;
        ev.mod       = event->scroll.state;
        ev.flags     = event->scroll.flags;
        ev.time      = static_cast<uint>(event->scroll.time * 1000.0 + 0.5);
        ev.pos       = Point<double>(event->scroll.x, event->scroll.y);
        ev.delta     = Point<double>(event->scroll.dx, event->scroll.dy);
        ev.direction = static_cast<ScrollDirection>(event->scroll.direction);
        pData->onPuglScroll(ev);
        break;
    }

    ///< Custom client message, a #PuglEventClient
    case PUGL_CLIENT:
        break;

    ///< Timer triggered, a #PuglEventTimer
    case PUGL_TIMER:
        if (IdleCallback* const idleCallback = reinterpret_cast<IdleCallback*>(event->timer.id))
            idleCallback->idleCallback();
        break;

    ///< Recursive loop entered, a #PuglEventLoopEnter
    case PUGL_LOOP_ENTER:
        break;

    ///< Recursive loop left, a #PuglEventLoopLeave
    case PUGL_LOOP_LEAVE:
        break;
    }

    return PUGL_SUCCESS;
}

// -----------------------------------------------------------------------

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
static int printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define FFMT            "%6.1f"
#define PFMT            FFMT " " FFMT
#define PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

	switch (event->type) {
	case PUGL_NOTHING:
		return 0;
	case PUGL_KEY_PRESS:
		return PRINT("%sKey press   code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_KEY_RELEASE:
		return PRINT("%sKey release code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_TEXT:
		return PRINT("%sText entry  code %3u char U+%04X (%s)\n",
		             prefix,
		             event->text.keycode,
		             event->text.character,
		             event->text.string);
	case PUGL_BUTTON_PRESS:
	case PUGL_BUTTON_RELEASE:
		return (PRINT("%sMouse %u %s at " PFMT " ",
		              prefix,
		              event->button.button,
		              (event->type == PUGL_BUTTON_PRESS) ? "down" : "up  ",
		              event->button.x,
		              event->button.y) +
		        printModifiers(event->scroll.state));
	case PUGL_SCROLL:
		return (PRINT("%sScroll %5.1f %5.1f at " PFMT " ",
		              prefix,
		              event->scroll.dx,
		              event->scroll.dy,
		              event->scroll.x,
		              event->scroll.y) +
		        printModifiers(event->scroll.state));
	case PUGL_POINTER_IN:
		return PRINT("%sMouse enter  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_POINTER_OUT:
		return PRINT("%sMouse leave  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_FOCUS_IN:
		return PRINT("%sFocus in %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_FOCUS_OUT:
		return PRINT("%sFocus out %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_CLIENT:
		return PRINT("%sClient %" PRIXPTR " %" PRIXPTR "\n",
		             prefix,
		             event->client.data1,
		             event->client.data2);
	case PUGL_TIMER:
		return PRINT("%sTimer %" PRIuPTR "\n", prefix, event->timer.id);
	default:
		break;
	}

	if (verbose) {
		switch (event->type) {
		case PUGL_CREATE:
			return fprintf(stderr, "%sCreate\n", prefix);
		case PUGL_DESTROY:
			return fprintf(stderr, "%sDestroy\n", prefix);
		case PUGL_MAP:
			return fprintf(stderr, "%sMap\n", prefix);
		case PUGL_UNMAP:
			return fprintf(stderr, "%sUnmap\n", prefix);
		case PUGL_UPDATE:
			return 0; // fprintf(stderr, "%sUpdate\n", prefix);
		case PUGL_CONFIGURE:
			return PRINT("%sConfigure " PFMT " " PFMT "\n",
			             prefix,
			             event->configure.x,
			             event->configure.y,
			             event->configure.width,
			             event->configure.height);
		case PUGL_EXPOSE:
			return PRINT("%sExpose    " PFMT " " PFMT "\n",
			             prefix,
			             event->expose.x,
			             event->expose.y,
			             event->expose.width,
			             event->expose.height);
		case PUGL_CLOSE:
			return PRINT("%sClose\n", prefix);
		case PUGL_MOTION:
			return PRINT("%sMouse motion at " PFMT "\n",
			             prefix,
			             event->motion.x,
			             event->motion.y);
		default:
			return PRINT("%sUnknown event type %d\n", prefix, (int)event->type);
		}
	}

#undef PRINT
#undef PFMT
#undef FFMT

	return 0;
}
#endif

#undef DGL_DBG
#undef DGL_DBGF

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
