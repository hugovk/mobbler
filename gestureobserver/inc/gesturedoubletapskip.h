/*
gesturedoubletapskip.h

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


#ifndef GESTUREDOUBLETAPSKIP_H_
#define GESTUREDOUBLETAPSKIP_H_

#include "gestureaction.h"
#include "gestureevent.h"
#include "gesturetimeout.h"

class CMobblerGestureDoubleTapSkip : public CMobblerGestureAction,
									 public MMobblerGestureNotifyTimeOut
	{
public:
	~CMobblerGestureDoubleTapSkip();
	static CMobblerGestureDoubleTapSkip* NewL(RPointerArray<MMobblerGestures>& aNotify);
	
public:
	// CMobblerGestureAction
	void Action(TMobblerGestureEvent& aEvent);
	
	// MMobblerGestureNotifyTimeOut
	void NotifyGestureTimeOut();
	
private:
	CMobblerGestureDoubleTapSkip(RPointerArray<MMobblerGestures>& aNotify);
	void ConstructL();
	
	void NotifySkip();
	TBool IsTap(TMobblerGestureEvent& aEvent);
	inline TInt Absolute(TInt aValue);
	
private:
	TInt iLastReading;
	TInt iTapCount;
	CMobblerGestureTimeout* iTimeOut;
	
	RPointerArray<MMobblerGestures>& iNotify;
	};

// Return absolute value of aValue
inline TInt CMobblerGestureDoubleTapSkip::Absolute(TInt aValue)
	{
	return (aValue < 0) ? (-1 * aValue) : aValue;
	}

#endif /*GESTUREDOUBLETAPSKIP_H_*/

// End of file
