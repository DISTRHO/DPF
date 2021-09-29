# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.

| Feature           | JACK/Standalone | LADSPA | DSSI | LV2           | VST2   | VST3   |
|-------------------|-----------------|--------|------|---------------|--------|--------|
| Audio port groups | [Yes*](FEATURES.md#jack-parameters-and-programs)            | No     | No   | Yes | No   | No*  |
| Audio port as CV  | Yes             | No     | No   | Yes           | No     | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| MIDI input        | Yes             | No     | Yes  | Yes           | Yes    | Yes    |
| MIDI output       | Yes             | No     | No   | Yes           | Yes    | Yes    |
| Parameter changes | Yes             | No     | No   | [No*](FEATURES.md#lv2-parameter-changes)         | Yes    | Yes    |
| Parameter groups  | No              | No     | No   | Yes           | Yes*   | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| Parameter outs    | No              | No     | No   | Yes           | Yes    | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| Programs          | Yes*            | No*    | Yes* | Yes           | No*    | Yes    |
| States            | Yes             | No     | Yes* | Yes           | Yes    | Yes    |
| UI                | [Yes*](FEATURES.md#jack-custom-ui-only)            | No     | Ext. | Embed or Ext. | Embed  | Embed  |
| UI host-resize    | Yes             | No     | Yes  | Yes           | No     | [No*](FEATURES.md#vst3-is-work-in-progress)  |
| UI sendNote       | Yes             | No     | Yes  | Yes           | Yes    | Yes    |

# Special notes

## JACK parameters and programs

Under JACK/Stanlone mode, MIDI input events will trigger program and parameter changes.  
MIDI program change events work as expected (that is, MIDI program change 0 will load 1st plugin program).  
MIDI CCs are used for parameter changes (matching the `midiCC` value you set on each parameter).

## JACK custom UI only

There is no generic plugin editor view.  
If your plugin has no custom UI, the standalone executable will run but not show any window.

## LV2 parameter changes

Possible through a custom extension, not implemented on most hosts.  
For now you can pretty much assume it is not supported.

## VST3 is work in progress

Feature is possible, just not implemented yet in DPF.

### TODO

A few notes for things to add to the table:

 - Sidechain tagged audio ports
 - Trigger parameters
 - UI background/foreground color
 - Host-side file browser
 - Remote/Instance access
 - Host-mapped bypass parameter
 - Time position

# Extra notes

| Format | Notes |
|--------|-------|
| JACK   | Parameters and programs are mapped to MIDI events, ; Audio port groups are set as JACK metadata |
| LADSPA | LADSPA only supports basic parameters and audio;<br/> Programs could be done via LRDF but not supported in DPF |
| DSSI   | DSSI only supports States via UI, no "full state" possible |
| LV2    | Everything supported :) |
| VST2   | VST2 program support requires saving state of all programs in memory, which is very expensive and thus not done in DPF;<br/> Parameter groups are supported, but not for audio ports (per VST2 spec limitations) |
| VST3   | Not sure if parameter outputs work (aka "read-only" on VST3);<br/> CV ports do not support custom ranges, not implemented yet;<br/> Port groups not implemented yet |
