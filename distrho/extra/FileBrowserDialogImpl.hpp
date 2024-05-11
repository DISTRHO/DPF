/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#if !defined(DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED) && !defined(DGL_FILE_BROWSER_DIALOG_HPP_INCLUDED)
# error bad include
#endif

#if !defined(DGL_USE_FILE_BROWSER) && defined(DISTRHO_UI_FILE_BROWSER) && DISTRHO_UI_FILE_BROWSER == 0
# error To use File Browser in DPF plugins please set DISTRHO_UI_FILE_BROWSER to 1
#endif

// --------------------------------------------------------------------------------------------------------------------
// File Browser Dialog stuff

struct FileBrowserData;
typedef FileBrowserData* FileBrowserHandle;

// --------------------------------------------------------------------------------------------------------------------

/**
  File browser options, for customizing the file browser dialog.@n
  By default the file browser dialog will be work as "open file" in the current working directory.
*/
struct FileBrowserOptions {
    /** Whether we are saving, opening files otherwise (default) */
    bool saving;

    /** Default filename when saving, required in some platforms (basename without path separators) */
    const char* defaultName;

    /** Start directory, uses current working directory if null */
    const char* startDir;

    /** File browser dialog window title, uses "FileBrowser" if null */
    const char* title;

    /** Class name of the matching Application instance that controls this dialog */
    const char* className;

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
        defaultName(nullptr),
        startDir(nullptr),
        title(nullptr),
        className(nullptr),
        buttons() {}
};

// --------------------------------------------------------------------------------------------------------------------

/**
  Create a new file browser dialog.

  @p isEmbed:     Whether the window this dialog belongs to is an embed/child window (needed to close dialog on Windows)
  @p windowId:    The native window id to attach this dialog to as transient parent (X11 Window, HWND or NSView*)
  @p scaleFactor: Scale factor to use (only used on X11)
  @p options:     Extra options, optional
  By default the file browser dialog will work as "open file" in the current working directory.
*/
FileBrowserHandle fileBrowserCreate(bool isEmbed,
                                    uintptr_t windowId,
                                    double scaleFactor,
                                    const FileBrowserOptions& options = FileBrowserOptions());

/**
  Idle the file browser dialog handle.@n
  Returns true if dialog was closed (with or without a file selection),
  in which case this idle function must not be called anymore for this handle.
  You can then call fileBrowserGetPath to know the selected file (or null if cancelled).
*/
bool fileBrowserIdle(FileBrowserHandle handle);

/**
  Close and free the file browser dialog, handle must not be used afterwards.
*/
void fileBrowserClose(FileBrowserHandle handle);

/**
  Get the path chosen by the user or null.@n
  Should only be called after fileBrowserIdle returns true.
*/
const char* fileBrowserGetPath(FileBrowserHandle handle);

// --------------------------------------------------------------------------------------------------------------------
