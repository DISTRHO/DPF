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

#define DPF_TEST_APPLICATION_CPP
#include "dgl/src/pugl.cpp"
#include "dgl/src/Application.cpp"
#include "dgl/src/ApplicationPrivateData.cpp"

#include "distrho/extra/Thread.hpp"

#define DISTRHO_ASSERT_EQUAL(v1, v2, msg) \
    if (v1 != v2) { d_stderr2("Test condition failed: %s; file:%s line:%i", msg, __FILE__, __LINE__); return 1; }

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class ApplicationQuitter : public Thread
{
    Application& app;

public:
    ApplicationQuitter(Application& a)
        : Thread("ApplicationQuitter"),
          app(a)
    {
        startThread();
    }

private:
    void run() override
    {
        d_sleep(5);
        app.quit();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main()
{
    USE_NAMESPACE_DGL;

    // regular usage
    {
        Application app(true);
        DISTRHO_ASSERT_EQUAL(app.isQuiting(), false, "app MUST NOT be set as quitting during init");
        app.idle();
        DISTRHO_ASSERT_EQUAL(app.isQuiting(), false, "app MUST NOT be set as quitting after idle()");
        app.quit();
        DISTRHO_ASSERT_EQUAL(app.isQuiting(), true, "app MUST be set as quitting after quit()");
    }

    // standalone exec, must not block forever due to quit() called from another thread
    {
        Application app(true);
        ApplicationQuitter appQuitter(app);
        app.exec();
        DISTRHO_ASSERT_EQUAL(appQuitter.isThreadRunning(), false, "app quit triggered because we told it so");
    }

    // TODO test idle callback
    // TODO test idle is called when exec(0) is used

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
