/*
gestureevent.h

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


#ifndef GESTUREEVENT_H_
#define GESTUREEVENT_H_

#include <e32base.h>

class TMobblerGestureEvent
	{
public:
	inline TMobblerGestureEvent();
	inline TMobblerGestureEvent(TInt aSensorId, TInt aData1, TInt aData2, TInt aData3);
public:
	TInt iSensorId;
	TInt iData1;
	TInt iData2;
	TInt iData3;
	};

inline TMobblerGestureEvent::TMobblerGestureEvent() {}

inline TMobblerGestureEvent::TMobblerGestureEvent(TInt aSensorId, TInt aData1,
												  TInt aData2, TInt aData3)
: iSensorId(aSensorId), iData1(aData1), iData2(aData2), iData3(aData3)
	{
	}

#endif /*GESTUREEVENT_H_*/

// End of file
