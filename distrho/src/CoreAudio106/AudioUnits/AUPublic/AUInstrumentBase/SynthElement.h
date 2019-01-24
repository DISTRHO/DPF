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
#ifndef __SynthElement__
#define __SynthElement__

#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>
#include "MusicDeviceBase.h"
#include "SynthNoteList.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AUInstrumentBase;

class SynthElement : public AUElement
{
public:
	SynthElement(AUInstrumentBase *audioUnit, UInt32 inElement);
	virtual ~SynthElement();

	UInt32 GetIndex() const { return mIndex; }
	
	AUInstrumentBase* GetAUInstrument() { return (AUInstrumentBase*)GetAudioUnit(); }
	
	CFStringRef GetName() const { return mName; }
	void SetName(CFStringRef inName) 
		{
			CFStringRef oldName = mName;
			mName = inName; 
			CFRetain(mName); 
			if (oldName) CFRelease(oldName);
		}
private:
	CFStringRef mName;
	UInt32 mIndex;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum {
	kMidiController_BankSelect				= 0,
	kMidiController_ModWheel				= 1,
	kMidiController_Breath					= 2,
	kMidiController_Foot					= 4,
	kMidiController_PortamentoTime			= 5,
	kMidiController_DataEntry				= 6,
	kMidiController_Volume					= 7,
	kMidiController_Balance					= 8,
	kMidiController_Pan						= 10,
	kMidiController_Expression				= 11,

		// these controls have a (0-63) == off, (64-127) == on
	kMidiController_Sustain					= 64, //hold1
	kMidiController_Portamento				= 65,
	kMidiController_Sostenuto				= 66,
	kMidiController_Soft					= 67,
	kMidiController_LegatoPedal				= 68,
	kMidiController_Hold2Pedal				= 69,
	kMidiController_FilterResonance			= 71,
	kMidiController_ReleaseTime				= 72,
	kMidiController_AttackTime				= 73,
	kMidiController_Brightness				= 74,
	kMidiController_DecayTime				= 75,
	kMidiController_VibratoRate				= 76,
	kMidiController_VibratoDepth			= 77,
	kMidiController_VibratoDelay			= 78,
	
		// these controls have a 0-127 range and in MIDI they have no LSB (so fractional values are lost in MIDI)
	kMidiController_ReverbLevel				= 91,
	kMidiController_ChorusLevel				= 93,

	kMidiController_AllSoundOff				= 120,
	kMidiController_ResetAllControllers		= 121,
	kMidiController_AllNotesOff				= 123
};

struct MidiControls
{
	MidiControls();
	void Reset();
	
	UInt8 mControls[128];
	UInt8 mPolyPressure[128];
	UInt8 mMonoPressure;
	UInt8 mProgramChange;
	UInt16 mPitchBend;
	UInt16 mActiveRPN;
	UInt16 mActiveNRPN;
	UInt16 mActiveRPValue;
	UInt16 mActiveNRPValue;
	
	UInt16 mPitchBendDepth;
	float mFPitchBendDepth;
	float mFPitchBend;
	
	SInt16 GetHiResControl(UInt32 inIndex) const 
		{   
			return ((mControls[inIndex] & 127) << 7) | (mControls[inIndex + 32] & 127);
		}
		
	void SetHiResControl(UInt32 inIndex, UInt8 inMSB, UInt8 inLSB)
		{ 
			mControls[inIndex] = inMSB;
			mControls[inIndex + 32] = inLSB;
		}
		
	float GetControl(UInt32 inIndex) const
		{
			if (inIndex < 32) {
				return (float)mControls[inIndex] + (float)mControls[inIndex + 32] / 127.;
			} else {
				return (float)mControls[inIndex];
			}
		}
		
	float PitchBend() const { return mFPitchBend * mFPitchBendDepth; }
	
};


class SynthGroupElement : public SynthElement
{
public:
	enum {
		kUnassignedGroup = 0xFFFFFFFF
	};
	
			SynthGroupElement(AUInstrumentBase *audioUnit, UInt32 inElement);

	void NoteOff(NoteInstanceID inNoteID, UInt32 inFrame);
	void SustainOn(UInt32 inFrame);
	void SustainOff(UInt32 inFrame);
	void SostenutoOn(UInt32 inFrame);
	void SostenutoOff(UInt32 inFrame);
	
	void NoteEnded(SynthNote *inNote, UInt32 inFrame);
	
	void AllNotesOff(UInt32 inFrame);
	void AllSoundOff(UInt32 inFrame);
	void ResetAllControllers(UInt32 inFrame);
	
	UInt32 GetOutputBus() const { return mOutputBus; }
	void SetOutputBus(UInt32 inBus) { mOutputBus = inBus; }
	
	void Reset();
	
	virtual OSStatus Render(UInt32 inNumberFrames);
	
	float GetControl(UInt32 inIndex) const { return mMidiControls.GetControl(inIndex); }
	float PitchBend() const { return mMidiControls.PitchBend(); }
	
	MusicDeviceGroupID		GroupID () const { return mGroupID; }
	void					SetGroupID (MusicDeviceGroupID inGroup);
	
private:
	friend class AUInstrumentBase;
	friend class AUMonotimbralInstrumentBase;
	friend class AUMultitimbralInstrumentBase;
	
	MidiControls mMidiControls;
	
	bool					mSustainIsOn;
	bool					mSostenutoIsOn;
	UInt32					mOutputBus;
	MusicDeviceGroupID		mGroupID;
	
	
	SynthNoteList mNoteList[kNumberOfSoundingNoteStates];	
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SynthKeyZone
{
	UInt8 mLoNote;
	UInt8 mHiNote;
	UInt8 mLoVelocity;
	UInt8 mHiVelocity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

const UInt32 kUnlimitedPolyphony = 0xFFFFFFFF;

class SynthPartElement : public SynthElement
{
public:
	SynthPartElement(AUInstrumentBase *audioUnit, UInt32 inElement);

	UInt32		GetGroupIndex() const { return mGroupIndex; }
	bool		InRange(Float32 inNote, Float32 inVelocity);
	
	UInt32		GetMaxPolyphony() const { return mMaxPolyphony; }
	void		SetMaxPolyphony(UInt32 inMaxPolyphony) { mMaxPolyphony = inMaxPolyphony; }
	
private:
	UInt32							mGroupIndex;
	UInt32							mPatchIndex;
	UInt32							mMaxPolyphony;
	SynthKeyZone					mKeyZone;	
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline AUInstrumentBase*	SynthNote::GetAudioUnit() const 
							{ 
								return (AUInstrumentBase*)mGroup->GetAudioUnit(); 
							}

inline Float32				SynthNote::GetGlobalParameter(AudioUnitParameterID inParamID) const 
							{
								return mGroup->GetAudioUnit()->Globals()->GetParameter(inParamID);
							}

inline void					SynthNote::NoteEnded(UInt32 inFrame) 
							{ 
								mGroup->NoteEnded(this, inFrame);
								mNoteID = 0xFFFFFFFF; 
							}

inline float				SynthNote::PitchBend() const 
							{ 
								return mGroup->PitchBend(); 
							}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
