/*	Copyright © 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
#include "ComponentBase.h"
#include "CAXException.h"

#if TARGET_OS_MAC
pthread_mutex_t ComponentInitLocker::sComponentOpenMutex = PTHREAD_MUTEX_INITIALIZER;
#elif TARGET_OS_WIN32
CAGuard	ComponentInitLocker::sComponentOpenGuard("sComponentOpenGuard");
#endif

#if CA_DO_NOT_USE_AUDIO_COMPONENT
	#include <dlfcn.h>
#endif

ComponentBase::~ComponentBase()
{
}

void			ComponentBase::PostConstructor()
{
}

void			ComponentBase::PreDestructor()
{
}

#if !TARGET_OS_IPHONE
OSStatus		ComponentBase::Version()
{
	return 0x00000001;
}

OSStatus		ComponentBase::ComponentEntryDispatch(ComponentParameters *p, ComponentBase *This)
{
	if (This == NULL) return paramErr;

	OSStatus result = noErr;
	
	switch (p->what) {
	case kComponentCloseSelect:
		This->PreDestructor();
		delete This;
		break;
	
	case kComponentVersionSelect:
		result = This->Version();
		break;

	case kComponentCanDoSelect:
		switch (p->params[0]) {
		case kComponentOpenSelect:
		case kComponentCloseSelect:
		case kComponentVersionSelect:
		case kComponentCanDoSelect:
			return 1;
		default:
			return 0;
		}
		
	default:
		result = badComponentSelector;
		break;
	}
	return result;
}
#endif

#if CA_DO_NOT_USE_AUDIO_COMPONENT 
static OSStatus ComponentBase_GetComponentDescription (const AudioComponentInstance & inInstance, AudioComponentDescription &outDesc);
#endif

AudioComponentDescription ComponentBase::GetComponentDescription() const
{
	AudioComponentDescription desc;
	OSStatus result;
	
#if CA_DO_NOT_USE_AUDIO_COMPONENT
	ca_require_noerr (result = ComponentBase_GetComponentDescription (mComponentInstance, desc), home);
#else
	AudioComponent comp = AudioComponentInstanceGetComponent(mComponentInstance);
	XAssert (comp);
	if (comp) {
		ca_require_noerr(result = AudioComponentGetDescription(comp, &desc), home);
	} else
		ca_require_noerr(result = -1, home);
#endif

home:
	if (result)
		memset (&desc, 0, sizeof(AudioComponentDescription));
	return desc;
}


#if CA_DO_NOT_USE_AUDIO_COMPONENT 
OSStatus ComponentBase_GetComponentDescription (const AudioComponentInstance & inInstance, AudioComponentDescription & outDesc)
{
		// we prefer to use the new API. If it is not available however, we have to go back to using the ComponentMgr one. 
	typedef AudioComponent (*AudioComponentInstanceGetComponentProc) (AudioComponentInstance);
	static AudioComponentInstanceGetComponentProc aciGCProc = NULL;
	
	typedef OSStatus (*AudioComponentGetDescriptionProc)(AudioComponent, AudioComponentDescription *);
	static AudioComponentGetDescriptionProc acGDProc = NULL;
	
	typedef OSErr (*GetComponentInfoProc)(Component, ComponentDescription *, void*, void*, void*);
	static GetComponentInfoProc gciProc = NULL;

	static int doneInit = 0;
	if (doneInit == 0) {
		doneInit = 1;
		bool loadCMgr = true;
		
		void* theImage = dlopen("/System/Library/Frameworks/AudioUnit.framework/AudioUnit", RTLD_LAZY);
		if (theImage != NULL)
		{
			//	we assume that all routine names passed here have a leading underscore which gets shaved
			//	off when passed to dlsym
			aciGCProc = (AudioComponentInstanceGetComponentProc)dlsym (theImage, "AudioComponentInstanceGetComponent");
			if (aciGCProc) {
				acGDProc = (AudioComponentGetDescriptionProc)dlsym (theImage, "AudioComponentGetDescription");
				if (acGDProc)
					loadCMgr = false;
			}
		}
		if (loadCMgr) {
			theImage = dlopen("/System/Library/Frameworks/CoreServices.framework/CoreServices", RTLD_LAZY);
			if (theImage != NULL)
			{	
				gciProc = (GetComponentInfoProc)dlsym (theImage, "GetComponentInfo");
			}
		}
	}
	
	OSStatus result = noErr;
	if (acGDProc && aciGCProc) {
		AudioComponent comp = (*aciGCProc)(inInstance);
		XAssert (comp);
		if (comp) {
			ca_require_noerr(result = (*acGDProc)(comp, &outDesc), home);
		} else
			ca_require_noerr(result = -1, home);
	
	} else if (gciProc) {
		// in this case we know that inInstance is directly castable to a Component
		ca_require_noerr(result = (*gciProc)((Component)inInstance, (ComponentDescription*)&outDesc, NULL, NULL, NULL), home);
	}
home:
	return result;
}
#endif //CA_DO_NOT_USE_AUDIO_COMPONENT
