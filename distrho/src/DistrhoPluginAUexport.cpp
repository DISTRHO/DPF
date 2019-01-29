/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginInternal.hpp"

#include <fstream>
#include <iostream>

// -----------------------------------------------------------------------

int au_generate_r(const char* const basename)
{
    USE_NAMESPACE_DISTRHO

    // Dummy plugin to get data from
    d_lastBufferSize = 512;
    d_lastSampleRate = 44100.0;
    PluginExporter plugin(nullptr, nullptr);
    d_lastBufferSize = 0;
    d_lastSampleRate = 0.0;

    String filename(basename);
    if (! filename.endsWith("/"))
        filename += "/";
    filename += "DistrhoPluginInfo.r";

    // ---------------------------------------------

    std::cout << "Writing DistrhoPluginInfo.r..."; std::cout.flush();
    std::fstream rFile(filename, std::ios::out);

    // full name
    {
        String fullname(plugin.getMaker());

        if (fullname.isEmpty())
            fullname = "DPF";

        fullname += ": ";
        fullname += plugin.getName();

        rFile << "#define DISTRHO_PLUGIN_FULL_NAME \"" + fullname + "\"\n";
    }

    // description
    {
        String description(plugin.getDescription());

        if (description.isNotEmpty())
        {
            description.replace('\n', ' ').replace('"', '\'');
        }
        else
        {
            description = plugin.getName();
            description += " AU";
        }

        rFile << "#define DISTRHO_PLUGIN_DESCRIPTION \"" + description + "\"\n";
    }

    // res id
    if (const int64_t uniqueId = plugin.getUniqueId())
    {
        if (uniqueId < 0)
        {
            d_stderr2("AU plugin Id cannot be negative");
            return 1;
        }
        if (uniqueId >= UINT32_MAX)
        {
            d_stderr2("AU plugin Id cannot be higher than uint32");
            return 1;
        }

        rFile << "#define DISTRHO_PLUGIN_AU_RES_ID \"" + String(uniqueId) + "\"\n";
    }

#ifndef DEBUG
    // version
    {
        String version(plugin.getVersion());

        if (version.isEmpty())
            version = "0";

        rFile << "#define DISTRHO_PLUGIN_VERSION " + version + "\n";
    }
#else
    rFile << "#define DISTRHO_PLUGIN_VERSION 0xFFFFFFFF\n";
#endif

    rFile.close();
    std::cout << " done!" << std::endl;

    return 0;
}

// -----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Single argument (output path) required!" << std::endl;
        return 1;
    }

    return au_generate_r(argv[1]);
}

// -----------------------------------------------------------------------

#include "DistrhoPlugin.cpp"

// -----------------------------------------------------------------------
