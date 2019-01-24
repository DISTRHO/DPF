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

#ifndef __AUDebugDispatcher_h__
#define __AUDebugDispatcher_h__

#if AU_DEBUG_DISPATCHER

#include "CAHostTimeBase.h"
#include "CAMutex.h"
#include "AUBase.h"


class AUDebugDispatcher {
public:
						AUDebugDispatcher (AUBase *au, FILE* file = stdout);
						~AUDebugDispatcher();

// these are the AU API calls
		void			Initialize (UInt64 nowTime, OSStatus result);

		void			Uninitialize (UInt64 nowTime, OSStatus result);

		void			GetPropertyInfo (UInt64 						nowTime, 
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										UInt32							*outDataSize,
										Boolean							*outWritable);

		void			GetProperty (	UInt64 							nowTime, 
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										UInt32							*ioDataSize,
										void							*outData);
										
		void			SetProperty (	UInt64 							nowTime, 
										OSStatus 						result,
										AudioUnitPropertyID 			inID,
										AudioUnitScope 					inScope,
										AudioUnitElement 				inElement,
										const void *					inData,
										UInt32 							inDataSize);
	
		void			Render (		UInt64 							nowTime, 
										OSStatus 						result,
										AudioUnitRenderActionFlags *	inRenderFlags,
										const AudioTimeStamp *			inTimeStamp,
										UInt32							inOutputBusNumber,
										UInt32							inNumFrames,
										AudioBufferList *				inIOData);
			
private:
		AUBase*			mAUBase;
		UInt64			mFirstTime;
		FILE*			mFile;
		bool			mHaveDoneProperty;
		CAMutex			mPrintMutex;


		OSStatus		mHostCB1_Result;
		OSStatus		mHostCB2_Result;
		OSStatus		mHostCB3_Result;
		int				mHostCB_WhenToPrint;
		int				mHostCB_WasPlaying;
		
		double 			SecsSinceStart(UInt64 inNowTime)
		{
			UInt64 nanos = CAHostTimeBase::ConvertToNanos(inNowTime - mFirstTime);
			return nanos * 1.0e-9;
		}

		unsigned int	RecordDispatch (UInt64 inStartTime, OSStatus result, const char* inMethod);

		void 			PrintHeaderString (UInt64 inNowTime, unsigned int inThread, const char* inMethod);
		
		unsigned int	AU() const { return (unsigned int)mAUBase->GetComponentInstance(); }
		
		void 			PrintProperty (AudioUnitPropertyID 	inID, AudioUnitScope inScope, AudioUnitElement inElement);	

		void			RenderActions_HostCallbacks ();
};

#endif // AU_DEBUG_DISPATCHER
#endif //__AUDebugDispatcher_h__
