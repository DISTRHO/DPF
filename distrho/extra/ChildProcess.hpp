// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

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

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class ChildProcess
{
   #ifdef _WIN32
    PROCESS_INFORMATION pinfo = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
   #else
    pid_t pid = -1;
   #endif

public:
    ChildProcess()
    {
    }

    ~ChildProcess()
    {
        stop();
    }

   #ifdef _WIN32
    bool start(const char* const args[], const WCHAR* const envp)
   #else
    bool start(const char* const args[], char* const* const envp = nullptr)
   #endif
    {
       #ifdef _WIN32
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

       #ifdef _WIN32
        if (pinfo.hProcess == INVALID_HANDLE_VALUE)
            return;

        const PROCESS_INFORMATION opinfo = pinfo;
        pinfo = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

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
       #ifdef _WIN32
        if (pinfo.hProcess == INVALID_HANDLE_VALUE)
            return false;

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode) == FALSE ||
            exitCode != STILL_ACTIVE ||
            WaitForSingleObject(pinfo.hProcess, 0) != WAIT_TIMEOUT)
        {
            const PROCESS_INFORMATION opinfo = pinfo;
            pinfo = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
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

   #ifndef _WIN32
    void signal(const int sig)
    {
        if (pid > 0)
            kill(pid, sig);
    }
   #endif

    void terminate()
    {
       #ifdef _WIN32
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
