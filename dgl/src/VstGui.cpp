/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

// #include "../../vstgui/vstgui/standalone/source/platform/gdk/gdkrunloop.h"
#include "../../vstgui/vstgui/vstgui_linux.cpp"
#include "../../vstgui/vstgui/plugin-bindings/plugguieditor.h"

namespace VSTGUI {
    void* soHandle = nullptr;
};

//------------------------------------------------------------------------
class RunLoop : public VSTGUI::X11::IRunLoop
{
public:
	using IEventHandler = VSTGUI::X11::IEventHandler;
	using ITimerHandler = VSTGUI::X11::ITimerHandler;

	static RunLoop& instance ();

	RunLoop ();
	~RunLoop () noexcept;

	bool registerEventHandler (int fd, IEventHandler* handler) override;
	bool unregisterEventHandler (IEventHandler* handler) override;

	bool registerTimer (uint64_t interval, ITimerHandler* handler) override;
	bool unregisterTimer (ITimerHandler* handler) override;

	void forget () override {}
	void remember () override {}

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

static VSTGUI::X11::ITimerHandler* timer1 = nullptr;
static VSTGUI::X11::ITimerHandler* timer2 = nullptr;
static VSTGUI::X11::ITimerHandler* timer3 = nullptr;
static VSTGUI::X11::IEventHandler* event1 = nullptr;
static VSTGUI::X11::IEventHandler* event2 = nullptr;

//------------------------------------------------------------------------
RunLoop& RunLoop::instance ()
{
	static RunLoop instance;
	return instance;
}

//------------------------------------------------------------------------
struct ExternalEventHandler
{
	VSTGUI::X11::IEventHandler* eventHandler{nullptr};
// 	GSource* source{nullptr};
// 	GIOChannel* ioChannel{nullptr};
};

//------------------------------------------------------------------------
struct ExternalTimerHandler
{
	VSTGUI::X11::ITimerHandler* timerHandler{nullptr};
// 	GSource* source{nullptr};
};

//------------------------------------------------------------------------
struct RunLoop::Impl
{
	using EventHandlerVector = std::vector<std::unique_ptr<ExternalEventHandler>>;
	using TimerHandlerVector = std::vector<std::unique_ptr<ExternalTimerHandler>>;

// 	GMainContext * mainContext{nullptr};
	EventHandlerVector eventHandlers;
	TimerHandlerVector timerHandlers;
};

//------------------------------------------------------------------------
RunLoop::RunLoop ()
{
	impl = std::unique_ptr<Impl> (new Impl);
// 	impl->mainContext = g_main_context_ref (g_main_context_default ());
}

//------------------------------------------------------------------------
RunLoop::~RunLoop () noexcept
{
    printf("%s\n", __func__);
}

//------------------------------------------------------------------------
bool RunLoop::registerEventHandler (int fd, IEventHandler* handler)
{
    printf("%s\n", __func__);
    if (event1 == nullptr)
    {
        event1 = handler;
        return true;
    }
    if (event2 == nullptr)
    {
        event2 = handler;
        return true;
    }
	return true;
}

//------------------------------------------------------------------------
bool RunLoop::unregisterEventHandler (IEventHandler* handler)
{
    printf("%s\n", __func__);
	return false;
}

//------------------------------------------------------------------------
bool RunLoop::registerTimer (uint64_t interval, ITimerHandler* handler)
{
    printf("%s\n", __func__);
    if (timer1 == nullptr)
    {
        timer1 = handler;
        return true;
    }
    if (timer2 == nullptr)
    {
        timer2 = handler;
        return true;
    }
    if (timer3 == nullptr)
    {
        timer3 = handler;
        return true;
    }
	return false;
}

//------------------------------------------------------------------------
bool RunLoop::unregisterTimer (ITimerHandler* handler)
{
    printf("%s\n", __func__);
	return false;
}

PluginGUIEditor::PluginGUIEditor (void *pEffect)
	: effect (pEffect)
{
	systemWindow = 0;
	lLastTicks   = getTicks ();
}

//-----------------------------------------------------------------------------
PluginGUIEditor::~PluginGUIEditor ()
{
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::draw (ERect *ppErect)
{
}

//-----------------------------------------------------------------------------
bool PluginGUIEditor::open (void *ptr)
{
	frame = new CFrame (CRect (0, 0, 0, 0), this);
	getFrame ()->setTransparency (true);

	IPlatformFrameConfig* config = nullptr;
	X11::FrameConfig x11config;
	x11config.runLoop = owned (new RunLoop ());
	config = &x11config;

    printf("%s %p %p\n", __func__, frame, ptr);
	getFrame ()->open (ptr, kDefaultNative, config);

	systemWindow = ptr;
	return true;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::idle ()
{
	if (frame)
		frame->idle ();
    if (timer1 != nullptr)
        timer1->onTimer();
    if (timer2 != nullptr)
        timer2->onTimer();
    if (timer3 != nullptr)
        timer3->onTimer();
    if (event1 != nullptr)
        event1->onEvent();
    if (event2 != nullptr)
        event2->onEvent();
}

//-----------------------------------------------------------------------------
int32_t PluginGUIEditor::knobMode = kCircularMode;

//-----------------------------------------------------------------------------
int32_t PluginGUIEditor::setKnobMode (int32_t val)
{
	PluginGUIEditor::knobMode = val;
	return 1;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::wait (uint32_t ms)
{
}

//-----------------------------------------------------------------------------
uint32_t PluginGUIEditor::getTicks ()
{
	return 0;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::doIdleStuff ()
{
}

//-----------------------------------------------------------------------------
bool PluginGUIEditor::getRect (ERect **ppErect)
{
	*ppErect = &rect;
	return true;
}
