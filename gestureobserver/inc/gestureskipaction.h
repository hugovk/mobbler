/*
gestureskipaction.h

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


#ifndef GESTURESKIPACTION_H_
#define GESTURESKIPACTION_H_

#include <e32base.h>
#include "gestureaction.h"
#include "gestureevent.h"
#include "gesturetimeout.h"


class CMobblerGestureSkipAction : public CMobblerGestureAction,
								  public MMobblerGestureNotifyTimeOut
	{
private:
	enum TSkipGestureState
		{
		EAccelerationStage, // Waiting for handset to accelerate to the right
		EDecelerationStage  // ... and to the left.
		};
        
public:
	CMobblerGestureSkipAction(RPointerArray<MMobblerGestures>& aNotify);
	static CMobblerGestureSkipAction* NewL(RPointerArray<MMobblerGestures>& aNotify);
    
public:
	// CMobblerGestureAction
	void Action(TMobblerGestureEvent& aEvent);
	
	// MMobblerNotifySkipTimeOut
	void NotifyGestureTimeOut();
	
private:
	~CMobblerGestureSkipAction();
	void ConstructL();
	
	void SkipGestureTest(TMobblerGestureEvent& aEvent);
	void NotifySkip();

private:
	TSkipGestureState iSkipGestureState;
	CMobblerGestureTimeout* iSkipTimer;
	
	RPointerArray<MMobblerGestures>& iNotify;
	};

#endif /*GESTURESKIPACTION_H_*/

// End of file
