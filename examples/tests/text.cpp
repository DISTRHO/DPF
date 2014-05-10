/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

// ------------------------------------------------------
// DGL Stuff

#define GL_GLEXT_PROTOTYPES

#include "App.hpp"
#include "Window.hpp"
#include "Widget.hpp"

#include "src/freetype-gl/text-buffer.h"
#include "src/freetype-gl/mat4.h"

#include <GL/glext.h>

#include <cstdio>
#include <cwchar>

// ------------------------------------------------------
extern "C" {

int z_verbose = 0;

void z_error (char* message)
{
    d_stderr2(message);
}

}

// ------------------------------------------------------
// use namespace

using namespace DGL;

// ------------------------------------------------------
// Single color widget

class TextWidget : public Widget
{
public:
    TextWidget(Window& parent)
        : Widget(parent),
#if 1
          atlas(nullptr),
          fontmgr(nullptr),
          font(nullptr),
#endif
          textbuf(nullptr)
    {
        vec2 pen = {{20, 200}};

        vec4 black = {{0.0, 0.0, 0.0, 1.0}};
        vec4 white = {{1.0, 1.0, 1.0, 1.0}};
        vec4 none  = {{1.0, 1.0, 1.0, 0.0}};

        markup_t markup = {
            "normal",
            24.0f, 0, 0,
            0.0, 0.0, 2.0f,
            white, none,
            0, white,
            0, white,
            0, white,
            0, white,
            0
        };

        wchar_t* text = L"A Quick Brown Fox Jumps Over The Lazy Dog 0123456789";

#if 1
        atlas = texture_atlas_new(600, 300, 1);
        DISTRHO_SAFE_ASSERT_RETURN(atlas != nullptr,);

        //fontmgr = font_manager_new(600, 200, 2);
        //DISTRHO_SAFE_ASSERT_RETURN(fontmgr != nullptr,);

        //font = font_manager_get_from_filename(fontmgr, "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf", 12.0f);
        //DISTRHO_SAFE_ASSERT_RETURN(font != nullptr,);

        font = texture_font_new_from_file(atlas, 12.0f, "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf");
        DISTRHO_SAFE_ASSERT_RETURN(font != nullptr,);
#endif

        textbuf = text_buffer_new(LCD_FILTERING_OFF);
        DISTRHO_SAFE_ASSERT_RETURN(textbuf != nullptr,);

        textbuf->base_color = black;

        //markup.family = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf";
        markup.font = font;

        text_buffer_printf(textbuf, &pen,
                           &markup, text, nullptr);

        text_buffer_add_text(textbuf, &pen, &markup, text, std::wcslen(text));

        mat4_set_identity(&projection);
        mat4_set_identity(&model);
        mat4_set_identity(&view);
    }

    ~TextWidget()
    {
        if (textbuf != nullptr)
        {
            text_buffer_delete(textbuf);
            textbuf = nullptr;
        }

#if 1
        if (font != nullptr)
        {
            texture_font_delete(font);
            font = nullptr;
        }

        if (fontmgr != nullptr)
        {
            font_manager_delete(fontmgr);
            fontmgr = nullptr;
        }

        if (atlas != nullptr)
        {
            texture_atlas_delete(atlas);
            atlas = nullptr;
        }
#endif
    }

private:
    void onDisplay() override
    {
        DISTRHO_SAFE_ASSERT_RETURN(textbuf != nullptr,);

        glClearColor(0.4f, 0.4f, 0.45f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glUseProgram(textbuf->shader);
        {
            glUniformMatrix4fv(glGetUniformLocation(textbuf->shader, "model"), 1, 0, model.data);
            glUniformMatrix4fv(glGetUniformLocation(textbuf->shader, "view"), 1, 0, view.data);
            glUniformMatrix4fv(glGetUniformLocation(textbuf->shader, "projection"), 1, 0, projection.data);

            text_buffer_render(textbuf);
        }
    }

    void onReshape(int width, int height) override
    {
        // make widget same size as window
        setSize(width, height);
        //Widget::onReshape(width, height);

        //mat4_set_identity(&projection);
        //mat4_set_identity(&model);
        //mat4_set_identity(&view);

        glViewport(0, 0, width, height);
        //mat4_set_orthographic(&projection, 0, width, 0, height, width, height);
        mat4_set_orthographic(&projection, 0, width, 0, height, -1, 1);
    }

    texture_atlas_t* atlas;
    font_manager_t* fontmgr;
    texture_font_t* font;

    text_buffer_t* textbuf;

    mat4 model, view, projection;
};

// ------------------------------------------------------
// main entry point

int main()
{
    App app;
    Window win(app);
    TextWidget color(win);

    win.setSize(600, 300);
    win.setTitle("Text");
    win.show();
    app.exec();

    return 0;
}

// ------------------------------------------------------
