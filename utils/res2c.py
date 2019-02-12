#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
#
# Permission to use, copy, modify, and/or distribute this software for any purpose with
# or without fee is hereby granted, provided that the above copyright notice and this
# permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
# TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
# NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import os, sys

# -----------------------------------------------------

def res2c(namespace, filenames):

    fdH = open("%s.hpp" % namespace, "w")
    fdH.write("/* (Auto-generated binary data file). */\n")
    fdH.write("\n")
    fdH.write("#ifndef BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("#define BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("\n")
    fdH.write("namespace %s\n" % namespace)
    fdH.write("{\n")

    fdC = open("%s.cpp" % namespace, "w")
    fdC.write("/* (Auto-generated binary data file). */\n")
    fdC.write("\n")
    fdC.write("#include \"%s.hpp\"\n" % namespace)
    fdC.write("\n")

    tempIndex = 1

    for filename in filenames:
        shortFilename = filename.rsplit(os.sep, 1)[-1].split(".", 1)[0]
        shortFilename = shortFilename.replace("-", "_")

        resData = open(filename, 'rb').read()

        print("Generating data for \"%s\"" % (filename))

        fdH.write("    extern const char* %sData;\n" % shortFilename)
        fdH.write("    const unsigned int %sDataSize = %i;\n" % (shortFilename, len(resData)))

        if tempIndex != len(filenames):
            fdH.write("\n")

        fdC.write("static const unsigned char temp_%s_%i[] = {\n" % (shortFilename, tempIndex))

        curColumn = 1
        fdC.write(" ")

        for data in resData:
            if curColumn == 0:
                fdC.write(" ")

            fdC.write(" %3u," % data)

            if curColumn > 20:
                fdC.write("\n ")
                curColumn = 1
            else:
                curColumn += 1

        fdC.write("};\n")
        fdC.write("const char* %s::%sData = (const char*)temp_%s_%i;\n" % (namespace, shortFilename, shortFilename, tempIndex))

        if tempIndex != len(filenames):
            fdC.write("\n")

        tempIndex += 1

    fdH.write("}\n")
    fdH.write("\n")
    fdH.write("#endif // BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("\n")
    fdH.close()

    fdC.write("\n")
    fdC.close()

# -----------------------------------------------------

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <namespace> <resource-folder>" % sys.argv[0])
        quit()

    namespace = sys.argv[1].replace("-","_")
    resFolder = sys.argv[2]

    if not os.path.exists(resFolder):
        print("Folder '%s' does not exist" % resFolder)
        quit()

    # find resource files
    resFiles = []

    for root, dirs, files in os.walk(resFolder):
        for name in files:
            resFiles.append(os.path.join(root, name))

    resFiles.sort()

    # create code now
    res2c(namespace, resFiles)
