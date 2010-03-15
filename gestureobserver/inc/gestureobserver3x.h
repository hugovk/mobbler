/*
gestureobserver.h

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

#ifndef GESTUREOBSERVER3X_H_
#define GESTUREOBSERVER3X_H_

#include "gestureobserver.h"
#include <rrsensorapi.h>


// S60 3rd Edition accelerometer gesture implementation

class CMobblerGestureObserver3x : public CMobblerGestureObserver,
								  public MRRSensorDataListener
	{
public:
	~CMobblerGestureObserver3x();
	static CMobblerGestureObserver* NewL();
	
public:
	// MRRSensorDataListener
	void HandleDataEventL(TRRSensorInfo aSensor, TRRSensorEvent aEvent);
	
private:
	CMobblerGestureObserver3x();
	void ConstructL();
	
	TRRSensorInfo GetAccelerometerL();
	
private:
	// S60 3rd Edition implementation of template functions
	void DoStartObservingL();
	void DoStopObservingL();
	
private:
	// Sensors handle
	CRRSensorApi* iSensorHandle;
	};


#endif /*GESTUREOBSERVER3X_H_*/

// End of file
