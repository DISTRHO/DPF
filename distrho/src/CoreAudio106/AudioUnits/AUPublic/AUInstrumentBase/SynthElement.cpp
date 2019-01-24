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
#include "SynthElement.h"
#include "AUInstrumentBase.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
MidiControls::MidiControls()
{
	Reset();
}

void MidiControls::Reset()
{
	memset(this, 0, sizeof(MidiControls));
	mControls[kMidiController_Pan] = 64;
	mControls[kMidiController_Expression] = 127;
	mPitchBendDepth = 2 << 7;
	mFPitchBendDepth = 2.;
}


SynthElement::SynthElement(AUInstrumentBase *audioUnit, UInt32 inElement) 
	: AUElement(audioUnit), mName(0), mIndex(inElement)
{
}

SynthElement::~SynthElement()
{
	if (mName) CFRelease(mName);
}

SynthGroupElement::SynthGroupElement(AUInstrumentBase *audioUnit, UInt32 inElement) 
	: SynthElement(audioUnit, inElement), mSustainIsOn(false), mSostenutoIsOn(false), mOutputBus(0), mGroupID(kUnassignedGroup)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::SynthGroupElement %d\n", inElement);
#endif
	for (UInt32 i=0; i<kNumberOfSoundingNoteStates; ++i)
		mNoteList[i].mState = i;
}

void	SynthGroupElement::SetGroupID (MusicDeviceGroupID inGroup)
{
		// can't re-assign a group once its been assigned
	if (mGroupID != kUnassignedGroup) throw static_cast<OSStatus>(kAudioUnitErr_InvalidElement);
	mGroupID = inGroup;
}

void SynthGroupElement::Reset() 
{
#if DEBUG_PRINT
	printf("SynthGroupElement::Reset\n");
#endif
	mMidiControls.Reset();
	for (UInt32 i=0; i<kNumberOfSoundingNoteStates; ++i)
		mNoteList[i].Empty();
}

SynthPartElement::SynthPartElement(AUInstrumentBase *audioUnit, UInt32 inElement) 
	: SynthElement(audioUnit, inElement)
{
}

void SynthGroupElement::NoteOff(NoteInstanceID inNoteID, UInt32 inFrame)
{	
#if DEBUG_PRINT
	printf("SynthGroupElement::NoteOff %d\n", inNoteID);
#endif
	SynthNote *note = mNoteList[kNoteState_Attacked].mHead;
	// see if this note is attacked.
	while (note && note->mNoteID != inNoteID)
	{
#if DEBUG_PRINT
		printf("   ? %08X %d\n", note, note->mNoteID);
#endif
		note = note->mNext;
	}
	
#if DEBUG_PRINT
	printf("  found %08X\n", note);
#endif
	if (note)
	{
#if DEBUG_PRINT
		printf("  old state %d\n", note->mState);
#endif
		mNoteList[kNoteState_Attacked].RemoveNote(note);
		note->Release(inFrame);
		if (mSustainIsOn) {
			mNoteList[kNoteState_ReleasedButSustained].AddNote(note);
		} else {
			mNoteList[kNoteState_Released].AddNote(note);
		}
#if DEBUG_PRINT
		printf("  new state %d\n", note->mState);
#endif
	}
	else if (mSostenutoIsOn)
	{
		// see if this note is sostenutoed.
		note = mNoteList[kNoteState_Sostenutoed].mHead;
		while (note && note->mNoteID != inNoteID)
			note = note->mNext;
		if (note)
		{
			mNoteList[kNoteState_Sostenutoed].RemoveNote(note);
			mNoteList[kNoteState_ReleasedButSostenutoed].AddNote(note);
		}
	}
}

void SynthGroupElement::NoteEnded(SynthNote *inNote, UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::NoteEnded %d %d\n", inNote->mNoteID, inNote->mState);
#endif
	SynthNoteList *list = mNoteList + inNote->mState;
	list->RemoveNote(inNote);
	
	GetAUInstrument()->AddFreeNote(inNote);
}

void SynthGroupElement::SostenutoOn(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::SostenutoOn\n");
#endif
	mSostenutoIsOn = true;
	mNoteList[kNoteState_Sostenutoed].TransferAllFrom(&mNoteList[kNoteState_Attacked], inFrame);
}

void SynthGroupElement::SostenutoOff(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::SostenutoOff\n");
#endif
	mSostenutoIsOn = false;
	mNoteList[kNoteState_Attacked].TransferAllFrom(&mNoteList[kNoteState_Sostenutoed], inFrame);
	if (mSustainIsOn) 
		mNoteList[kNoteState_ReleasedButSustained].TransferAllFrom(&mNoteList[kNoteState_ReleasedButSostenutoed], inFrame);
	else
		mNoteList[kNoteState_Released].TransferAllFrom(&mNoteList[kNoteState_ReleasedButSostenutoed], inFrame);
}


void SynthGroupElement::SustainOn(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::SustainOn\n");
#endif
	mSustainIsOn = true;	
}

void SynthGroupElement::SustainOff(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::SustainOff\n");
#endif
	mSustainIsOn = false;
	
	mNoteList[kNoteState_Released].TransferAllFrom(&mNoteList[kNoteState_ReleasedButSustained], inFrame);
}

void SynthGroupElement::AllNotesOff(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::AllNotesOff\n");
#endif
	SynthNote *note;
	
	for (UInt32 i=0 ; i<kNumberOfActiveNoteStates; ++i)
	{
		note = mNoteList[i].mHead;
		while (note)
		{
			SynthNote *nextNote = note->mNext;
			
			mNoteList[i].RemoveNote(note);
			note->FastRelease(inFrame);
			mNoteList[kNoteState_FastReleased].AddNote(note);
			
			note = nextNote;
		}
	}	
}

void SynthGroupElement::AllSoundOff(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::AllSoundOff\n");
#endif
	AllNotesOff(inFrame);
}

void SynthGroupElement::ResetAllControllers(UInt32 inFrame)
{
#if DEBUG_PRINT
	printf("SynthGroupElement::ResetAllControllers\n");
#endif
	mMidiControls.Reset();
}

OSStatus SynthGroupElement::Render(UInt32 inNumberFrames)
{
	SynthNote *note;
	AudioBufferList& bufferList = GetAudioUnit()->GetOutput(mOutputBus)->GetBufferList();
	
	for (UInt32 i=0 ; i<kNumberOfSoundingNoteStates; ++i)
	{
		note = mNoteList[i].mHead;
		while (note)
		{
#if DEBUG_PRINT
			printf("   note %d %08X   %d\n", i, note, inNumberFrames);
#endif
			SynthNote *nextNote = note->mNext;
			
			OSStatus err = note->Render(inNumberFrames, bufferList);
			if (err) return err;
			
			note = nextNote;
		}
	}
	
	return noErr;
}


