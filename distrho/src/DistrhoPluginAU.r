#define UseExtendedThingResource 1
#include <AudioUnit/AudioUnit.r>

#include "DistrhoPluginInfo.h"

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define kAudioUnitResID_dpfplugin 1000

#define RES_ID       kAudioUnitResID_dpfplugin
#define COMP_TYPE    kAudioUnitType_Effect
#define COMP_SUBTYPE "Damn"
#define COMP_MANUF   "????"

#define VERSION      0x00010000

#ifdef DISTRHO_PLUGIN_BRAND
 #define NAME        DISTRHO_PLUGIN_BRAND ": " DISTRHO_PLUGIN_NAME
#else
 #define NAME        "DPF: " DISTRHO_PLUGIN_NAME
#endif

#define DESCRIPTION  "TODO here"
#define ENTRY_POINT  "PluginAUEntry"

#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUResources.r"

