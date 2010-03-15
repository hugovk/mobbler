/*
gesturedoubletapskip.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008  Michael Coffey

http://code.google.com/p/mobbler

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "gesturedoubletapskip.h"

/**
 * This action delegate object is not reliable, as it conflicts too much
 * with shaking the phone backwards and forwards.
 * 
 * The code is here in case it is improved at a later date, but note that
 * it is not included in either plug-in DLL at present, as it is not
 * included in the MMP files.
 */

// Accelerometer sensor UID
const TInt KAccelerometerSensorUID(0x10273024);

// Microsecond time-out constants for user tapping the phone.
const TTimeIntervalMicroSeconds32 KSecondTapArrivalTimeOut(400000);
const TTimeIntervalMicroSeconds32 KReflectionTimeOut(50000);

// Constants defining the acceleration points of interest in the accelerometer's
// scaled version of m/s^2
const TInt KPulseMinPeakAcceleration(500);
const TInt KPulseMaxReflectionAcceleration(380);
const TInt KPulseMinReflectionAcceleration(20);

// Number of taps to notify after (2 for double tap)
const TInt KNumberOfTaps(2);

CMobblerGestureDoubleTapSkip::~CMobblerGestureDoubleTapSkip()
	{
	delete iTimeOut;
	}

CMobblerGestureDoubleTapSkip* CMobblerGestureDoubleTapSkip::NewL(RPointerArray<MMobblerGestures>& aNotify)
	{
	CMobblerGestureDoubleTapSkip* self(new(ELeave) CMobblerGestureDoubleTapSkip(aNotify));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CMobblerGestureDoubleTapSkip::CMobblerGestureDoubleTapSkip(RPointerArray<MMobblerGestures>& aNotify)
: iLastReading(0), iTapCount(0), iNotify(aNotify)
	{
	}

void CMobblerGestureDoubleTapSkip::ConstructL()
	{
	iTimeOut = CMobblerGestureTimeout::NewL(CActive::EPriorityStandard, *this);
	}

void CMobblerGestureDoubleTapSkip::Action(TMobblerGestureEvent& aEvent)
	{
	if (aEvent.iSensorId != KAccelerometerSensorUID)
		{
		return;
		}
	
	if (IsTap(aEvent))
		{
		// Count this Tap
		++iTapCount;
		
		if (iTapCount == KNumberOfTaps)
			{
			NotifySkip();
			iTapCount = 0;
			}
		else
			{
			iTimeOut->Begin(KSecondTapArrivalTimeOut);
			}
		}
	}

TBool CMobblerGestureDoubleTapSkip::IsTap(TMobblerGestureEvent& aEvent)
	{
	TBool userHasTapped(EFalse);
	
	if (Absolute(aEvent.iData3) >= KPulseMinPeakAcceleration)
		{
		// This is potentially the acceleration stage of a tap.
		// Reset the timer and save this value.
		iTimeOut->Cancel();
		iLastReading = aEvent.iData3;
		iTimeOut->Begin(KReflectionTimeOut);
		}
	else if ( iLastReading // Guard for divide by 0 with lazy evaluation
			&& (Absolute(iLastReading) / iLastReading) 
				!= (Absolute(aEvent.iData3) / aEvent.iData3) )
		{
		// Sign is different to previous reading,
		// Test to see if it's a potential reflection.
		if (Absolute(aEvent.iData3) >= KPulseMinReflectionAcceleration
				&& Absolute(aEvent.iData3 <= KPulseMinPeakAcceleration))
			{
			iLastReading = 0;
			iTimeOut->Cancel();
			userHasTapped = ETrue;
			}
		}
	
	return userHasTapped;
	}

void CMobblerGestureDoubleTapSkip::NotifyGestureTimeOut()
	{
	iLastReading = 0;
	iTapCount = 0;
	}

void CMobblerGestureDoubleTapSkip::NotifySkip()
	{
	const TInt KNumberOfListeners(iNotify.Count());
	for(TInt i(0); i < KNumberOfListeners; ++i)
		{
		iNotify[i]->AccelerometerSkipTrack();
		}
	}

// End of file
