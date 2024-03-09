# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.  
If the limitation is within DPF, a link is provided to a description below on the reason for it.

| Feature             | JACK/Standalone                       | LADSPA             | DSSI                | LV2                           | VST2                       | VST3                       | CLAP                       | AU                         |  Feature            |
|---------------------|---------------------------------------|--------------------|---------------------|-------------------------------|----------------------------|----------------------------|----------------------------|----------------------------|---------------------|
| Audio port groups   | [Yes*](#jack-audio-port-groups)       | No                 | No                  | Yes                           | No                         | Yes                        | Yes                        | [No*](#work-in-progress)   | Audio port groups   |
| Audio port as CV    | Yes                                   | No                 | No                  | Yes                           | No                         | [Yes*](#vst3-cv)           | [No*](#work-in-progress)   | No                         | Audio port as CV    |
| Audio sidechan      | Yes                                   | No                 | No                  | Yes                           | [No*](#vst2-deprecated)    | Yes                        | Yes                        | [No*](#work-in-progress)   | Audio sidechan      |
| Bypass control      | No                                    | No                 | No                  | Yes                           | [No*](#vst2-deprecated)    | Yes                        | Yes                        | Yes                        | Bypass control      |
| MIDI input          | Yes                                   | No                 | Yes                 | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | MIDI input          |
| MIDI output         | Yes                                   | No                 | No                  | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | MIDI output         |
| Parameter changes   | Yes                                   | No                 | No                  | [No*](#lv2-parameter-changes) | Yes                        | Yes                        | Yes                        | Yes                        | Parameter changes   |
| Parameter groups    | No                                    | No                 | No                  | Yes                           | Yes                        | [No*](#work-in-progress)   | Yes                        | [No*](#work-in-progress)   | Parameter groups    |
| Parameter outputs   | No                                    | No                 | No                  | Yes                           | No                         | Yes                        | Yes                        | Yes                        | Parameter outputs   |
| Parameter triggers  | Yes                                   | No                 | No                  | Yes                           | [No*](#parameter-triggers) | [No*](#parameter-triggers) | [No*](#parameter-triggers) | [No*](#parameter-triggers) | Parameter triggers  |
| Programs            | [Yes*](#jack-parameters-and-programs) | [No*](#ladspa-rdf) | [Yes*](#dssi-state) | Yes                           | [No*](#vst2-programs)      | Yes                        | No                         | Yes                        | Programs            |
| States              | Yes                                   | No                 | [Yes*](#dssi-state) | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | States              |
| Full/internal state | Yes                                   | No                 | No                  | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | Full/internal state |
| Time position       | Yes                                   | No                 | No                  | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | Time position       |
| UI                  | [Yes*](#jack-custom-ui-only)          | No                 | External only       | Yes                           | Embed only                 | Embed only                 | Yes                        | Embed only                 | UI                  |
| UI bg/fg colors     | No                                    | No                 | No                  | Yes                           | No                         | No?                        | No                         | No                         | UI bg/fg colors     |
| UI direct access    | Yes                                   | No                 | No                  | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | UI direct access    |
| UI host-filebrowser | No                                    | No                 | No                  | Yes                           | [No*](#vst2-deprecated)    | [No*](#work-in-progress)   | [No*](#work-in-progress)   | No                         | UI host-filebrowser |
| UI host-resize      | Yes                                   | No                 | Yes                 | Yes                           | No                         | Yes                        | Yes                        | No                         | UI host-resize      |
| UI remote control   | No                                    | No                 | Yes                 | Yes                           | No                         | Yes                        | No                         | Yes                        | UI remote control   |
| UI send midi note   | Yes                                   | No                 | Yes                 | Yes                           | Yes                        | Yes                        | Yes                        | Yes                        | UI send midi note   |

For things that could be unclear:

- "States" refers to DPF API support, supporting key-value string pairs for internal state saving
- "Full state" refers to plugins updating their state internally without outside intervention (like host or UI)
- "UI direct access" means `DISTRHO_PLUGIN_WANT_DIRECT_ACCESS` is possible, that is, running DSP and UI on the same process
- "UI remote control" means running the UI on a separate machine (for example over the network)
- An external UI on this table means that it cannot be embed into the host window, but the plugin can still provide one

# Special notes

## Parameter triggers

Trigger-style parameters (parameters which value is reset to its default every run) are only supported in JACK and LV2.  
For all other plugin formats DPF will simulate the behaviour through a parameter change request.

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

## LADSPA RDF

Programs for LADSPA could be done via LRDF but this is not supported in DPF.

## DSSI State

DSSI only supports state changes when called via UI, no "full state" possible.  
This also makes it impossibe to use programs and state at the same time with DSSI,
because in DPF changing programs can lead to state changes but there is no way to fetch this information on DSSI plugins.

To make it simpler to understand, think of DSSI programs and states as UI properties.  
Because in DPF changing the state happens from UI to DSP side, regular DSSI can be supported.  
But if we involve programs, they would need to pass through the UI in order to work. Which goes against DPF's design.

## LV2 parameter changes

Although this is already implemented in DPF (through a custom extension), this is not implemented on most hosts.  
So for now you can pretty much treat it as if not supported.

## VST2 deprecated

Not supported in DPF at the moment.  
It could eventually be, but likely not due to VST2 being phased out by Steinberg.  
Contact DPF authors if you require such a feature.

## VST2 programs

VST2 program support requires saving state of all programs in memory, which is very expensive and thus not done in DPF.

## VST3 CV

Although VST3 officially supports CV (Control Voltage) tagged audio ports,
at the moment no host supports such feature and thus it is not possible to validate it.

## Work in progress

Feature is possible, just not implemented yet in DPF.
