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

#ifndef DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED
#define DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
namespace DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE {
#endif

// --------------------------------------------------------------------------------------------------------------------
// File Browser Dialog stuff

struct FileBrowserData;
typedef FileBrowserData* FileBrowserHandle;

// --------------------------------------------------------------------------------------------------------------------

/**
  File browser options.
  @see Window::openFileBrowser

  @note This is exactly the same API as provided by the Window class,
        but redefined so that non-embed/DGL based UIs can still use file browser related functions.
*/
struct FileBrowserOptions {
    /** Whether we are saving, opening files otherwise (default) */
    bool saving;

    /** Start directory, uses current working directory if null */
    const char* startDir;

    /** File browser dialog window title, uses "FileBrowser" if null */
    const char* title;

    // TODO file filter

   /**
      File browser button state.
      This allows to customize the behaviour of the file browse dialog buttons.
      Note these are merely hints, not all systems support them.
    */
    enum ButtonState {
      kButtonInvisible,
      kButtonVisibleUnchecked,
      kButtonVisibleChecked,
    };

   /**
      File browser buttons.
    */
    struct Buttons {
      /** Whether to list all files vs only those with matching file extension */
      ButtonState listAllFiles;
      /** Whether to show hidden files */
      ButtonState showHidden;
      /** Whether to show list of places (bookmarks) */
      ButtonState showPlaces;

      /** Constructor for default values */
      Buttons()
            : listAllFiles(kButtonVisibleChecked),
              showHidden(kButtonVisibleUnchecked),
              showPlaces(kButtonVisibleChecked) {}
    } buttons;

    /** Constructor for default values */
    FileBrowserOptions()
      : saving(false),
        startDir(nullptr),
        title(nullptr),
        buttons() {}
};

// --------------------------------------------------------------------------------------------------------------------

FileBrowserHandle fileBrowserOpen(bool isEmbed, uintptr_t windowId, double scaleFactor,
                                  const char* startDir, const char* windowTitle, const FileBrowserOptions& options);

// --------------------------------------------------------------------------------------------------------------------
// returns true if dialog was closed (with or without a file selection)

bool fileBrowserIdle(const FileBrowserHandle handle);

// --------------------------------------------------------------------------------------------------------------------
// close sofd file dialog

void fileBrowserClose(const FileBrowserHandle handle);

// --------------------------------------------------------------------------------------------------------------------
// get path chosen via sofd file dialog

const char* fileBrowserGetPath(const FileBrowserHandle handle);

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
}
#endif

END_NAMESPACE_DISTRHO

#endif // DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED
