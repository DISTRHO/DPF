# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.

| Feature     | JACK/Standalone | LADSPA | DSSI | LV2 | VST2 | VST3 |
|-------------|-----------------|--------|------|-----|------|------|
| Param outs  | Yes*            | Yes    | Yes  | Yes | No   | Yes? |
| Programs    | Yes*            | No*    | Yes* | Yes | No*  | Yes  |
| States      | Yes             | No     | Yes* | Yes | Yes  | Yes  |
| UI sendNote | Yes             | No     | Yes  | Yes | Yes  | Yes  |
| CV          | Yes             | No     | No   | Yes | No   | No*  |
| MIDI in     | Yes             | No     | Yes  | Yes | Yes  | Yes  |
| MIDI out    | Yes             | No     | No   | Yes | Yes  | Yes  |
| Port groups | Yes*            | No     | No   | Yes | Yes* | No*  |

Special notes:

| Format | Notes |
|--------|-------|
| JACK   | Parameters and programs are mapped to MIDI events, there is no generic plugin editor; Audio port groups are set as JACK metadata |
| LADSPA | LADSPA only supports basic parameters and audio;<br/> Programs could be done via LRDF but not supported in DPF |
| DSSI   | DSSI only supports States via UI, no "full state" possible |
| LV2    | Everything supported :) |
| VST2   | VST2 program support requires saving state of all programs in memory, which is very expensive and thus not done in DPF;<br/> Parameter groups are supported, but not for audio ports (per VST2 spec limitations) |
| VST3   | Not sure if parameter outputs work (aka "read-only" on VST3);<br/> CV ports do not support custom ranges, not implemented yet;<br/> Port groups not implemented yet |

A few notes for things to add to the table:

 - Custom UI (embed, external, both or none)
 - Host/user-side UI resize
 - Parameter inputs change from plugin-side
 - Sidechain tagged audio ports
 - Trigger parameters
 - UI background/foreground color
 - Host-side file browser
 - Remote/Instance access
 - Host-mapped bypass parameter
 - Time position
