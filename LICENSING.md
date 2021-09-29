# DPF - DISTRHO Plugin Framework

Even though DPF is quite liberally licensed, not all plugin formats follow the same ideals.  
This usually due to plugin APIs/headers being tied to a specific license or having commercial restrictions.  
This file describes the licensing that applies to each individual plugin format as a way to make it clear what is possible and compatible.  
Note that if you are making GPLv2+ licensed plugins this does not apply to you, as so far everything is GPLv2+ compatible.

Target | License(s) | License restrictions | Attribution
JACK/Standalone | ISC (JACK bridge methods) + ?? (RtAudio) | Copyright attribution | RtAudio: 2001-2019 Gary P. Scavone
LADSPA | LGPLv2.1+ | ??? | 2000-2002 Richard W. E. Furse, Paul Barton-Davis, Stefan Westerfeld
DSSI | LGPLv2.1+ | ??? | ALSA: 1998-2001 Jaroslav Kysela, Abramo Bagnara, Takashi Iwai; DSSI: 2004, 2009 Chris Cannam, Steve Harris and Sean Bolton
LV2 | ISC + GPL? (atom-helpers.h not sure if used) | Copyright attribution | TODO, check all headers used
VST2 | GPLv2+ or commercial* | Must also be GPLv2+ or alternatively use Steingberg VST2 SDK (no longer available for new plugins) | Same or later license if GPL; Alternatively, custom agreement with Steingberg
VST3 | ISC | Copyright attribution

Regardless of target format, DPF itself needs to be mentioned in attribution. See LICENSE file for copyright details.
