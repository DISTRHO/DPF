# About the Project
This is a completely "clean room" untainted reverse engineered "SDK" for the VST 2.x interface. It was reverse engineered from binaries where no license restricting the reverse engineering was attached, or where the legal system explicitly allowed reverse engineering for the purpose of interoperability.

# Frequently Asked Questions
## Is this legal? Can I use this in my own product?
**Disclaimer:** I am not a lawyer. The information presented below is purely from available copyright laws that I could find about this topic. You should always consult with a lawyer first before including this in your product.

As this only enables interoperability with existing VST 2.x programs and addons, it is considered to be reverse engineering in the name of interoperability. In most of the developed world, this is considered completely legal and is fine to be used by anyone, as long as it is not the only function of the product.

Note that this does not grant any patent licenses, nor does it grant you any right to use trademarks in the names. That could mean that you can't advertise your product as having support for VST, and can't use VST in the name or presentation of the product at all unless you have permission to do so.

## Why recreate an SDK for something officially abandoned by the creators?
There is a ton of software that is only capable of loading VST2.x audio effects, and Steinberg has made no effort to create a VST3-to-VST2-adapter for that software. Notable software includes Audacity and OBS Studio, which both likely felt restricted by the license additions Steinberg added to the GPL license.

## How did you reverse engineer this?
The reverse engineering was done with various tools (mostly disassemblers to x86 assembly code), hooking into system APIs, attempting to mimic functionality through observation and testing, and other methods. Primarily Visual Studio Code was used to write the header files, and Visual Studio 2019 Express was used to create fake VST plugins/hosts to figure out actual behavior.

### Which binaries were disassembled?
* A fake VST2 host (using this header) was created to verify against existing plugins.
* A fake VST2 plugin (using this header) was created to verify against existing hosts.
* OBS Studio and Audacity were used to verify compatability between closed source and open source VST hosts.
