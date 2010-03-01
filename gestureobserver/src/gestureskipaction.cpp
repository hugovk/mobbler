/*
gestureskipaction.cpp

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

#include "gestureskipaction.h"

// Accelerometer sensor UID
const TInt KAccelerometerSensorUID(0x10273024);

// Constants for identifying skip acceleration (in scaled m/s^2)
// Values below these constants are to be considered insignificant,
// as they are likely to be background noise.
const TInt KSkipGestureMinAcceleration(600);
const TInt KSkipGestureMinDeceleration(-600);

/*
Analysis using the logging macro in the plug-in MMP file indicates that
this should be considered a reasonable average for the deceleration to peak
following acceleration from a user making a skip gesture.

i.e. The time from the maximum (average) acceleration to the right, until
         the maximum deceleration (negative quantity) returning to the left.
*/
const TTimeIntervalMicroSeconds32 KSkipDecelerationPeriod(150000);


CMobblerGestureSkipAction::CMobblerGestureSkipAction(RPointerArray<MMobblerGestures>& aNotify)
: iSkipGestureState(EAccelerationStage), iNotify(aNotify)
	{
	}

CMobblerGestureSkipAction::~CMobblerGestureSkipAction()
	{
	delete iSkipTimer;
	}

CMobblerGestureSkipAction* CMobblerGestureSkipAction::NewL(RPointerArray<MMobblerGestures>& aNotify)
	{
	CMobblerGestureSkipAction* self(new(ELeave) CMobblerGestureSkipAction(aNotify));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CMobblerGestureSkipAction::ConstructL()
	{
	iSkipTimer = CMobblerGestureTimeout::NewL(CActive::EPriorityStandard, *this);
	}

/* Gesture Test
 * 
 * (1) Wait for acceleration constant to be exceeded.
 *              -> Start the cancel timer
 *              -> Change state to EDecelerationStage
 * (2) Wait for deceleration constant to be exceeded.
 *              -> Cancel the timer
 *              -> Change state back to EAccelerationStage
 *              -> Notify observers of skip request.
 * 
 */
void CMobblerGestureSkipAction::Action(TMobblerGestureEvent& aEvent)
	{
	if (aEvent.iSensorId != KAccelerometerSensorUID)
		{
		return;
		}
	
	switch(iSkipGestureState)
		{
		default:
		case EAccelerationStage:
			if (aEvent.iData2 >= KSkipGestureMinAcceleration)
				{
				iSkipGestureState = EDecelerationStage;
				iSkipTimer->Begin(KSkipDecelerationPeriod);
				}
			break;
		case EDecelerationStage:
			if (aEvent.iData2 <= KSkipGestureMinDeceleration)
				{
				iSkipTimer->Cancel();
				iSkipGestureState = EAccelerationStage;
				NotifySkip();
				}
			break;
		}
	}

/* 
 * If a skip gesture has been running too long after its first
 * phase, this function is called, and state changes back
 * to EAccelerationStage.
 * 
 */
void CMobblerGestureSkipAction::NotifyGestureTimeOut()
	{
	iSkipGestureState = EAccelerationStage;
	}

void CMobblerGestureSkipAction::NotifySkip()
	{
	const TInt KNumberOfListeners(iNotify.Count());
	for (TInt i(0); i < KNumberOfListeners; ++i)
		{
		TRAP_IGNORE(iNotify[i]->HandleSingleShakeL(EShakeRight));
		}
	}

// End of file
