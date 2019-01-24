#define UseExtendedThingResource 1
#include <AudioUnit/AudioUnit.r>

#include "DistrhoPluginInfo.h"
#include "DistrhoPluginInfo.r"

#ifndef DISTRHO_PLUGIN_AU_TYPE
 #define DISTRHO_PLUGIN_AU_TYPE kAudioUnitType_Effect
#endif

#ifndef DISTRHO_PLUGIN_AU_SUBTYPE
 #define DISTRHO_PLUGIN_AU_SUBTYPE '????'
#endif

#ifndef DISTRHO_PLUGIN_AU_MANUF
 #define DISTRHO_PLUGIN_AU_MANUF '????'
#endif

#ifndef DISTRHO_PLUGIN_AU_RES_ID
 #define DISTRHO_PLUGIN_AU_RES_ID 1000
#endif

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define RES_ID          DISTRHO_PLUGIN_AU_RES_ID

#define COMP_TYPE       DISTRHO_PLUGIN_AU_TYPE
#define COMP_SUBTYPE    DISTRHO_PLUGIN_AU_SUBTYPE
#define COMP_MANUF      DISTRHO_PLUGIN_AU_MANUF

#define NAME            DISTRHO_PLUGIN_FULL_NAME
#define DESCRIPTION     DISTRHO_PLUGIN_DESCRIPTION
#define VERSION         DISTRHO_PLUGIN_VERSION
#define ENTRY_POINT     "PluginAUEntry"

#include "AUResources.r"
