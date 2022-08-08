# DPF - DISTRHO Plugin Framework

Even though DPF is quite liberally licensed, not all plugin formats follow the same ideals.  
This is usually due to plugin APIs/headers being tied to a specific license or having commercial restrictions.  
This file describes the licensing that applies to each individual plugin format as a way to make it clear what is possible and compatible.  
Note that if you are making GPLv2+ licensed plugins this does not apply to you, as so far everything is GPLv2+ compatible.

Regardless of target format, DPF itself needs to be mentioned in attribution.
See the [LICENSE](LICENSE) file for copyright details.

| Target          | License(s)           | License restrictions  | Additional attribution |
|-----------------|----------------------|-----------------------|------------------------|
| JACK/Standalone | MIT (RtAudio)        | Copyright attribution | **RtAudio**: 2001-2019 Gary P. Scavone |
| LADSPA          | LGPLv2.1+            | ??? (*)               | 2000-2002 Richard W. E. Furse, Paul Barton-Davis, Stefan Westerfeld |
| DSSI            | LGPLv2.1+            | ??? (*)               | **DSSI**: 2004, 2009 Chris Cannam, Steve Harris and Sean Bolton;<br/> **ALSA**: 1998-2001 Jaroslav Kysela, Abramo Bagnara, Takashi Iwai |
| LV2             | ISC                  | Copyright attribution | 2006-2020 Steve Harris, David Robillard;<br/> 2000-2002 Richard W.E. Furse, Paul Barton-Davis, Stefan Westerfeld |
| VST2            | GPLv2+ or commercial | Must be GPLv2+ compatible or alternatively use Steinberg VST2 SDK (no longer available for new plugins) | GPLv2+ compatible license or custom agreement with Steinberg |
| VST3            | ISC                  | Copyright attribution | (none, only DPF files used) |

### LADSPA and DSSI special note

The header files on LADSPA and DSSI are LGPLv2.1+ licensed, which is unusual for pure APIs without libraries.  
LADSPA authors mention this on ladspa.org homepage:

> LADSPA has been released under LGPL (GNU Lesser General Public License).
> This is not intended to be the final license for LADSPA.
> In the long term it is hoped that LADSPA will have a public license that is even less restrictive, so that commercial applications can use it (in a protected way) without having to use a derived LGPL library.
> It may be that LGPL is already free enough for this, but we aren't sure.

So the situation for LADSPA/DSSI plugins is unclear for commercial plugins.  
These formats are very limited and not much used anymore anyway, feel free to skip them if this situation is a potential issue for you.

### VST2 special note

By default DPF uses the free reverse-engineered [vestige header](distrho/src/vestige/vestige.h) file.  
This file is GPLv2+ licensed, so that applies to plugins built with it as well.  
You can alternatively build DPF-based VST2 plugins using the official Steinberg VST2 SDK,
simply set the `VESTIGE_HEADER` compiler macro to `0` during build.  
You will need to provide your own VST2 SDK files then, as DPF does not ship with them.  
Note there are legal issues surrounding releasing new VST2 plugins using the official SDK, as that is no longer supported by Steinberg.

### VST3 special note

Contrary to most plugins, DPF does not use the official VST3 SDK.  
Instead, the API definitions are provided by the [travesty](distrho/src/travesty/) sub-project, licensed in the same way as DPF.
This allows us to freely build plugins without being encumbered by restrictive licensing deals.  
It makes the internal implementation much harder for DPF, but this is not an issue for external developers.
