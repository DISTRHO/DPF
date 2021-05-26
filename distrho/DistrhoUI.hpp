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

#ifndef DISTRHO_UI_HPP_INCLUDED
#define DISTRHO_UI_HPP_INCLUDED

#include "extra/LeakDetector.hpp"
#include "src/DistrhoPluginChecks.h"

#ifdef DGL_CAIRO
# include "Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# include "OpenGL.hpp"
#endif
#ifdef DGL_VULKAN
# include "Vulkan.hpp"
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "../dgl/Base.hpp"
# include "extra/ExternalWindow.hpp"
typedef DISTRHO_NAMESPACE::ExternalWindow UIWidget;
#elif DISTRHO_UI_USE_CUSTOM
# include DISTRHO_UI_CUSTOM_INCLUDE_PATH
typedef DISTRHO_UI_CUSTOM_WIDGET_TYPE UIWidget;
#elif DISTRHO_UI_USE_CAIRO
# include "../dgl/Cairo.hpp"
typedef DGL_NAMESPACE::CairoTopLevelWidget UIWidget;
#elif DISTRHO_UI_USE_NANOVG
# include "../dgl/NanoVG.hpp"
typedef DGL_NAMESPACE::NanoTopLevelWidget UIWidget;
#else
# include "../dgl/TopLevelWidget.hpp"
typedef DGL_NAMESPACE::TopLevelWidget UIWidget;
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * DPF UI */

/**
   @addtogroup MainClasses
   @{
 */

/**
   DPF UI class from where UI instances are created.

   @note You must call setSize during construction,
   @TODO Detailed information about this class.
 */
class UI : public UIWidget
{
public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    UI(uint width = 0, uint height = 0);

   /**
      Destructor.
    */
    virtual ~UI();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Get the color used for UI background (i.e. window color) in RGBA format.
      Returns 0 by default, in case of error or lack of host support.

      The following example code can be use to extract individual colors:
      ```
      const int red   = (bgColor >> 24) & 0xff;
      const int green = (bgColor >> 16) & 0xff;
      const int blue  = (bgColor >>  8) & 0xff;
      ```
    */
    uint getBackgroundColor() const noexcept;

   /**
      Get the color used for UI foreground (i.e. text color) in RGBA format.
      Returns 0xffffffff by default, in case of error or lack of host support.

      The following example code can be use to extract individual colors:
      ```
      const int red   = (fgColor >> 24) & 0xff;
      const int green = (fgColor >> 16) & 0xff;
      const int blue  = (fgColor >>  8) & 0xff;
      ```
    */
    uint getForegroundColor() const noexcept;

   /**
      Get the current sample rate used in plugin processing.
      @see sampleRateChanged(double)
    */
    double getSampleRate() const noexcept;

   /**
      editParameter.

      Touch/pressed-down event.
      Lets the host know the user is tweaking a parameter.
      Required in some hosts to record automation.
    */
    void editParameter(uint32_t index, bool started);

   /**
      setParameterValue.

      Change a parameter value in the Plugin.
    */
    void setParameterValue(uint32_t index, float value);

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      setState.
      @TODO Document this.
    */
    void setState(const char* key, const char* value);
#endif

#if DISTRHO_PLUGIN_WANT_STATEFILES
   /**
      Request a new file from the host, matching the properties of a state key.@n
      This will use the native host file browser if available, otherwise a DPF built-in file browser is used.@n
      Response will be sent asynchronously to stateChanged, with the matching key and the new file as the value.@n
      It is not possible to know if the action was cancelled by the user.

      @return Success if a file-browser was opened, otherwise false.
      @note You cannot request more than one file at a time.
    */
    bool requestStateFile(const char* key);
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
   /**
      Send a single MIDI note from the UI to the plugin DSP side.@n
      A note with zero velocity will be sent as note-off (MIDI 0x80), otherwise note-on (MIDI 0x90).
    */
    void sendNote(uint8_t channel, uint8_t note, uint8_t velocity);
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
   /* --------------------------------------------------------------------------------------------------------
    * Direct DSP access - DO NOT USE THIS UNLESS STRICTLY NECESSARY!! */

   /**
      getPluginInstancePointer.
      @TODO Document this.
    */
    void* getPluginInstancePointer() const noexcept;
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
   /* --------------------------------------------------------------------------------------------------------
    * External UI helpers */

   /**
      Get the bundle path that will be used for the next UI.
      @note: This function is only valid during createUI(),
             it will return null when called from anywhere else.
    */
    static const char* getNextBundlePath() noexcept;

   /**
      Get the scale factor that will be used for the next UI.
      @note: This function is only valid during createUI(),
             it will return 1.0 when called from anywhere else.
    */
    static double getNextScaleFactor() noexcept;

# if DISTRHO_PLUGIN_HAS_EMBED_UI
   /**
      Get the Window Id that will be used for the next created window.
      @note: This function is only valid during createUI(),
             it will return 0 when called from anywhere else.
    */
    static uintptr_t getNextWindowId() noexcept;
# endif
#endif

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    virtual void parameterChanged(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      A program has been loaded on the plugin side.@n
      This is called by the host to inform the UI about program changes.
    */
    virtual void programLoaded(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      A state has changed on the plugin side.@n
      This is called by the host to inform the UI about state changes.
    */
    virtual void stateChanged(const char* key, const char* value) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks (optional) */

   /**
      Optional callback to inform the UI about a sample rate change on the plugin side.
      @see getSampleRate()
    */
    virtual void sampleRateChanged(double newSampleRate);

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
   /* --------------------------------------------------------------------------------------------------------
    * UI Callbacks (optional) */

   /**
      uiIdle.
      @TODO Document this.
    */
    virtual void uiIdle() {}

# ifndef DGL_FILE_BROWSER_DISABLED
   /**
      File browser selected function.
      @see Window::onFileSelected(const char*)
    */
    virtual void uiFileBrowserSelected(const char* filename);
# endif

   /**
      OpenGL window reshape function, called when parent window is resized.
      You can reimplement this function for a custom OpenGL state.
      @see Window::onReshape(uint,uint)
    */
    virtual void uiReshape(uint width, uint height);

   /* --------------------------------------------------------------------------------------------------------
    * UI Resize Handling, internal */

   /**
      OpenGL widget resize function, called when the widget is resized.
      This is overriden here so the host knows when the UI is resized by you.
      @see Widget::onResize(const ResizeEvent&)
    */
    void onResize(const ResizeEvent& ev) override;
#endif

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const uiData;
    friend class UIExporter;
    friend class UIExporterWindow;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

/**
   @addtogroup EntryPoints
   @{
 */

/**
   createUI.
   @TODO Document this.
 */
extern UI* createUI();

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
