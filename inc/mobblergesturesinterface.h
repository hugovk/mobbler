/*
mobblergesturesinterface.h

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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


#ifndef MOBBLERGESTURESINTERFACE_H_
#define MOBBLERGESTURESINTERFACE_H_

/**
 * Direction information for a shake gesture callback.
 * Descriptions are true if the normal phone orientation is assumed, with the phone
 * screen facing the user and the keys (if any) are at the bottom.
 */
enum TMobblerShakeGestureDirection
	{
	EShakeRight,
	EShakeLeft,
	EShakeAwayFromUser,
	EShakeTowardUser
	};

/**
 * Supported sensor gestures.
 * This is the callback interface for the gesture observer plugin
 * to invoke application functions after recognising a gesture.
 */
class MMobblerGestures
	{
public:
	// Accelerometer: Single shake in a direction specified by aDirection.
	virtual void HandleSingleShakeL(TMobblerShakeGestureDirection aDirection) = 0;
	};

/**
 * Plugin interface specification.
 * Gesture observer plugin must implment this interface.
 */
class CMobblerGesturesInterface : public CBase
	{
public:
	virtual void ObserveGesturesL(MMobblerGestures& aNotify) = 0;
	virtual void StopObserverL(MMobblerGestures& aNotify) = 0;
	};

#endif /*MOBBLERGESTURESINTERFACE_H_*/
