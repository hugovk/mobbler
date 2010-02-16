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


#ifndef GESTUREOBSERVER_H_
#define GESTUREOBSERVER_H_

#include <e32base.h>
#include <f32file.h>

#include "gestureaction.h"
#include "gestureevent.h"

#include <mobbler\mobblergesturesinterface.h>


class CMobblerGestureObserver : public CMobblerGesturesInterface
	{
public:
	virtual ~CMobblerGestureObserver();
	
	// CMobblerGesturesInterface
	void ObserveGesturesL(MMobblerGestures& aNotify);
	void StopObserverL(MMobblerGestures& aNotify);
	
protected:
	void BaseConstructL();
	void DelegateEvent(TMobblerGestureEvent& aEvent);
	
	// Template implementation for variant abstraction.
	virtual void DoStartObservingL() = 0;
	virtual void DoStopObservingL() = 0;
	
private:
	void LoadGesturesL();
	
private:
	// Objects implementing callback functions (listeners not owned)
	RPointerArray<MMobblerGestures> iGestureListeners;
	
	// Action delegates (owned)
	RPointerArray<CMobblerGestureAction> iActions;
	
#ifdef __MOBBLER_SENSOR_DATA_LOGGING__
	// File handle for logging of sensor data.
	RFs iLoggingFsSession;
	RFile iSensorCsvFile;
	
	// Time the plugin started listening.
	TTime iStartTime;
#endif 	// __MOBBLER_SENSOR_DATA_LOGGING__
	};

#endif /*GESTUREOBSERVER_H_*/

// End of file
