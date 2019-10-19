# DPF - DISTRHO Plugin Framework
[![Build Status](https://travis-ci.org/DISTRHO/DPF.png)](https://travis-ci.org/DISTRHO/DPF)

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


List of plugins made with DPF:<br/>
 - [DISTRHO glBars](https://github.com/DISTRHO/glBars)
 - [DISTRHO Kars](https://github.com/DISTRHO/Kars)
 - [DISTRHO Mini-Series](https://github.com/DISTRHO/Mini-Series)
 - [DISTRHO MVerb](https://github.com/DISTRHO/MVerb)
 - [DISTRHO Nekobi](https://github.com/DISTRHO/Nekobi)
 - [DISTRHO ProM](https://github.com/DISTRHO/ProM)
 - [DISTRHO ndc Plugs](https://github.com/DISTRHO/ndc-Plugs)
 - [Juice Plugins](https://github.com/DISTRHO/JuicePlugins) (work in progress)
 - [ZamAudio Suite](https://github.com/zamaudio/zam-plugins)
 - [DragonFly-Reverb](https://github.com/michaelwillis/dragonfly-reverb)
 - [Wolf-Shaper](https://github.com/pdesaulniers/wolf-shaper)
 - [Wolf-Spectrum](https://github.com/pdesaulniers/wolf-spectrum)
 - [Wolf-LFO](https://github.com/pdesaulniers/wolf-lfo)
 - [YK Chorus](https://github.com/SpotlightKid/ykchorus)
 - [Ninjas2](https://github.com/rghvdberg/ninjas2)
 - [String-machine](https://github.com/jpcima/string-machine)
 - [Rezonateur](https://github.com/jpcima/rezonateur)
 - [QuadraFuzz](https://github.com/jpcima/quadrafuzz)
 - [Shiru Plugins](https://github.com/linuxmao-org/shiru-plugins)
 - [Uhhyou Plugins](https://github.com/ryukau/LV2Plugins)
 - [midiomatic](https://github.com/SpotlightKid/midiomatic)
 - [osamc-lv2-workshop](https://github.com/osamc-lv2-workshop/lv2-workshop)


Plugin examples are available in the `example/` folder inside this repo.<br/>
Extra OpenGL UI examples are available [here](https://github.com/DISTRHO/gl-examples).


Online documentation is available at [https://distrho.github.io/DPF/](https://distrho.github.io/DPF/).
