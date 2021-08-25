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

#define DPF_TEST_APPLICATION_CPP
#include "dgl/src/pugl.cpp"
#include "dgl/src/Application.cpp"
#include "dgl/src/ApplicationPrivateData.cpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct IdleCallbackCounter : IdleCallback
{
    int counter;

    IdleCallbackCounter()
        : counter(0) {}

    void idleCallback() override
    {
        ++counter;
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
        IdleCallbackCounter idleCounter;
        app.addIdleCallback(&idleCounter);
        DISTRHO_ASSERT_EQUAL(app.isQuitting(), false, "app MUST NOT be set as quitting during init");
        DISTRHO_ASSERT_EQUAL(idleCounter.counter, 0, "app MUST NOT have triggered idle callbacks yet");
        app.idle();
        DISTRHO_ASSERT_EQUAL(app.isQuitting(), false, "app MUST NOT be set as quitting after idle()");
        DISTRHO_ASSERT_EQUAL(idleCounter.counter, 1, "app MUST have triggered 1 idle callback");
        app.idle();
        DISTRHO_ASSERT_EQUAL(idleCounter.counter, 2, "app MUST have triggered 2 idle callbacks");
        app.quit();
        DISTRHO_ASSERT_EQUAL(app.isQuitting(), true, "app MUST be set as quitting after quit()");
        DISTRHO_ASSERT_EQUAL(idleCounter.counter, 2, "app MUST have triggered only 2 idle callbacks in its lifetime");
    }

    // standalone exec, must not block forever due to quit() called from another thread
    {
        Application app(true);
        ApplicationQuitter appQuitter(app);
        IdleCallbackCounter idleCounter;
        app.addIdleCallback(&idleCounter);
        app.exec();
        DISTRHO_ASSERT_EQUAL(appQuitter.isThreadRunning(), false, "app quit triggered because we told it so");
        DISTRHO_ASSERT_NOT_EQUAL(idleCounter.counter, 0, "app idle callbacks MUST have been triggered");
    }

    // standalone exec, but with 0 as timeout
    {
        Application app(true);
        ApplicationQuitter appQuitter(app);
        IdleCallbackCounter idleCounter;
        app.addIdleCallback(&idleCounter);
        app.exec(0);
        DISTRHO_ASSERT_EQUAL(appQuitter.isThreadRunning(), false, "app quit triggered because we told it so");
        DISTRHO_ASSERT_NOT_EQUAL(idleCounter.counter, 0, "app idle callbacks MUST have been triggered");
    }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
