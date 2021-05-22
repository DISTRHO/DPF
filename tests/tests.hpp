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

#include "dgl/Application.hpp"

#include "distrho/extra/Thread.hpp"

#define DISTRHO_ASSERT_EQUAL(v1, v2, msg) \
    if (v1 != v2) { d_stderr2("Test condition failed: %s; file:%s line:%i", msg, __FILE__, __LINE__); return 1; }

#define DISTRHO_ASSERT_NOT_EQUAL(v1, v2, msg) \
    if (v1 == v2) { d_stderr2("Test condition failed: %s; file:%s line:%i", msg, __FILE__, __LINE__); return 1; }

#define DISTRHO_ASSERT_SAFE_EQUAL(v1, v2, msg) \
    if (d_isNotEqual(v1, v2)) { d_stderr2("Test condition failed: %s; file:%s line:%i", msg, __FILE__, __LINE__); return 1; }

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class ApplicationQuitter : public Thread
{
    Application& app;
    const int numSecondsToWait;

public:
    ApplicationQuitter(Application& a, const int s = 2)
        : Thread("ApplicationQuitter"),
          app(a),
          numSecondsToWait(s)
    {
        startThread();
    }

private:
    void run() override
    {
        d_sleep(numSecondsToWait);
        d_stdout("About to quit now...");
        app.quit();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
