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

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#ifndef DISTRHO_PLUGIN_BRAND_ID
# error DISTRHO_PLUGIN_BRAND_ID undefined!
#endif

#ifndef DISTRHO_PLUGIN_UNIQUE_ID
# error DISTRHO_PLUGIN_UNIQUE_ID undefined!
#endif

#include <fstream>
#include <iostream>

USE_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

void generate_au_plist(const PluginExporter& plugin,
                       const char* const basename,
                       const char* const license)
{
    std::cout << "Writing Info.plist..."; std::cout.flush();
    std::fstream outputFile("Info.plist", std::ios::out);

    const uint32_t version = plugin.getVersion();

    const uint32_t majorVersion = (version & 0xFF0000) >> 16;
    const uint32_t minorVersion = (version & 0x00FF00) >> 8;
    const uint32_t microVersion = (version & 0x0000FF) >> 0;

    outputFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    outputFile << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    outputFile << "<plist>\n";
    outputFile << "  <dict>\n";
    outputFile << "    <key>CFBundleExecutable</key>\n";
    outputFile << "    <string>" << basename << "</string>\n";
    outputFile << "    <key>CFBundleIconFile</key>\n";
    outputFile << "    <string></string>\n";
    outputFile << "    <key>CFBundleIdentifier</key>\n";
    outputFile << "    <string>" DISTRHO_PLUGIN_CLAP_ID "</string>\n";
    outputFile << "    <key>CFBundleName</key>\n";
    outputFile << "    <string>" << basename << "</string>\n";
    outputFile << "    <key>CFBundleDisplayName</key>\n";
    outputFile << "    <string>" << plugin.getName() << "</string>\n";
    outputFile << "    <key>CFBundlePackageType</key>\n";
    outputFile << "    <string>BNDL</string>\n";
    outputFile << "    <key>CFBundleSignature</key>\n";
    outputFile << "    <string>" << "????" << "</string>\n";
    outputFile << "    <key>CFBundleShortVersionString</key>\n";
    outputFile << "    <string>" << majorVersion << "." << minorVersion << "." << microVersion << "</string>\n";
    outputFile << "    <key>CFBundleVersion</key>\n";
    outputFile << "    <string>" << majorVersion << "." << minorVersion << "." << microVersion << "</string>\n";
    if (license != nullptr && license[0] != '\0')
    {
        outputFile << "    <key>NSHumanReadableCopyright</key>\n";
        outputFile << "    <string>" << license << "</string>\n";
    }
    outputFile << "    <key>NSHighResolutionCapable</key>\n";
    outputFile << "    <true/>\n";
    outputFile << "    <key>AudioComponents</key>\n";
    outputFile << "    <array>\n";
    outputFile << "      <dict>\n";
    outputFile << "        <key>name</key>\n";
    outputFile << "        <string>" << plugin.getMaker() << ": " << plugin.getName() << "</string>\n";
    outputFile << "        <key>description</key>\n";
    outputFile << "        <string>" << plugin.getDescription() << "</string>\n";
    outputFile << "        <key>factoryFunction</key>\n";
    outputFile << "        <string>PluginAUFactory</string>\n";
    outputFile << "        <key>type</key>\n";
    outputFile << "        <string>" STRINGIFY(DISTRHO_PLUGIN_AU_TYPE) "</string>\n";
    outputFile << "        <key>subtype</key>\n";
    outputFile << "        <string>" STRINGIFY(DISTRHO_PLUGIN_UNIQUE_ID) "</string>\n";
    outputFile << "        <key>manufacturer</key>\n";
    outputFile << "        <string>" STRINGIFY(DISTRHO_PLUGIN_BRAND_ID) "</string>\n";
    outputFile << "        <key>version</key>\n";
    outputFile << "        <integer>" << version << "</integer>\n";
    outputFile << "        <key>resourceUsage</key>\n";
    outputFile << "        <dict>\n";
    outputFile << "          <key>network.client</key>\n";
    outputFile << "          <true/>\n";
    outputFile << "          <key>temporary-exception.files.all.read-write</key>\n";
    outputFile << "          <true/>\n";
    outputFile << "        </dict>\n";
    outputFile << "      </dict>\n";
    outputFile << "    </array>\n";
    outputFile << "  </dict>\n";
    outputFile << "</plist>\n";

    outputFile.close();
    std::cout << " done!" << std::endl;
}

// --------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc <= 1)
        return 1;

    // Dummy plugin to get data from
    d_nextBufferSize = 512;
    d_nextSampleRate = 44100.0;
    d_nextPluginIsDummy = true;
    PluginExporter plugin(nullptr, nullptr, nullptr, nullptr);
    d_nextBufferSize = 0;
    d_nextSampleRate = 0.0;
    d_nextPluginIsDummy = false;

    String license(plugin.getLicense());

    if (license.isEmpty())
    {}
    // License as URL, use as-is
    else if (license.contains("://"))
    {}
    // License contains quotes, use as-is
    else if (license.contains('"'))
    {}
    // Regular license string, convert to URL as much as we can
    else
    {
        const String uplicense(license.asUpper());

        // for reference, see https://spdx.org/licenses/

        // common licenses
        /**/ if (uplicense == "AGPL-1.0-ONLY" ||
                 uplicense == "AGPL1" ||
                 uplicense == "AGPLV1")
        {
            license = "http://spdx.org/licenses/AGPL-1.0-only.html";
        }
        else if (uplicense == "AGPL-1.0-OR-LATER" ||
                 uplicense == "AGPL1+" ||
                 uplicense == "AGPLV1+")
        {
            license = "http://spdx.org/licenses/AGPL-1.0-or-later.html";
        }
        else if (uplicense == "AGPL-3.0-ONLY" ||
                 uplicense == "AGPL3" ||
                 uplicense == "AGPLV3")
        {
            license = "http://spdx.org/licenses/AGPL-3.0-only.html";
        }
        else if (uplicense == "AGPL-3.0-OR-LATER" ||
                 uplicense == "AGPL3+" ||
                 uplicense == "AGPLV3+")
        {
            license = "http://spdx.org/licenses/AGPL-3.0-or-later.html";
        }
        else if (uplicense == "APACHE-2.0" ||
                 uplicense == "APACHE2" ||
                 uplicense == "APACHE-2")
        {
            license = "http://spdx.org/licenses/Apache-2.0.html";
        }
        else if (uplicense == "BSD-2-CLAUSE" ||
                 uplicense == "BSD2" ||
                 uplicense == "BSD-2")
        {
            license = "http://spdx.org/licenses/BSD-2-Clause.html";
        }
        else if (uplicense == "BSD-3-CLAUSE" ||
                 uplicense == "BSD3" ||
                 uplicense == "BSD-3")
        {
            license = "http://spdx.org/licenses/BSD-3-Clause.html";
        }
        else if (uplicense == "GPL-2.0-ONLY" ||
                 uplicense == "GPL2" ||
                 uplicense == "GPLV2")
        {
            license = "http://spdx.org/licenses/GPL-2.0-only.html";
        }
        else if (uplicense == "GPL-2.0-OR-LATER" ||
                 uplicense == "GPL2+" ||
                 uplicense == "GPLV2+" ||
                 uplicense == "GPLV2.0+" ||
                 uplicense == "GPL V2+")
        {
            license = "http://spdx.org/licenses/GPL-2.0-or-later.html";
        }
        else if (uplicense == "GPL-3.0-ONLY" ||
                 uplicense == "GPL3" ||
                 uplicense == "GPLV3")
        {
            license = "http://spdx.org/licenses/GPL-3.0-only.html";
        }
        else if (uplicense == "GPL-3.0-OR-LATER" ||
                 uplicense == "GPL3+" ||
                 uplicense == "GPLV3+" ||
                 uplicense == "GPLV3.0+" ||
                 uplicense == "GPL V3+")
        {
            license = "http://spdx.org/licenses/GPL-3.0-or-later.html";
        }
        else if (uplicense == "ISC")
        {
            license = "http://spdx.org/licenses/ISC.html";
        }
        else if (uplicense == "LGPL-2.0-ONLY" ||
                 uplicense == "LGPL2" ||
                 uplicense == "LGPLV2")
        {
            license = "http://spdx.org/licenses/LGPL-2.0-only.html";
        }
        else if (uplicense == "LGPL-2.0-OR-LATER" ||
                 uplicense == "LGPL2+" ||
                 uplicense == "LGPLV2+")
        {
            license = "http://spdx.org/licenses/LGPL-2.0-or-later.html";
        }
        else if (uplicense == "LGPL-2.1-ONLY" ||
                 uplicense == "LGPL2.1" ||
                 uplicense == "LGPLV2.1")
        {
            license = "http://spdx.org/licenses/LGPL-2.1-only.html";
        }
        else if (uplicense == "LGPL-2.1-OR-LATER" ||
                 uplicense == "LGPL2.1+" ||
                 uplicense == "LGPLV2.1+")
        {
            license = "http://spdx.org/licenses/LGPL-2.1-or-later.html";
        }
        else if (uplicense == "LGPL-3.0-ONLY" ||
                 uplicense == "LGPL3" ||
                 uplicense == "LGPLV3")
        {
            license = "http://spdx.org/licenses/LGPL-2.0-only.html";
        }
        else if (uplicense == "LGPL-3.0-OR-LATER" ||
                 uplicense == "LGPL3+" ||
                 uplicense == "LGPLV3+")
        {
            license = "http://spdx.org/licenses/LGPL-3.0-or-later.html";
        }
        else if (uplicense == "MIT")
        {
            license = "http://spdx.org/licenses/MIT.html";
        }

        // generic fallbacks
        else if (uplicense.startsWith("GPL"))
        {
            license = "http://opensource.org/licenses/gpl-license";
        }
        else if (uplicense.startsWith("LGPL"))
        {
            license = "http://opensource.org/licenses/lgpl-license";
        }

        // unknown or not handled yet, log a warning
        else
        {
            d_stderr("Unknown license string '%s'", license.buffer());
        }
    }

    generate_au_plist(plugin, argv[1], license);

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
