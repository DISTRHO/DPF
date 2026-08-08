/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_WINDOWS_INCLUDE_H_INCLUDED
#define DISTRHO_WINDOWS_INCLUDE_H_INCLUDED

#ifndef _WIN32
# error Wrong include
#endif

// Set minimum target version to Windows 2000
#if !(defined(WINVER) || defined(_WIN32_WINNT))
# define WINVER 0x0500
# define _WIN32_WINNT 0x0500
#endif

// Disable as many things from windows.h as possible
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

// CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#ifndef NOGDICAPMASKS
# define NOGDICAPMASKS
#endif

//  SM_*
#ifndef NOSYSMETRICS
# define NOSYSMETRICS
#endif

// MK_*
#ifndef NOKEYSTATES
# define NOKEYSTATES
#endif

// OEM Resource values
#ifndef OEMRESOURCE
# define OEMRESOURCE
#endif

// Atom Manager routines
#ifndef NOATOM
# define NOATOM
#endif

// Screen colors
#ifndef NOCOLOR
# define NOCOLOR
#endif

// DrawText() and DT_*
#ifndef NODRAWTEXT
# define NODRAWTEXT
#endif

// All KERNEL defines and routines
#ifndef NOKERNEL
# define NOKERNEL
#endif

// MB_* and MessageBox()
#ifndef NOMB
# define NOMB
#endif

// GMEM_*, LMEM_*, GHND, LHND, associated routines
#ifndef NOMEMMGR
# define NOMEMMGR
#endif

// typedef METAFILEPICT
#ifndef NOMETAFILE
# define NOMETAFILE
#endif

// Macros min(a,b) and max(a,b)
#ifndef NOMINMAX
# define NOMINMAX
#endif

// OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#ifndef NOOPENFILE
# define NOOPENFILE
#endif

// SB_* and scrolling routines
#ifndef NOSCROLL
# define NOSCROLL
#endif

// All Service Controller routines, SERVICE_ equates, etc.
#ifndef NOSERVICE
# define NOSERVICE
#endif

// Sound driver routines
#ifndef NOSOUND
# define NOSOUND
#endif

// SetWindowsHook and WH_*
#ifndef NOWH
# define NOWH
#endif

// COMM driver routines
#ifndef NOCOMM
# define NOCOMM
#endif

// Kanji support stuff
#ifndef NOKANJI
# define NOKANJI
#endif

// Help engine interface
#ifndef NOHELP
# define NOHELP
#endif

// Profiler interface
#ifndef NOPROFILER
# define NOPROFILER
#endif

// DeferWindowPos routines
#ifndef NODEFERWINDOWPOS
# define NODEFERWINDOWPOS
#endif

// Modem Configuration Extensions
#ifndef NOMCX
# define NOMCX
#endif

// typedef TEXTMETRIC and associated routines
#ifndef NOTEXTMETRIC
# define NOTEXTMETRIC
#endif

// other
#ifndef NOCRYPT
# define NOCRYPT
#endif

// needed for Cairo
#undef NORASTEROPS // Binary and Tertiary raster ops

// needed for pugl
#undef NOVIRTUALKEYCODES // VK_*
#undef NOWINMESSAGES // WM_*, EM_*, LB_*, CB_*
#undef NOWINSTYLES // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#undef NOMENUS // MF_*
#undef NOICONS // IDI_*
#undef NOSYSCOMMANDS // SC_*
#undef NOSHOWWINDOW // SW_*
#undef NOCLIPBOARD // Clipboard routines
#undef NOCTLMGR // Control and Dialog routines
#undef NOGDI // All GDI defines and routines
#undef NOUSER // All USER defines and routines
#undef NONLS // All NLS defines and routines
#undef NOMSG // typedef MSG and associated routines
#undef NOWINOFFSETS // GWL_*, GCL_*, associated routines

#define Rectangle WindowsRectangle

#include <winsock2.h>
#include <windows.h>

#undef Rectangle

#endif // DISTRHO_WINDOWS_INCLUDE_H_INCLUDED
