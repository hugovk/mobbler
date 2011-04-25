/*
gestureobserver3x.cpp

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

#include <ecom/implementationproxy.h>
#include "gestureobserver3x.h"

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x20039AFE};
#else
const TInt KImplementationUid = {0xA000B6D0};
#endif

// Required for ECOM plugin
const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerGestureObserver3x::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

// Accelerometer sensor UID
const TInt KAccelerometerSensorUID(0x10273024);

// S60 3rd Edition implementation
CMobblerGestureObserver3x::CMobblerGestureObserver3x()
	{
	}

CMobblerGestureObserver3x::~CMobblerGestureObserver3x()
	{
	DoStopObservingL();
	}

CMobblerGestureObserver* CMobblerGestureObserver3x::NewL()
	{
	CMobblerGestureObserver3x* self(new(ELeave) CMobblerGestureObserver3x);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CMobblerGestureObserver3x::ConstructL()
	{
#ifdef __WINS__
	// Fail the plug-in load on the emulator
	User::Leave(KErrNotSupported);
#else	
	BaseConstructL();
	
	// Get the accelerometer info, if it exists,
	// if not, leave and plug-in construction will abort.
	TRRSensorInfo accelerometerInfo(GetAccelerometerL());
	
	// Instantiate the API handle and register self as listener.
	iSensorHandle = CRRSensorApi::NewL(accelerometerInfo);
#endif
	}
	
void CMobblerGestureObserver3x::HandleDataEventL(TRRSensorInfo aSensor,
		TRRSensorEvent aEvent)
	{
	// Gesture detection.
	// Offer the event to all action delegates
	TMobblerGestureEvent event(aSensor.iSensorId, aEvent.iSensorData1, 
			aEvent.iSensorData2, aEvent.iSensorData3);
	
	// Pass up to base class
	DelegateEvent(event);
	}
	
TRRSensorInfo CMobblerGestureObserver3x::GetAccelerometerL()
	{
	// Find available sensors on this device.
	RArray<TRRSensorInfo> sensors;
	CleanupClosePushL(sensors);
	
#ifndef __WINS__
	// Can't populate this array on the emulator. API not available.
	CRRSensorApi::FindSensorsL(sensors);
#endif
	
	// Iterate through the sensor list to find the accelerometer
	TRRSensorInfo accelerometerInfo;
	const TInt KNumberOfSensors(sensors.Count());

	for (TInt i(0); i < KNumberOfSensors; ++i)
		{
		if (sensors[i].iSensorId == KAccelerometerSensorUID)
			{
			accelerometerInfo = sensors[i];
			break;
			}
		else if ((i + 1) == KNumberOfSensors)
			{
			// Couldn't find an accelerometer, feature not supported.
			User::Leave(KErrNotSupported);
			}
		}
	
	// Cleanup and return the accelerometer info
	CleanupStack::PopAndDestroy(&sensors);
	return accelerometerInfo;
	}

void CMobblerGestureObserver3x::DoStartObservingL()
	{
#ifndef __WINS__
	// Does nothing on the emulator.
	if (! iSensorHandle)
		{
		// Have to delete and recreate the sensor handle to work around
		// S60 bug on N82.
		TRRSensorInfo accelerometer(GetAccelerometerL());
		iSensorHandle = CRRSensorApi::NewL(accelerometer);
		}
	iSensorHandle->AddDataListener(this);
#endif
	}

void CMobblerGestureObserver3x::DoStopObservingL()
	{
	if (iSensorHandle)
		{
		// Stop observing sensor data
		iSensorHandle->RemoveDataListener();
		delete iSensorHandle;
		iSensorHandle = NULL;
		}
	}

// End of file
