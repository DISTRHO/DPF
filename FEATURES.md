# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.

| Format | Param outs | Programs | States | UI sendNote | CV | MIDI in | MIDI out | Port groups |
|--------|------------|----------|--------|-------------|----|---------|----------|-------------|
| JACK   | Yes* | Yes* | Yes | Yes | Yes | Yes | Yes | Yes* |
| LADSPA | Yes | No* | No | No | No | No | No | No |
| DSSI   | Yes | Yes* | Yes* | Yes | No | Yes | No | No |
| LV2    | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| VST2   | No   | No* | Yes | Yes | No | Yes | Yes | Yes* |
| VST3   | Yes? | Yes | Yes | Yes | NO* | Yes | Yes | No* |

Special notes:

| Format | Notes |
|--------|-------|
| JACK   | Parameters have programs mapped to MIDI CC, there is no generic plugin editor; Port groups as JACK metadata |
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
