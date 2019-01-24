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
/*=============================================================================
 *  AUDebugDispatcher.h
 *  CAServices

=============================================================================*/

#if AU_DEBUG_DISPATCHER

#warning "This should  * * NOT * *  be seen on a Release Version"


#include "AUDebugDispatcher.h"

static char* AUErrorStr (OSStatus result);

class AUDD_Locker {
public:
	AUDD_Locker (CAMutex &guard) :mGuard (guard) { didLock = mGuard.Lock(); }
	~AUDD_Locker () { if (didLock) mGuard.Unlock(); }

private:
	bool 		didLock;
	CAMutex 	&mGuard;
};
	
AUDebugDispatcher::AUDebugDispatcher (AUBase *au, FILE* file)
	: mAUBase (au),
	  mFile (file),
	  mHaveDoneProperty (false),
	  mPrintMutex ("AU Debug Dispatcher Printer"),
	  mHostCB1_Result (0),
	  mHostCB2_Result (0),
	  mHostCB3_Result (0),
	  mHostCB_WhenToPrint (0),
	  mHostCB_WasPlaying (0)
{
		// lets gather some info about this instance...
	AudioComponentDescription desc = mAUBase->GetComponentDescription();
	fprintf (mFile, "\nAUBase=0x%X, Type=%4.4s, SubType=%4.4s, Manu=%4.4s\n\n", AU(),
							(char*)&desc.componentType, 
							(char*)&desc.componentSubType, 
							(char*)&desc.componentManufacturer);
	mFirstTime = CAHostTimeBase::GetCurrentTime();
}


AUDebugDispatcher::~AUDebugDispatcher()
{
	PrintHeaderString (CAHostTimeBase::GetCurrentTime(), (unsigned int)pthread_self(), "Close");
	fprintf (mFile, "\n");
}

void AUDebugDispatcher::PrintHeaderString (UInt64 inNowTime, unsigned int inThread, const char* inMethod)
{
	double secsSinceStart = SecsSinceStart(inNowTime);
	fprintf (mFile, "[AUDisp:AUBase = 0x%X, Time = %.6lf secs, Thread = 0x%X, IsInitialized = '%c'] %s()\n", 
							AU(), secsSinceStart, inThread, (mAUBase->IsInitialized() ? 'T' : 'F'), inMethod);
}

unsigned int	AUDebugDispatcher::RecordDispatch (UInt64 inStartTime, OSStatus result, const char* inMethod)
{
	UInt64 nowTime = CAHostTimeBase::GetCurrentTime();
	
	unsigned int theThread = (unsigned int)pthread_self();
	
	PrintHeaderString (nowTime, theThread, inMethod);
	
	UInt64 nanos = CAHostTimeBase::ConvertToNanos(nowTime - inStartTime);

	fprintf (mFile, "\t[Time To execute = %.6lf secs", (nanos * 1.0e-9));

	if (result)
		fprintf (mFile, ", * * * result = %ld, %s * * * ", result, AUErrorStr(result));

	fprintf (mFile, "]\n");

	return theThread;
}

#pragma mark -
#pragma mark __AU Dispatch
#pragma mark -

void		AUDebugDispatcher::Initialize (UInt64 nowTime, OSStatus result)
{
	AUDD_Locker lock (mPrintMutex);
	RecordDispatch (nowTime, result, "Initialize");
}

void		AUDebugDispatcher::Uninitialize (UInt64 nowTime, OSStatus result)
{
	AUDD_Locker lock (mPrintMutex);
	RecordDispatch (nowTime, result, "Uninitialize");
}

void		AUDebugDispatcher::GetPropertyInfo (UInt64 					nowTime,
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										UInt32							*outDataSize,
										Boolean							*outWritable)
{
	AUDD_Locker lock (mPrintMutex);
	RecordDispatch (nowTime, result, "GetPropertyInfo");
	PrintProperty (inID, inScope, inElement);
}

void		AUDebugDispatcher::GetProperty (UInt64 						nowTime, 
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										UInt32							*ioDataSize,
										void							*outData)
{
	//err -> ioDataSize == NULL or 0
	//outData == NULL -> Dispatches to GetPropertyInfo
	
	// still should log these as calls to GetProperty...
	AUDD_Locker lock (mPrintMutex);
	const char *dispStr = outData != NULL ? "GetProperty" : "GetProperty - Info";
	RecordDispatch (nowTime, result, dispStr);
	PrintProperty (inID, inScope, inElement);
}

void		AUDebugDispatcher::SetProperty (UInt64 						nowTime, 
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										const void *					inData,
										UInt32 							inDataSize)
{
	// inData could be NULL to remove property value...
	AUDD_Locker lock (mPrintMutex);
	RecordDispatch (nowTime, result, "SetProperty");
	PrintProperty (inID, inScope, inElement);
}

void		AUDebugDispatcher::Render (	UInt64 							nowTime, 
										OSStatus 						result,
										AudioUnitRenderActionFlags *	inRenderFlags,
										const AudioTimeStamp *			inTimeStamp,
										UInt32							inOutputBusNumber,
										UInt32							inNumFrames,
										AudioBufferList *				inIOData)
{
	if (mHaveDoneProperty) {
		AUDD_Locker lock (mPrintMutex);
		RecordDispatch (nowTime, result, "Render");
		fprintf (mFile, "\t\t[Sample Time = %.0lf, NumFrames = %ld]\n", inTimeStamp->mSampleTime, inNumFrames);
		mHaveDoneProperty = false;
	}
	RenderActions_HostCallbacks ();
}

#define kBeginLine "         "

void	AUDebugDispatcher::RenderActions_HostCallbacks ()
{
	bool doPrint = !(mHostCB_WhenToPrint++ % 5000);
	
// (3) Host Transport State callback 
//		- this is printed below, but we use the transStateChange to see if we have something to print
	Boolean isPlaying, transStateChange, isCycling;
	Float64 currentSample, cycleStartBeat, cycleEndBeat;
	OSStatus result = mAUBase->CallHostTransportState (&isPlaying, &transStateChange, &currentSample,
													&isCycling, &cycleStartBeat, &cycleEndBeat);
	bool newCB3Result = false;
	if (result != mHostCB3_Result) {
		mHostCB3_Result = result;
		newCB3Result = true;
	}

	if (transStateChange) mHostCB_WhenToPrint = 1;
	
// Code to test the Host Callbacks
	Float64 currentBeat, currentTempo;
	
// (1) Beat and Tempo callback
	result = mAUBase->CallHostBeatAndTempo (&currentBeat, &currentTempo);
	
	if (result) {
		if (result != mHostCB1_Result) {
			printf ("_HCback_ Error Calling HostBeatAndTempo:%ld\n", result);
			mHostCB1_Result = result;
		}
	} else {
		if (doPrint || currentBeat < 0 || transStateChange) 
			printf ("_HCback_ Beat and Tempo: Current Beat:%f, Current Tempo:%f\n", currentBeat, currentTempo);
	}
	
// (2) Musical Time callback
	UInt32 deltaSampleOffset, timeSig_Denom;
	Float32 timeSig_Num;
	Float64 currentMeasureDownBeat;
	result = mAUBase->CallHostMusicalTimeLocation (&deltaSampleOffset, &timeSig_Num, &timeSig_Denom, &currentMeasureDownBeat);

	if (result) {
		if (result != mHostCB2_Result) {
			printf ("%sError Calling CallHostMusicalTimeLocation:%ld\n", kBeginLine, result);
			mHostCB2_Result = result;
		}
	} else {
		if (doPrint || currentMeasureDownBeat < 0 || deltaSampleOffset < 0 || transStateChange) {
			printf ("%sMusical Time: Delta Sample Offset:%ld, Time Sig:Num:%.1f, Time Sig:Denom:%ld, DownBeat:%f\n", 
						kBeginLine, deltaSampleOffset, timeSig_Num, timeSig_Denom, currentMeasureDownBeat);
		}
	}
	
	if (mHostCB3_Result) {
		if (newCB3Result) {
			printf ("%sError Calling CallHostTransportState:%ld\n", kBeginLine, mHostCB3_Result);
		}
	} else {
		if (doPrint || (mHostCB_WasPlaying != isPlaying) || transStateChange || currentSample < 0) 
		{			
			printf ("%sTransport State: Was Playing:%d, ", kBeginLine, mHostCB_WasPlaying);
			mHostCB_WasPlaying = isPlaying;
			printf ("Is Playing:%d, Transport State Changed:%d", isPlaying, transStateChange);
			if (isPlaying) {
				printf (", Current Sample:%.1f", currentSample);
				if (isCycling)
					printf (", Is Cycling [Start Beat:%.2f, End Beat:%.2f]", cycleStartBeat, cycleEndBeat);
			}
			printf ("\n");
		}
	}
}

#pragma mark -
#pragma mark __Utilities
#pragma mark -

static char* AUScopeStr (AudioUnitScope inScope)
{
	switch (inScope) {
		case kAudioUnitScope_Global: return "Global";
		case kAudioUnitScope_Output: return "Output";
		case kAudioUnitScope_Input: return "Input";
		case kAudioUnitScope_Group: return "Group";
	}
	return NULL;
}

static char* AUErrorStr (OSStatus result)
{
	switch (result) {
		case kAudioUnitErr_InvalidProperty: return "Invalid Property";
		case kAudioUnitErr_InvalidParameter: return "Invalid Parameter";
		case kAudioUnitErr_InvalidElement: return "Invalid Element";
		case kAudioUnitErr_NoConnection: return "Invalid Connection";
		case kAudioUnitErr_FailedInitialization: return "Failed Initialization";
		case kAudioUnitErr_TooManyFramesToProcess: return "Too Many Frames";
		case kAudioUnitErr_IllegalInstrument: return "Illegal Instrument";
		case kAudioUnitErr_InstrumentTypeNotFound: return "Instrument Type Not Found";
		case kAudioUnitErr_InvalidFile: return "Invalid File";
		case kAudioUnitErr_UnknownFileType: return "Unknown File Type";
		case kAudioUnitErr_FileNotSpecified: return "File Not Specified";
		case kAudioUnitErr_FormatNotSupported: return "Format Not Supported";
		case kAudioUnitErr_Uninitialized: return "Un Initialized";
		case kAudioUnitErr_InvalidScope: return "Invalid Scope";
		case kAudioUnitErr_PropertyNotWritable: return "Property Not Writable";
		case kAudioUnitErr_InvalidPropertyValue: return "Invalid Property Value";
		case kAudioUnitErr_PropertyNotInUse: return "Property Not In Use";
		case kAudioUnitErr_Initialized: return "Initialized";

	// some common system errors
		case badComponentSelector: return "Bad Component Selector";
		case paramErr: return "Parameter Error";
		case badComponentInstance: return "Bad Component Instance";
	}
	return "Unknown Error";
}

static char* AUPropertyStr (AudioUnitPropertyID inID)
{
	switch (inID) {
		case kAudioUnitProperty_ClassInfo: return "Class Info";
		case kAudioUnitProperty_MakeConnection: return "Connection";
		case kAudioUnitProperty_SampleRate: return "Sample Rate";
		case kAudioUnitProperty_ParameterList: return "Parameter List";
		case kAudioUnitProperty_ParameterInfo: return "Parameter Info";
		case kAudioUnitProperty_FastDispatch: return "Fast Dispatch";
		case kAudioUnitProperty_CPULoad: return "CPU Load";
		case kAudioUnitProperty_StreamFormat: return "Format";
		case kAudioUnitProperty_ReverbRoomType: return "Reverb Room Type";
		case kAudioUnitProperty_ElementCount: return "Element Count";
		case kAudioUnitProperty_Latency: return "Latency";
		case kAudioUnitProperty_SupportedNumChannels: return "Supported Num Channels";
		case kAudioUnitProperty_MaximumFramesPerSlice: return "Max Frames Per Slice";
		case kAudioUnitProperty_SetExternalBuffer: return "Set External Buffer";
		case kAudioUnitProperty_ParameterValueStrings: return "Parameter Value Strings";
		case kAudioUnitProperty_GetUIComponentList: return "Carbon UI";
		case kAudioUnitProperty_AudioChannelLayout: return "Audio Channel Layout";  
		case kAudioUnitProperty_TailTime: return "Tail Time";
		case kAudioUnitProperty_BypassEffect: return "Bypass Effect";
		case kAudioUnitProperty_LastRenderError: return "Last Render Error";
		case kAudioUnitProperty_SetRenderCallback: return "Render Callback";
		case kAudioUnitProperty_FactoryPresets: return "Factory Preset";
		case kAudioUnitProperty_ContextName: return "Context Name";
		case kAudioUnitProperty_RenderQuality: return "Render Quality";
		case kAudioUnitProperty_HostCallbacks: return "Host Callbacks";
		case kAudioUnitProperty_InPlaceProcessing: return "In Place Processing";
		case kAudioUnitProperty_ElementName: return "Element Name";
		case kAudioUnitProperty_CocoaUI: return "Cocoa UI";
		case kAudioUnitProperty_SupportedChannelLayoutTags: return "Supported Channel Layout Tags";
		case kAudioUnitProperty_ParameterStringFromValue: return "Parameter Value Name";
		case kAudioUnitProperty_UsesInternalReverb: return "Use Internal Reverb";
		case kAudioUnitProperty_ParameterIDName: return "Parameter ID Name";
		case kAudioUnitProperty_ParameterClumpName: return "Clump Name";
		case kAudioUnitProperty_PresentPreset: return "Present Preset";

	
		case kMusicDeviceProperty_InstrumentCount: return "Instrument Count";
		case kMusicDeviceProperty_InstrumentName: return "Instrument Name";
		case kMusicDeviceProperty_SoundBankFSRef: return "Sound Bank - File";
		case kMusicDeviceProperty_InstrumentNumber: return "Instrument Number";
		case kMusicDeviceProperty_MIDIXMLNames: return "MIDI XML Names";
		case kMusicDeviceProperty_BankName: return "Bank Name";
		case kMusicDeviceProperty_SoundBankData: return "Sound Bank - Data";
		
		
		case kAudioOutputUnitProperty_CurrentDevice: return "Current AudioDevice";
		case kAudioOutputUnitProperty_IsRunning: return "Is Running";
		case kAudioOutputUnitProperty_ChannelMap: return "Channel Map";
		case kAudioOutputUnitProperty_EnableIO: return "Enable I/O";
		case kAudioOutputUnitProperty_StartTime: return "Start Time";
		case kAudioOutputUnitProperty_SetInputCallback: return "I/O Input Callback";
	}
	return "Unknown";
}

	
void 		AUDebugDispatcher::PrintProperty ( AudioUnitPropertyID 	inID,
											AudioUnitScope 			inScope,
											AudioUnitElement 		inElement)
{										
	char* scopeStr = AUScopeStr(inScope);
	char* propStr = AUPropertyStr (inID);
	
	if (scopeStr != NULL)
		fprintf (mFile, "\t\t[ID = %ld, %s, Scope = %s, El = %ld]\n", inID, propStr, scopeStr, inElement);
	else
		fprintf (mFile, "\t\t[ID = %ld, %s, Scope = %ld, El = %ld]\n", inID, propStr, inScope , inElement);
	
	bool iscback = false;
	bool isInput = false;
	switch (inID) 
	{
		case kAudioUnitProperty_SetRenderCallback:
			iscback = true;
		case kAudioUnitProperty_MakeConnection:
		{
			AUInputElement *el = mAUBase->GetInput (inElement);
			if (el) {
				bool hasInput = false;
				if (iscback)
					hasInput = el->IsCallback();
				else
					hasInput = el->HasConnection();
				fprintf (mFile, "\t\tHas Input=%c, ", (hasInput ? 'T' : 'F'));
				isInput = true;
			}
		}
		case kAudioUnitProperty_SampleRate:
		case kAudioUnitProperty_StreamFormat:
		{
			CAStreamBasicDescription desc = mAUBase->GetStreamFormat (inScope, inElement);
			if (!isInput)
				fprintf (mFile, "\t\t");
			desc.Print (mFile);
			break;
		}
		default:
			break;
	}
	
	mHaveDoneProperty = true;
}

#endif //AU_DEBUG_DISPATCHER
