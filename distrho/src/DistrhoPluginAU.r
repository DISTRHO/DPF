#define UseExtendedThingResource 1
#include <AudioUnit/AudioUnit.r>

#include "DistrhoPluginInfo.h"

#ifdef DEBUG
    #define VERSION 0xFFFFFFFF
#else
    #define VERSION 0x00010000  
#endif

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define kAudioUnitResID_dpfplugin 1000

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ dpfplugin~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define RES_ID          kAudioUnitResID_dpfplugin
#define COMP_TYPE       kAudioUnitType_Effect
#define COMP_SUBTYPE    'FuCk'
#define COMP_MANUF      'ShIt' 

#define NAME            DISTRHO_PLUGIN_BRAND ": " DISTRHO_PLUGIN_NAME
#define DESCRIPTION     DISTRHO_PLUGIN_NAME " AU"
#define ENTRY_POINT     "PluginAUEntry"

#include "AUResources.r"
