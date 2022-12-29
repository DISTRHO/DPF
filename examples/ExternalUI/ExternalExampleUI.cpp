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

// needed for IDE
#include "DistrhoPluginInfo.h"

#include "DistrhoUI.hpp"

#define MPV_TEST
// #define KDE_FIFO_TEST

#ifdef KDE_FIFO_TEST
// Extra includes for current path and fifo stuff
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

START_NAMESPACE_DISTRHO

#ifdef KDE_FIFO_TEST
// TODO: generate a random, not-yet-existing, filename
const char* const kFifoFilename = "/tmp/dpf-fifo-test";

// Helper to get current path of this plugin
static const char* getCurrentPluginFilename()
{
    Dl_info exeInfo;
    void* localSymbol = (void*)kFifoFilename;
    dladdr(localSymbol, &exeInfo);
    return exeInfo.dli_fname;
}

// Helper to check if a file exists
static bool fileExists(const char* const filename)
{
    return access(filename, F_OK) != -1;
}

// Helper function to keep trying to write until it succeeds or really errors out
static ssize_t
writeRetry(int fd, const void* src, size_t size)
{
    ssize_t error;
    int attempts = 0;

    do {
        error = write(fd, src, size);
    } while (error == -1 && (errno == EINTR || errno == EPIPE) && ++attempts < 5);

    return error;
}
#endif

// -----------------------------------------------------------------------------------------------------------

class ExternalExampleUI : public UI
{
public:
    ExternalExampleUI()
        : UI(405, 256),
         #ifdef KDE_FIFO_TEST
          fFifo(-1),
          fExternalScript(getNextBundlePath()),
         #endif
          fValue(0.0f)
    {
       #ifdef KDE_FIFO_TEST
        if (fExternalScript.isEmpty())
        {
            fExternalScript = getCurrentPluginFilename();
            fExternalScript.truncate(fExternalScript.rfind('/'));
        }

        fExternalScript += "/ExternalLauncher.sh";
        d_stdout("External script = %s", fExternalScript.buffer());
       #endif

        if (isVisible() || isEmbed())
            visibilityChanged(true);
    }

    ~ExternalExampleUI()
    {
        if (isEmbed())
            terminateAndWaitForExternalProcess();
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        if (index != 0)
            return;

        fValue = value;

       #ifdef KDE_FIFO_TEST
        if (fFifo == -1)
            return;

        // NOTE: This is a terrible way to pass values, also locale might get in the way...
        char valueStr[24];
        std::memset(valueStr, 0, sizeof(valueStr));
        std::snprintf(valueStr, 23, "%i\n", static_cast<int>(value + 0.5f));

        DISTRHO_SAFE_ASSERT(writeRetry(fFifo, valueStr, 24) == sizeof(valueStr));
       #endif
    }

   /* --------------------------------------------------------------------------------------------------------
    * External Window overrides */

   /**
      Keep-alive.
    */
    void uiIdle() override
    {
       #ifdef KDE_FIFO_TEST
        if (fFifo == -1)
            return;

        writeRetry(fFifo, "idle\n", 5);
       #endif
    }

   /**
      Manage external process and IPC when UI is requested to be visible.
    */
    void visibilityChanged(const bool visible) override
    {
       #ifdef KDE_FIFO_TEST
        if (visible)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fileExists(fExternalScript),);

            mkfifo(kFifoFilename, 0666);
            sync();

            char winIdStr[24];
            std::memset(winIdStr, 0, sizeof(winIdStr));
            std::snprintf(winIdStr, 23, "%lu", getTransientWindowId());

            const char* args[] = {
                fExternalScript.buffer(),
                kFifoFilename,
                "--progressbar", "External UI example",
                "--title", getTitle(),
                nullptr,
            };
            DISTRHO_SAFE_ASSERT_RETURN(startExternalProcess(args),);

            // NOTE: this can lockup the current thread if the other side does not read the file!
            fFifo = open(kFifoFilename, O_WRONLY);
            DISTRHO_SAFE_ASSERT_RETURN(fFifo != -1,);

            parameterChanged(0, fValue);
        }
        else
        {
            if (fFifo != -1)
            {
                if (isRunning())
                {
                    DISTRHO_SAFE_ASSERT(writeRetry(fFifo, "quit\n", 5) == 5);
                    fsync(fFifo);
                }
                ::close(fFifo);
                fFifo = -1;
            }

            unlink(kFifoFilename);
            terminateAndWaitForExternalProcess();
        }
       #endif
       #ifdef MPV_TEST
        if (visible)
        {
            const char* const file = "/home/falktx/Videos/HD/"; // TODO make this a state file?

            if (isEmbed())
            {
                char winIdStr[64];
                snprintf(winIdStr, sizeof(winIdStr), "--wid=%lu", getParentWindowHandle());
                const char* args[] = {
                    "mpv",
                    "--ao=jack",
                    winIdStr,
                    file,
                    nullptr
                };
                unsetenv("LD_LIBRARY_PATH");
                startExternalProcess(args);
            }
            else
            {
                const char* args[] = {
                    "mpv",
                    "--ao=jack",
                    file,
                    nullptr
                };
                startExternalProcess(args);
            }
        }
        else
        {
            terminateAndWaitForExternalProcess();
        }
       #endif
    }

    // -------------------------------------------------------------------------------------------------------

private:
   #ifdef KDE_FIFO_TEST
    // IPC Stuff
    int fFifo;

    // Path to external ui script
    String fExternalScript;
   #endif

    // Current value, cached for when UI becomes visible
    float fValue;

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new ExternalExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
