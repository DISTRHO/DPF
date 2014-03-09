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
// DGL StandaloneWindow

#include "StandaloneWindow.hpp"

// ------------------------------------------------------
// include all source code here to make building easier

#include "nekobi-ui_src/DistrhoArtworkNekobi.cpp"
#include "nekobi-ui_src/DistrhoUINekobi.cpp"

// ------------------------------------------------------
// main entry point

int main()
{
    DGL::StandaloneWindow appWin;
    DGL::Window& win(appWin.getWindow());

    DistrhoUINekobi gui(win);
    win.setResizable(false);
    win.setSize(gui.getWidth(), gui.getHeight());
    win.setTitle("DGL UI Test");
    win.show();

    DGL::App& app(appWin.getApp());

    while (! app.isQuiting())
    {
        gui.idle();
        app.idle();
        msleep(10);
    }

    return 0;
}

// ------------------------------------------------------
