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

#ifndef DGL_IMAGE_WIDGETS_HPP_INCLUDED
#define DGL_IMAGE_WIDGETS_HPP_INCLUDED

#include "Image.hpp"

START_NAMESPACE_DGL

DISTRHO_DEPRECATED_BY("OpenGLImageAboutWindow")
typedef OpenGLImageAboutWindow ImageAboutWindow;

DISTRHO_DEPRECATED_BY("OpenGLImageButton")
typedef OpenGLImageButton ImageButton;

DISTRHO_DEPRECATED_BY("OpenGLImageKnob")
typedef OpenGLImageKnob ImageKnob;

DISTRHO_DEPRECATED_BY("OpenGLImageSlider")
typedef OpenGLImageSlider ImageSlider;

DISTRHO_DEPRECATED_BY("OpenGLImageSwitch")
typedef OpenGLImageSwitch ImageSwitch;

END_NAMESPACE_DGL

#endif // DGL_IMAGE_WIDGETS_HPP_INCLUDED
