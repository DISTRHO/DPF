# DPF - DISTRHO Plugin Framework
[![makefile](https://github.com/DISTRHO/DPF/actions/workflows/makefile.yml/badge.svg)](https://github.com/DISTRHO/DPF/actions/workflows/makefile.yml)
[![cmake](https://github.com/DISTRHO/DPF/actions/workflows/cmake.yml/badge.svg)](https://github.com/DISTRHO/DPF/actions/workflows/cmake.yml)
[![example-plugins](https://github.com/DISTRHO/DPF/actions/workflows/example-plugins.yml/badge.svg)](https://github.com/DISTRHO/DPF/actions/workflows/example-plugins.yml)

DPF is designed to make development of new plugins an easy and enjoyable task.<br/>
It allows developers to create plugins with custom UIs using a simple C++ API.<br/>
The framework facilitates exporting various different plugin formats from the same code-base.<br/>

DPF can build for LADSPA, DSSI, LV2 and VST formats.<br/>
All current plugin format implementations are complete.<br/>
A JACK/Standalone mode is also available, allowing you to quickly test plugins.<br/>

Plugin DSP and UI communication is done via key-value string pairs.<br/>
You send messages from the UI to the DSP side, which is automatically saved in the host when required.<br/>
(You can also store state internally if needed, but this breaks DSSI compatibility).<br/>

Getting time information from the host is possible.<br/>
It uses the same format as the JACK Transport API, making porting some code easier.<br/>


## Licensing

DPF is released under ISC, which basically means you can do whatever you want as long as you credit the original authors.  
Some plugin formats may have additional restrictions, see [LICENSING.md](LICENSING.md) for details.


## Help and documentation

Bug reports happen on the [DPF github project](https://github.com/DISTRHO/DPF/issues).

Online documentation is available at [https://distrho.github.io/DPF/](https://distrho.github.io/DPF/).

Online help and discussion about DPF happens in the [kx.studio chat, DPF room](https://chat.kx.studio/).


## List of plugins made with DPF:
 - [DISTRHO glBars](https://github.com/DISTRHO/glBars)
 - [DISTRHO Kars](https://github.com/DISTRHO/Kars)
 - [DISTRHO Mini-Series](https://github.com/DISTRHO/Mini-Series)
 - [DISTRHO MVerb](https://github.com/DISTRHO/MVerb)
 - [DISTRHO ndc Plugs](https://github.com/DISTRHO/ndc-Plugs)
 - [DISTRHO Nekobi](https://github.com/DISTRHO/Nekobi)
 - [DISTRHO ProM](https://github.com/DISTRHO/ProM)
 - [Dragonfly Reverb](https://michaelwillis.github.io/dragonfly-reverb)
 - [Fogpad-port](https://github.com/linuxmao-org/fogpad-port)
 - [Ninjas2](https://github.com/rghvdberg/ninjas2)
 - [osamc-lv2-workshop](https://github.com/osamc-lv2-workshop/lv2-workshop) (simple plugins code examples)
 - [QuadraFuzz](https://github.com/jpcima/quadrafuzz)
 - [Regrader-Port](https://github.com/linuxmao-org/regrader-port)
 - [Rezonateur](https://github.com/jpcima/rezonateur)
 - [Spectacle-analyzer](https://github.com/jpcima/spectacle/)
 - [Stone Phaser](https://github.com/jpcima/stone-phaser)
 - [String-machine](https://github.com/jpcima/string-machine)
 - [Uhhyou Plugins](https://github.com/ryukau/LV2Plugins)
 - [VL1-emulator](https://github.com/linuxmao-org/VL1-emulator)
 - [Wolf Shaper](https://github.com/pdesaulniers/wolf-shaper)
 - [Wolf Spectrum](https://github.com/pdesaulniers/wolf-spectrum)
 - [YK Chorus](https://github.com/SpotlightKid/ykchorus)
 - [ZamAudio Suite](https://github.com/zamaudio/zam-plugins)
 ## Work in progress
 - [CV-LFO-blender-LV2](https://github.com/BramGiesen/cv-lfo-blender-lv2)
 - [fverb](https://github.com/jpcima/fverb)
 - [Juice Plugins](https://github.com/DISTRHO/JuicePlugins)
 - [gunshot](https://github.com/soerenbnoergaard/gunshot)
 - [midiomatic](https://github.com/SpotlightKid/midiomatic)
 - [Shiro Plugins](https://github.com/ninodewit/SHIRO-Plugins/)
 - [Shiru Plugins](https://github.com/linuxmao-org/shiru-plugins)

Checking the [github "DPF" tag](https://github.com/topics/dpf) can potentially bring up other DPF-made plugins.

Plugin examples are available in the `example/` folder inside this repo.
