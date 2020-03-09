/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
  Plugin to demonstrate File handling within DPF.
 */
class FileHandlingExamplePlugin : public Plugin
{
public:
    FileHandlingExamplePlugin()
        : Plugin(kParameterCount, 0, kStateCount)
    {
        std::memset(fParameters, 0, sizeof(fParameters));
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const override
    {
        return "FileHandling";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Plugin to demonstrate File handling.";
    }

   /**
      Get the plugin author/maker.
    */
    const char* getMaker() const override
    {
        return "DISTRHO";
    }

   /**
      Get the plugin homepage.
    */
    const char* getHomePage() const override
    {
        return "https://github.com/DISTRHO/DPF";
    }

   /**
      Get the plugin license name (a single line of text).
      For commercial plugins this should return some short copyright information.
    */
    const char* getLicense() const override
    {
        return "ISC";
    }

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const override
    {
        return d_version(0, 0, 0);
    }

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    int64_t getUniqueId() const override
    {
        return d_cconst('d', 'F', 'i', 'H');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsOutput|kParameterIsInteger;

        switch (index)
        {
        case kParameterFileSize1:
            param.name   = "Size #1";
            param.symbol = "size1";
            break;
        case kParameterFileSize2:
            param.name   = "Size #2";
            param.symbol = "size2";
            break;
        case kParameterFileSize3:
            param.name   = "Size #3";
            param.symbol = "size3";
            break;
        }
    }

   /**
      Set the state key and default value of @a index.@n
      This function will be called once, shortly after the plugin is created.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        switch (index)
        {
        case kStateFile1:
            stateKey = "file1";
            break;
        case kStateFile2:
            stateKey = "file2";
            break;
        case kStateFile3:
            stateKey = "file3";
            break;
        }

        defaultStateValue = "";
    }

   /**
      TODO API under construction
    */
    bool isStateFile(uint32_t index) override
    {
        switch (index)
        {
        case kStateFile1:
        case kStateFile2:
        case kStateFile3:
            return true;
        }

        return false;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
      We have no parameters in this plugin example, so we do nothing with the function.
    */
    float getParameterValue(uint32_t index) const override
    {
        return fParameters[index];
    }

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.

      This function will only be called for parameter inputs.
      Since we have no parameters inputs in this example, so we do nothing with the function.
    */
    void setParameterValue(uint32_t, float) override {}

   /**
      Change an internal state @a key to @a value.
    */
    void setState(const char* key, const char* value) override
    {
        d_stdout("DSP setState %s %s", key, value);
        int fileId = -1;

        /**/ if (std::strcmp(key, "file1") == 0)
            fileId = 0;
        else if (std::strcmp(key, "file2") == 0)
            fileId = 1;
        else if (std::strcmp(key, "file3") == 0)
            fileId = 2;

        if (fileId == -1)
            return;

        if (FILE* const fh = fopen(value, "r"))
        {
            fseek(fh, 0, SEEK_END);

            // filesize
            const long int size = ftell(fh);
            d_stdout("size of %s is %li", value, size);
            fParameters[kParameterFileSize1 + fileId] = static_cast<float>(size) / 1000.0f;

            fclose(fh);
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
       /**
          This plugin doesn't do audio, it just demonstrates file handling usage.
          So here we directly copy inputs over outputs, leaving the audio untouched.
          We need to be careful in case the host re-uses the same buffer for both ins and outs.
        */
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*frames);
    }

    // -------------------------------------------------------------------------------------------------------

private:
    float fParameters[kParameterCount];

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileHandlingExamplePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new FileHandlingExamplePlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
