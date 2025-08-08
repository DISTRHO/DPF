/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "Sleep.hpp"
#include "Time.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <string>
# include <winsock2.h>
# include <windows.h>
#else
# include <cerrno>
# include <ctime>
# include <signal.h>
# include <sys/wait.h>
#endif

#if defined(DISTRHO_OS_LINUX)
# include <sys/prctl.h>
#elif defined(DISTRHO_OS_MAC)
# include <dispatch/dispatch.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class ChildProcess
{
   #ifdef DISTRHO_OS_WINDOWS
    PROCESS_INFORMATION pinfo;
   #else
    pid_t pid;
   #endif

   #ifdef DISTRHO_OS_MAC
    static void _proc_exit_handler(void*) { ::kill(::getpid(), SIGTERM); }
   #endif

public:
    ChildProcess()
       #ifdef DISTRHO_OS_WINDOWS
        : pinfo(CPP_AGGREGATE_INIT(PROCESS_INFORMATION){ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 })
       #else
        : pid(-1)
       #endif
    {
    }

    ~ChildProcess()
    {
        stop();
    }

   #ifdef DISTRHO_OS_WINDOWS
    bool start(const char* const args[], const WCHAR* const envp = nullptr)
   #else
    bool start(const char* const args[], char* const* const envp = nullptr)
   #endif
    {
       #ifdef DISTRHO_OS_WINDOWS
        std::string cmd;

        for (uint i = 0; args[i] != nullptr; ++i)
        {
            if (i != 0)
                cmd += " ";

            if (args[i][0] != '"' && std::strchr(args[i], ' ') != nullptr)
            {
                cmd += "\"";
                cmd += args[i];
                cmd += "\"";
            }
            else
            {
                cmd += args[i];
            }
        }

        wchar_t wcmd[PATH_MAX];
        if (MultiByteToWideChar(CP_UTF8, 0, cmd.data(), -1, wcmd, PATH_MAX) <= 0)
            return false;

        STARTUPINFOW si = {};
        si.cb = sizeof(si);

        d_stdout("will start process with args '%s'", cmd.data());

        return CreateProcessW(nullptr,    // lpApplicationName
                              wcmd,       // lpCommandLine
                              nullptr,    // lpProcessAttributes
                              nullptr,    // lpThreadAttributes
                              TRUE,       // bInheritHandles
                              CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
                              const_cast<LPWSTR>(envp), // lpEnvironment
                              nullptr,    // lpCurrentDirectory
                              &si,        // lpStartupInfo
                              &pinfo) != FALSE;
       #else
        const pid_t ret = pid = vfork();

        switch (ret)
        {
        // child process
        case 0:
           #if defined(DISTRHO_OS_LINUX)
            ::prctl(PR_SET_PDEATHSIG, SIGTERM);
           #elif defined(DISTRHO_OS_MAC)
            if (const dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC,
                                                                        ::getppid(),
                                                                        DISPATCH_PROC_EXIT,
                                                                        nullptr))
            {
                dispatch_source_set_event_handler_f(source, _proc_exit_handler);
                dispatch_resume(source);
            }
           #endif

            if (envp != nullptr)
                execve(args[0], const_cast<char* const*>(args), envp);
            else
                execvp(args[0], const_cast<char* const*>(args));

            d_stderr2("exec failed: %d:%s", errno, std::strerror(errno));
            _exit(1);
            break;

        // error
        case -1:
            d_stderr2("vfork() failed: %d:%s", errno, std::strerror(errno));
            break;
        }

        return ret > 0;
       #endif
    }

    void stop(const uint32_t timeoutInMilliseconds = 2000)
    {
        const uint32_t timeout = d_gettime_ms() + timeoutInMilliseconds;
        bool sendTerminate = true;

       #ifdef DISTRHO_OS_WINDOWS
        if (pinfo.hProcess == INVALID_HANDLE_VALUE)
            return;

        const PROCESS_INFORMATION opinfo = pinfo;
        pinfo = (PROCESS_INFORMATION){ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

        for (DWORD exitCode;;)
        {
            if (GetExitCodeProcess(opinfo.hProcess, &exitCode) == FALSE ||
                exitCode != STILL_ACTIVE ||
                WaitForSingleObject(opinfo.hProcess, 0) != WAIT_TIMEOUT)
            {
                CloseHandle(opinfo.hThread);
                CloseHandle(opinfo.hProcess);
                return;
            }

            if (sendTerminate)
            {
                sendTerminate = false;
                TerminateProcess(opinfo.hProcess, ERROR_BROKEN_PIPE);
            }

            if (d_gettime_ms() < timeout)
            {
                d_msleep(5);
                continue;
            }
            d_stderr("ChildProcess::stop() - timed out");
            TerminateProcess(opinfo.hProcess, 9);
            d_msleep(5);
            CloseHandle(opinfo.hThread);
            CloseHandle(opinfo.hProcess);
            break;
        }
       #else
        if (pid <= 0)
            return;

        const pid_t opid = pid;
        pid = -1;

        for (pid_t ret;;)
        {
            try {
                ret = ::waitpid(opid, nullptr, WNOHANG);
            } DISTRHO_SAFE_EXCEPTION_BREAK("waitpid");

            switch (ret)
            {
            case -1:
                if (errno == ECHILD)
                {
                    // success, child doesn't exist
                    return;
                }
                else
                {
                    d_stderr("ChildProcess::stop() - waitpid failed: %d:%s", errno, std::strerror(errno));
                    return;
                }
                break;

            case 0:
                if (sendTerminate)
                {
                    sendTerminate = false;
                    kill(opid, SIGTERM);
                }
                if (d_gettime_ms() < timeout)
                {
                    d_msleep(5);
                    continue;
                }

                d_stderr("ChildProcess::stop() - timed out");
                kill(opid, SIGKILL);
                waitpid(opid, nullptr, WNOHANG);
                break;

            default:
                if (ret == opid)
                {
                    // success
                    return;
                }
                else
                {
                    d_stderr("ChildProcess::stop() - got wrong pid %i (requested was %i)", int(ret), int(opid));
                    return;
                }
            }

            break;
        }
       #endif
    }

    bool isRunning()
    {
       #ifdef DISTRHO_OS_WINDOWS
        if (pinfo.hProcess == INVALID_HANDLE_VALUE)
            return false;

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode) == FALSE ||
            exitCode != STILL_ACTIVE ||
            WaitForSingleObject(pinfo.hProcess, 0) != WAIT_TIMEOUT)
        {
            const PROCESS_INFORMATION opinfo = pinfo;
            pinfo = (PROCESS_INFORMATION){ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
            CloseHandle(opinfo.hThread);
            CloseHandle(opinfo.hProcess);
            return false;
        }

        return true;
       #else
        if (pid <= 0)
            return false;

        const pid_t ret = ::waitpid(pid, nullptr, WNOHANG);

        if (ret == pid || (ret == -1 && errno == ECHILD))
        {
            pid = 0;
            return false;
        }

        return true;
       #endif
    }

   #ifndef DISTRHO_OS_WINDOWS
    void signal(const int sig)
    {
        if (pid > 0)
            kill(pid, sig);
    }
   #endif

    void terminate()
    {
       #ifdef DISTRHO_OS_WINDOWS
        if (pinfo.hProcess != INVALID_HANDLE_VALUE)
            TerminateProcess(pinfo.hProcess, 15);
       #else
        if (pid > 0)
            kill(pid, SIGTERM);
       #endif
    }

    DISTRHO_DECLARE_NON_COPYABLE(ChildProcess)
};

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
