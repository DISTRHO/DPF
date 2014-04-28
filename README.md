-- DPF - DISTRHO Plugin Framework

DPF is a plugin framework designed to make development of new plugins an easy and enjoyable task.
You write your plugin code against the simple DPF API and let it handle all the different plugin formats.

DPF can build for LADSPA, DSSI, LV2 and VST formats.
LADSPA and DSSI implementations are complete, LV2 and VST at ~75% completion.


Plugin DSP and UI communication is done via key-value string pairs.
You send messages from the UI to the DSP side, which is automatically saved in the host when required.

Getting time information from the host is possible.
It uses the same format as the JACK Transport API, making porting some code easier.
