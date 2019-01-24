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
#include "CARingBuffer.h"
#include "CABitOperations.h"
#include "CAAutoDisposer.h"
#include "CAAtomic.h"

#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <libkern/OSAtomic.h>

CARingBuffer::CARingBuffer() :
	mBuffers(NULL), mNumberChannels(0), mCapacityFrames(0), mCapacityBytes(0)
{

}

CARingBuffer::~CARingBuffer()
{
	Deallocate();
}


void	CARingBuffer::Allocate(int nChannels, UInt32 bytesPerFrame, UInt32 capacityFrames)
{
	Deallocate();
	
	//capacityFrames = NextPowerOfTwo(capacityFrames);
	
	mNumberChannels = nChannels;
	mBytesPerFrame = bytesPerFrame;
	mCapacityFrames = capacityFrames;
	//mCapacityFramesMask = capacityFrames - 1;
	mCapacityBytes = bytesPerFrame * capacityFrames;

	// put everything in one memory allocation, first the pointers, then the deinterleaved channels
	UInt32 allocSize = (mCapacityBytes + sizeof(Byte *)) * nChannels;
	Byte *p = (Byte *)CA_malloc(allocSize);
	memset(p, 0, allocSize);
	mBuffers = (Byte **)p;
	p += nChannels * sizeof(Byte *);
	for (int i = 0; i < nChannels; ++i) {
		mBuffers[i] = p;
		p += mCapacityBytes;
	}
	
	for (UInt32 i = 0; i<kGeneralRingTimeBoundsQueueSize; ++i)
	{
		mTimeBoundsQueue[i].mStartTime = 0;
		mTimeBoundsQueue[i].mEndTime = 0;
		mTimeBoundsQueue[i].mUpdateCounter = 0;
	}
	mTimeBoundsQueuePtr = 0;
}

void	CARingBuffer::Deallocate()
{
	if (mBuffers) {
		free(mBuffers);
		mBuffers = NULL;
	}
	mNumberChannels = 0;
	mCapacityBytes = 0;
	mCapacityFrames = 0;
}

inline void ZeroRange(Byte **buffers, int nchannels, int offset, int nbytes)
{
	while (--nchannels >= 0) {
		memset(*buffers + offset, 0, nbytes);
		++buffers;
	}
}

inline void StoreABL(Byte **buffers, int destOffset, const AudioBufferList *abl, int srcOffset, int nbytes)
{
	int nchannels = abl->mNumberBuffers;
	const AudioBuffer *src = abl->mBuffers;
	while (--nchannels >= 0) {
		memcpy(*buffers + destOffset, (Byte *)src->mData + srcOffset, nbytes);
		++buffers;
		++src;
	}
}

inline void FetchABL(AudioBufferList *abl, int destOffset, Byte **buffers, int srcOffset, int nbytes)
{
	int nchannels = abl->mNumberBuffers;
	AudioBuffer *dest = abl->mBuffers;
	while (--nchannels >= 0) {
		memcpy((Byte *)dest->mData + destOffset, *buffers + srcOffset, nbytes);
		++buffers;
		++dest;
	}
}

inline void ZeroABL(AudioBufferList *abl, int destOffset, int nbytes)
{
	int nBuffers = abl->mNumberBuffers;
	AudioBuffer *dest = abl->mBuffers;
	while (--nBuffers >= 0) {
		memset((Byte *)dest->mData + destOffset, 0, nbytes);
		++dest;
	}
}


CARingBufferError	CARingBuffer::Store(const AudioBufferList *abl, UInt32 framesToWrite, SampleTime startWrite)
{
	if (framesToWrite > mCapacityFrames)
		return kCARingBufferError_TooMuch;		// too big!

	SampleTime endWrite = startWrite + framesToWrite;
	
	if (startWrite < EndTime()) {
		// going backwards, throw everything out
		SetTimeBounds(startWrite, startWrite);
	} else if (endWrite - StartTime() <= mCapacityFrames) {
		// the buffer has not yet wrapped and will not need to
	} else {
		// advance the start time past the region we are about to overwrite
		SampleTime newStart = endWrite - mCapacityFrames;	// one buffer of time behind where we're writing
		SampleTime newEnd = std::max(newStart, EndTime());
		SetTimeBounds(newStart, newEnd);
	}
	
	// write the new frames
	Byte **buffers = mBuffers;
	int nchannels = mNumberChannels;
	int offset0, offset1, nbytes;
	SampleTime curEnd = EndTime();
	
	if (startWrite > curEnd) {
		// we are skipping some samples, so zero the range we are skipping
		offset0 = FrameOffset(curEnd);
		offset1 = FrameOffset(startWrite);
		if (offset0 < offset1)
			ZeroRange(buffers, nchannels, offset0, offset1 - offset0);
		else {
			ZeroRange(buffers, nchannels, offset0, mCapacityBytes - offset0);
			ZeroRange(buffers, nchannels, 0, offset1);
		}
		offset0 = offset1;
	} else {
		offset0 = FrameOffset(startWrite);
	}

	offset1 = FrameOffset(endWrite);
	if (offset0 < offset1)
		StoreABL(buffers, offset0, abl, 0, offset1 - offset0);
	else {
		nbytes = mCapacityBytes - offset0;
		StoreABL(buffers, offset0, abl, 0, nbytes);
		StoreABL(buffers, 0, abl, nbytes, offset1);
	}
	
	// now update the end time
	SetTimeBounds(StartTime(), endWrite);
	
	return kCARingBufferError_OK;	// success
}

void	CARingBuffer::SetTimeBounds(SampleTime startTime, SampleTime endTime)
{
	UInt32 nextPtr = mTimeBoundsQueuePtr + 1;
	UInt32 index = nextPtr & kGeneralRingTimeBoundsQueueMask;
	
	mTimeBoundsQueue[index].mStartTime = startTime;
	mTimeBoundsQueue[index].mEndTime = endTime;
	mTimeBoundsQueue[index].mUpdateCounter = nextPtr;
	CAAtomicCompareAndSwap32Barrier(mTimeBoundsQueuePtr, mTimeBoundsQueuePtr + 1, (SInt32*)&mTimeBoundsQueuePtr);
}

CARingBufferError	CARingBuffer::GetTimeBounds(SampleTime &startTime, SampleTime &endTime)
{
	for (int i=0; i<8; ++i) // fail after a few tries.
	{
		UInt32 curPtr = mTimeBoundsQueuePtr;
		UInt32 index = curPtr & kGeneralRingTimeBoundsQueueMask;
		CARingBuffer::TimeBounds* bounds = mTimeBoundsQueue + index;
		
		startTime = bounds->mStartTime;
		endTime = bounds->mEndTime;
		UInt32 newPtr = bounds->mUpdateCounter;
		
		if (newPtr == curPtr) 
			return kCARingBufferError_OK;
	}
	return kCARingBufferError_CPUOverload;
}

CARingBufferError	CARingBuffer::CheckTimeBounds(SampleTime& startRead, SampleTime& endRead)
{
	SampleTime startTime, endTime;
	
	CARingBufferError err = GetTimeBounds(startTime, endTime);
	if (err) return err;
	
	startRead = std::max(startRead, startTime);
	endRead = std::min(endRead, endTime);

	if (startRead < startTime)
	{
		if (endRead > endTime)
			return kCARingBufferError_TooMuch;
	
		if (endRead < startTime)
			return kCARingBufferError_WayBehind;
		else
			return kCARingBufferError_SlightlyBehind;
	}
	
	if (endRead > endTime)	// we are going to read chunks of zeros its okay
	{
		if (startRead > endTime)
			return kCARingBufferError_WayAhead;
		else
			return kCARingBufferError_SlightlyAhead;
	}
	
	return kCARingBufferError_OK;	// success
}

CARingBufferError worse(CARingBufferError a, CARingBufferError b)
{
	// return the worst error.
	CARingBufferError aa = a < 0 ? -a : a;
	CARingBufferError bb = b < 0 ? -b : b;
	if (aa > bb) return a;
	return b;
}

CARingBufferError	CARingBuffer::Fetch(AudioBufferList *abl, UInt32 nFrames, SampleTime startRead, bool outOfBoundsOK)
{
	SampleTime endRead = startRead + nFrames;

	SampleTime startRead0 = startRead;
	SampleTime endRead0 = endRead;
	SampleTime size;
		
	CARingBufferError err = CheckTimeBounds(startRead, endRead);
	size = endRead - startRead;
	if (err) {
		if (!outOfBoundsOK) return err;
		if (size <= 0) return err; // there is nothing to read
	}
	
	SInt32 destStartOffset = startRead - startRead0; 
	if (destStartOffset > 0) {
		ZeroABL(abl, 0, destStartOffset * mBytesPerFrame);
	}

	SInt32 destEndSize = endRead0 - endRead; 
	if (destEndSize > 0) {
		ZeroABL(abl, destStartOffset + size, destEndSize * mBytesPerFrame);
	}
	
	Byte **buffers = mBuffers;
	int offset0 = FrameOffset(startRead);
	int offset1 = FrameOffset(endRead);
	int nbytes;
	
	if (offset0 < offset1) {
		FetchABL(abl, destStartOffset, buffers, offset0, nbytes = offset1 - offset0);
	} else {
		nbytes = mCapacityBytes - offset0;
		FetchABL(abl, destStartOffset, buffers, offset0, nbytes);
		FetchABL(abl, destStartOffset + nbytes, buffers, 0, offset1);
		nbytes += offset1;
	}

	int nchannels = abl->mNumberBuffers;
	AudioBuffer *dest = abl->mBuffers;
	while (--nchannels >= 0)
	{
		dest->mDataByteSize = nbytes;
		dest++;
	}

	// have to check bounds again because the data may have been overwritten before we could finish reading it. 
	OSStatus err2 = CheckTimeBounds(startRead, endRead);
	err2 = worse(err, err2);
	size = endRead - startRead;
	if (err2) {
		if (!outOfBoundsOK) return err2;
		if (size <= 0) return err2; // there is nothing to read
	}
	return err2;
}
