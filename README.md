# DPF - DISTRHO Plugin Framework

DPF is designed to make development of new plugins an easy and enjoyable task.<br/>
It allows developers to create plugins with custom UIs using a simple C++ API.<br/>
The framework facilitates exporting various different plugin formats from the same code-base.<br/>

DPF can build for LADSPA, DSSI, LV2 and VST formats.<br/>
LADSPA and DSSI implementations are complete, LV2 and VST at ~75% completion.<br/>


Plugin DSP and UI communication is done via key-value string pairs.<br/>
You send messages from the UI to the DSP side, which is automatically saved in the host when required.<br/>

Getting time information from the host is possible.<br/>
It uses the same format as the JACK Transport API, making porting some code easier.<br/>


List of plugins made with DPF:<br/>
 - [DISTRHO Mini-Series](https://github.com/DISTRHO/mini-series)
 - [DISTRHO Nekobi](https://github.com/DISTRHO/nekobi)
 - [DISTRHO ProM](https://github.com/DISTRHO/prom)
 - [ZamAudio Suite](https://github.com/zamaudio/zam-plugins-DPF)
