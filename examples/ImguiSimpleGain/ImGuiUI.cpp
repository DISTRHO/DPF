/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
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

#include "Application.hpp"

#include <GL/glew.h>

#include "ImGuiUI.hpp"
#include <imgui.h>
#if !defined(IMGUI_GL2) && !defined(IMGUI_GL3)
# define IMGUI_GL2 1
#endif
#if defined(IMGUI_GL2)
# include <imgui_impl_opengl2.h>
#elif defined(IMGUI_GL3)
# include <imgui_impl_opengl3.h>
#endif
#include <chrono>
#include <cmath>

START_NAMESPACE_DGL

struct ImGuiUI::Impl
{
    explicit Impl(ImGuiUI* self);
    ~Impl();

    void setupGL();
    void cleanupGL();

    // perhaps DPF will implement this in the future
    float getScaleFactor() const { return 1.0f; }

    static int mouseButtonToImGui(int button);

    ImGuiUI* fSelf = nullptr;
    ImGuiContext* fContext = nullptr;
    Color fBackgroundColor{0.25f, 0.25f, 0.25f};
    int fRepaintIntervalMs = 15;

    using Clock = std::chrono::steady_clock;
    Clock::time_point fLastRepainted;
    bool fWasEverPainted = false;
};

ImGuiUI::ImGuiUI(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo),
      fImpl(new ImGuiUI::Impl(this))
{
    getApp().addIdleCallback(this);
}

ImGuiUI::~ImGuiUI()
{
    delete fImpl;
}

void ImGuiUI::setBackgroundColor(Color color)
{
    fImpl->fBackgroundColor = color;
}

void ImGuiUI::setRepaintInterval(int intervalMs)
{
    fImpl->fRepaintIntervalMs = intervalMs;
}

void ImGuiUI::onDisplay()
{
    ImGui::SetCurrentContext(fImpl->fContext);

#if defined(IMGUI_GL2)
    ImGui_ImplOpenGL2_NewFrame();
#elif defined(IMGUI_GL3)
    ImGui_ImplOpenGL3_NewFrame();
#endif

    ImGui::NewFrame();
    onImGuiDisplay();
    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();

    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

    Color backgroundColor = fImpl->fBackgroundColor;
    glClearColor(
        backgroundColor.red, backgroundColor.green,
        backgroundColor.blue, backgroundColor.alpha);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

#if defined(IMGUI_GL2)
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#elif defined(IMGUI_GL3)
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

    fImpl->fLastRepainted = Impl::Clock::now();
    fImpl->fWasEverPainted = true;
}

bool ImGuiUI::onKeyboard(const KeyboardEvent& event)
{
    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    if (event.press)
        io.AddInputCharacter(event.key);

    int imGuiKey = event.key;
    if (imGuiKey >= 0 && imGuiKey < 128)
    {
        if (imGuiKey >= 'a' && imGuiKey <= 'z')
            imGuiKey = imGuiKey - 'a' + 'A';
        io.KeysDown[imGuiKey] = event.press;
    }

    return io.WantCaptureKeyboard;
}

bool ImGuiUI::onSpecial(const SpecialEvent& event)
{
    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    int imGuiKey = IM_ARRAYSIZE(io.KeysDown) - event.key;
    io.KeysDown[imGuiKey] = event.press;

    switch (event.key)
    {
    case kKeyShift:
        io.KeyShift = event.press;
        break;
    case kKeyControl:
        io.KeyCtrl = event.press;
        break;
    case kKeyAlt:
        io.KeyAlt = event.press;
        break;
    case kKeySuper:
        io.KeySuper = event.press;
        break;
    default:
        break;
    }

    return io.WantCaptureKeyboard;
}

bool ImGuiUI::onMouse(const MouseEvent& event)
{
    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    int imGuiButton = Impl::mouseButtonToImGui(event.button);
    if (imGuiButton != -1)
        io.MouseDown[imGuiButton] = event.press;

    return io.WantCaptureMouse;
}

bool ImGuiUI::onMotion(const MotionEvent& event)
{
    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    const float scaleFactor = fImpl->getScaleFactor();
    io.MousePos.x = std::round(scaleFactor * event.pos.getX());
    io.MousePos.y = std::round(scaleFactor * event.pos.getY());

    return false;
}

bool ImGuiUI::onScroll(const ScrollEvent& event)
{
    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    io.MouseWheel += event.delta.getY();
    io.MouseWheelH += event.delta.getX();

    return io.WantCaptureMouse;
}

void ImGuiUI::onResize(const ResizeEvent& event)
{
    TopLevelWidget::onResize(event);

    const uint width = event.size.getWidth();
    const uint height = event.size.getHeight();

    ImGui::SetCurrentContext(fImpl->fContext);
    ImGuiIO &io = ImGui::GetIO();

    const float scaleFactor = fImpl->getScaleFactor();
    io.DisplaySize.x = std::round(scaleFactor * width);
    io.DisplaySize.y = std::round(scaleFactor * height);
}

void ImGuiUI::idleCallback()
{
    bool shouldRepaint;

    if (fImpl->fWasEverPainted)
    {
        Impl::Clock::duration elapsed =
            Impl::Clock::now() - fImpl->fLastRepainted;
        std::chrono::milliseconds elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        shouldRepaint = elapsedMs.count() > fImpl->fRepaintIntervalMs;
    }
    else
    {
        shouldRepaint = true;
    }

    if (shouldRepaint)
        repaint();
}

ImGuiUI::Impl::Impl(ImGuiUI* self)
    : fSelf(self)
{
    setupGL();
}

ImGuiUI::Impl::~Impl()
{
    cleanupGL();
}

void ImGuiUI::Impl::setupGL()
{
    DISTRHO_SAFE_ASSERT_RETURN(glewInit() == 0,);

    IMGUI_CHECKVERSION();
    fContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(fContext);

    ImGuiIO &io = ImGui::GetIO();
    const float scaleFactor = getScaleFactor();
    io.DisplaySize.x = std::round(scaleFactor * fSelf->getWidth());
    io.DisplaySize.y = std::round(scaleFactor * fSelf->getHeight());
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    io.KeyMap[ImGuiKey_Tab] = '\t';
    io.KeyMap[ImGuiKey_LeftArrow] = IM_ARRAYSIZE(io.KeysDown) - kKeyLeft;
    io.KeyMap[ImGuiKey_RightArrow] = IM_ARRAYSIZE(io.KeysDown) - kKeyRight;
    io.KeyMap[ImGuiKey_UpArrow] = IM_ARRAYSIZE(io.KeysDown) - kKeyUp;
    io.KeyMap[ImGuiKey_DownArrow] = IM_ARRAYSIZE(io.KeysDown) - kKeyDown;
    io.KeyMap[ImGuiKey_PageUp] = IM_ARRAYSIZE(io.KeysDown) - kKeyPageUp;
    io.KeyMap[ImGuiKey_PageDown] = IM_ARRAYSIZE(io.KeysDown) - kKeyPageDown;
    io.KeyMap[ImGuiKey_Home] = IM_ARRAYSIZE(io.KeysDown) - kKeyHome;
    io.KeyMap[ImGuiKey_End] = IM_ARRAYSIZE(io.KeysDown) - kKeyEnd;
    io.KeyMap[ImGuiKey_Insert] = IM_ARRAYSIZE(io.KeysDown) - kKeyInsert;
    io.KeyMap[ImGuiKey_Delete] = 127;
    io.KeyMap[ImGuiKey_Backspace] = '\b';
    io.KeyMap[ImGuiKey_Space] = ' ';
    io.KeyMap[ImGuiKey_Enter] = '\r';
    io.KeyMap[ImGuiKey_Escape] = 27;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

#if defined(IMGUI_GL2)
    ImGui_ImplOpenGL2_Init();
#elif defined(IMGUI_GL3)
    ImGui_ImplOpenGL3_Init();
#endif
}

void ImGuiUI::Impl::cleanupGL()
{
    ImGui::SetCurrentContext(fContext);
#if defined(IMGUI_GL2)
    ImGui_ImplOpenGL2_Shutdown();
#elif defined(IMGUI_GL3)
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui::DestroyContext(fContext);
}

int ImGuiUI::Impl::mouseButtonToImGui(int button)
{
    switch (button)
    {
    default:
        return -1;
    case 1:
        return 0;
    case 2:
        return 2;
    case 3:
        return 1;
    }
}

END_NAMESPACE_DGL
