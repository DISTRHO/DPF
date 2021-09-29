# DPF - DISTRHO Plugin Framework

This file describes the available features for each plugin format.  
The limitations could be due to the plugin format itself or within DPF.

| Format | Audio IO | Parameters | Parameter outputs | Programs | States | UI->DSP sendNote | CV ports | MIDI input | MIDI output | Port groups | Special notes |
|--------|----------|------------|-------------------|----------|--------|------------------|----------|------------|-------------|-------------|---------------|
| JACK/Standalone | Yes | Yes* | Yes* | Yes* | Yes | Yes | Yes | Yes | Yes | Yes* | Parameters have programs mapped to MIDI CC, there is no generic plugin editor; Port groups as JACK metadata |
| LADSPA | Yes | Yes | Yes | No* | No | No | No | No | No | No | LADSPA only supports basic parameters and audio; Programs could be done via LRDF but not supported in DPF |
| DSSI | Yes | Yes | Yes | Yes* | Yes* | Yes | No | Yes | No | No | DSSI only supports States via UI, no "full state" possible |
| LV2 | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Everything supported :) |
| VST2 | Yes | Yes | No | No* | Yes | Yes | No | Yes | Yes | Yes* | VST2 program support requires saving state of all programs in memory, which is very expensive and thus not done in DPF; Parameter groups are supported, but not for audio ports (per VST2 spec limitations) |
| VST3 | Yes | Yes | Yes? | Yes | Yes | Yes | NO* | Yes | Yes | No* | Not sure if parameter outputs work (aka "read-only" on VST3); CV ports do not support custom ranges, not implemented yet; Port groups not implemented yet |

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
