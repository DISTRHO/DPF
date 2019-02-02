#include <Cocoa/Cocoa.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AUCocoaUIView.h>

#define MAX_PARAMS 100

@interface PluginAU_CocoaUI : NSView
{
    AudioUnit mAU;
    AudioUnitParameter mParameter[MAX_PARAMS];
    AUParameterListenerRef mParameterListener;
    UInt32 paramCount;
}

#pragma mark ____ PUBLIC FUNCTIONS ____
- (void)setAU:(AudioUnit)inAU;

#pragma mark ____ PRIVATE FUNCTIONS
- (void)_synchronizeUIWithParameterValues;
- (void)_addListeners;
- (void)_removeListeners;

#pragma mark ____ LISTENER CALLBACK DISPATCHEE ____
- (void)_parameterListener:(void *)inObject
      parameter:(const AudioUnitParameter *)inParameter
      value:(Float32)inValue;

@end


@implementation PluginAU_CocoaUI

#pragma mark ____ (INIT /) DEALLOC ____
- (void)dealloc {
    [self _removeListeners];
    [super dealloc];
}

#pragma mark ____ PUBLIC FUNCTIONS ____
- (void)setAU:(AudioUnit)inAU {
    // remove previous listeners
    if (mAU) [self _removeListeners];
    mAU = inAU;

    paramCount = 1; //globalUI->dspPtr.getParameterCount();

    UInt32 i;
    for (i = 0; i < paramCount; ++i) {
        mParameter[i].mAudioUnit = inAU;
        mParameter[i].mParameterID = i;
        mParameter[i].mScope = kAudioUnitScope_Global;
        mParameter[i].mElement = 0;
    }

    //auUI* dspUI = [self dspUI];

    // add new listeners
    [self _addListeners];

    // initial setup
    [self _synchronizeUIWithParameterValues];

}

#pragma mark ____ LISTENER CALLBACK DISPATCHER ____
void ParameterListenerDispatcher (void *inRefCon, void *inObject,
                                  const AudioUnitParameter *inParameter,
                                  Float32 inValue)
{
    PluginAU_CocoaUI *SELF = (PluginAU_CocoaUI *)inRefCon;

    [SELF _parameterListener:inObject parameter:inParameter value:inValue];
}

#pragma mark ____ PRIVATE FUNCTIONS ____
- (void)_addListeners {
    NSAssert (AUListenerCreate(ParameterListenerDispatcher, self,
              CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.100, // 100 ms
              &mParameterListener ) == noErr,
              @"[_addListeners] AUListenerCreate()");

    UInt32 i;
    for (i = 0; i < paramCount; ++i) {
        NSAssert(AUListenerAddParameter(mParameterListener,
                                        NULL, &mParameter[i]) == noErr,
                 @"[_addListeners] AUListenerAddParameter()");
    }
}

- (void)_removeListeners {
    UInt32 i;
    for (i = 0; i < paramCount; ++i) {
        NSAssert (AUListenerRemoveParameter(mParameterListener,
                                            NULL, &mParameter[i]) == noErr,
                  @"[_removeListeners] AUListenerRemoveParameter()");
    }

    NSAssert (AUListenerDispose(mParameterListener) == noErr,
              @"[_removeListeners] AUListenerDispose()");
}

- (void)_synchronizeUIWithParameterValues {
    Float32 value;
    UInt32 i;

    for (i = 0; i < paramCount; ++i) {
        // only has global parameters
        NSAssert (AudioUnitGetParameter(mAU, mParameter[i].mParameterID,
                    kAudioUnitScope_Global, 0, &value) == noErr,
                    @"[synchronizeUIWithParameterValues] (x.1)");
        NSAssert (AUParameterSet(mParameterListener, self, &mParameter[i],
	                         value, 0) == noErr,
                    @"[synchronizeUIWithParameterValues] (x.2)");
        NSAssert (AUParameterListenerNotify (mParameterListener, self,
	                                     &mParameter[i]) == noErr,
                    @"[synchronizeUIWithParameterValues] (x.3)");
    }
}

#pragma mark ____ LISTENER CALLBACK DISPATCHEE ____
- (void)_parameterListener:(void *)inObject
         parameter: (const AudioUnitParameter *)inParameter
         value: (Float32)inValue
{
    //DPF: setUIParameter(inParameter->mParameterID, inValue);
}

@end

@interface PluginAU_CocoaUIFactory : NSObject <AUCocoaUIBase>
{
    IBOutlet PluginAU_CocoaUI *uiFreshlyLoadedView;	
}

- (NSString *) description;	

@end

@implementation PluginAU_CocoaUIFactory

- (unsigned) interfaceVersion {
	return 0;
}

- (NSString *) description {
	return @"DPF: AU UI";
}

- (NSView *)uiViewForAudioUnit:(AudioUnit)inAU withSize:(NSSize)inPreferredSize {
    PluginAU_CocoaUI *view = [[PluginAU_CocoaUI alloc] init];
    [view setAU:inAU];
    return [view autorelease];
}

@end
