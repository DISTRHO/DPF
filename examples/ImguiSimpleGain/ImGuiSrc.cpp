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

#include <imgui.h>
#if !defined(IMGUI_GL2) && !defined(IMGUI_GL3)
# define IMGUI_GL2 1
#endif
#if defined(IMGUI_GL2)
# include <imgui_impl_opengl2.h>
#elif defined(IMGUI_GL3)
# include <imgui_impl_opengl3.h>
#endif

#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_tables.cpp>
#include <imgui_widgets.cpp>
#if defined(IMGUI_GL2)
#include <imgui_impl_opengl2.cpp>
#elif defined(IMGUI_GL3)
#include <imgui_impl_opengl3.cpp>
#endif
