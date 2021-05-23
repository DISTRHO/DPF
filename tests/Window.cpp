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

#include "tests.hpp"

#define DPF_TEST_POINT_CPP
#define DPF_TEST_WINDOW_CPP
#include "dgl/src/pugl.cpp"
#include "dgl/src/Application.cpp"
#include "dgl/src/ApplicationPrivateData.cpp"
#include "dgl/src/Geometry.cpp"
#include "dgl/src/Window.cpp"
#include "dgl/src/WindowPrivateData.cpp"

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    USE_NAMESPACE_DGL;

    using DGL_NAMESPACE::Window;

    // creating and destroying simple window
    {
        Application app(true);
        Window win(app);
    }

    // creating and destroying simple window, with a delay
    {
        Application app(true);
        ApplicationQuitter appQuitter(app);
        Window win(app);
        app.exec();
    }

    // showing and closing simple window, MUST be visible on screen
    {
        Application app(true);
        ApplicationQuitter appQuitter(app);
        Window win(app);
        win.show();
        app.exec();
    }

    // TODO

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
