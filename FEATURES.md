# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.

| Feature             | JACK/Standalone                                  | LADSPA                             | DSSI | LV2           | VST2   | VST3   |
|---------------------|--------------------------------------------------|------------------------------------|------|---------------|--------|--------|
| Audio port groups   | [Yes*](FEATURES.md#jack-audio-port-groups)       | No                                 | No   | Yes           | No     | No*    |
| Audio port as CV    | Yes                                              | No                                 | No   | Yes           | No     | [No*](#vst3-is-work-in-progress)  |
| Audio sidechan      | Yes                                              | No                                 | No   | Yes           | [No*](#vst2-potential-support)  | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| Bypass control
| MIDI input          | Yes                                              | No                                 | Yes  | Yes           | Yes    | Yes    |
| MIDI output         | Yes                                              | No                                 | No   | Yes           | Yes    | Yes    |
| Parameter changes   | Yes                                              | No                                 | No   | [No*](FEATURES.md#lv2-parameter-changes)         | Yes    | Yes    |
| Parameter groups    | No                                               | No                                 | No   | Yes           | Yes*   | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| Parameter outputs   | No                                               | No                                 | No   | Yes           | Yes    | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| Parameter triggers  |
| Programs            | [Yes*](FEATURES.md#jack-parameters-and-programs) | [No*](FEATURES.md#ladspa-programs) | Yes* | Yes           | No*    | Yes    |
| States              | Yes                                              | No                                 | Yes* | Yes           | Yes    | Yes    |
| Full/internal state
| Time position
| UI                  | [Yes*](FEATURES.md#jack-custom-ui-only)          | No                                 | Ext. | Embed or Ext. | Embed  | Embed  |
| UI bg/fg colors
| UI direct access
| UI host-filebrowser
| UI host-resize      | Yes                                              | No                                 | Yes  | Yes           | No     | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| UI remote control
| UI sendNote         | Yes                                              | No                                 | Yes  | Yes           | Yes    | Yes    |

For things that could be unclear:

- "States" refers to DPF API support, supporting key-value string pairs for internal state saving
- "Full state" refers to plugins updating their state internally without outside intervention (like host or UI)
- "UI direct access" means `DISTRHO_PLUGIN_WANT_DIRECT_ACCESS` is possible, that is, running DSP and UI on the same process
- "UI remote control" means running the UI on a separate machine (for example over the network)

# Special notes

## JACK audio port groups

DPF will set JACK metadata information for grouping audio ports.  
This is not supported by most JACK applications at the moment.

## JACK parameters and programs

Under JACK/Stanlone mode, MIDI input events will trigger program and parameter changes.  
MIDI program change events work as expected (that is, MIDI program change 0 will load 1st plugin program).  
MIDI CCs are used for parameter changes (matching the `midiCC` value you set on each parameter).

## JACK custom UI only

There is no generic plugin editor view.  
If your plugin has no custom UI, the standalone executable will run but not show any window.

## LADSPA programs

Programs for LADSPA could be done via LRDF but this is not supported in DPF.

## DSSI State

DSSI only supports states when called via UI, no "full state" possible

## LV2 parameter changes

Possible through a custom extension, not implemented on most hosts.  
For now you can pretty much assume it is not supported.

## VST2 potential support

Not supported in DPF at the moment.  
It could eventually be, but likely not due to VST2 being phased out by Steinberg.  
Contact DPF authors if you require such a feature.

## VST2 programs

VST2 program support requires saving state of all programs in memory, which is very expensive and thus not done in DPF.

## VST3 is work in progress

Feature is possible, just not implemented yet in DPF.
