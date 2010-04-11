/*
gestureobserver.cpp

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

#include "gestureevent.h"
#include "gestureobserver.h"

// Actions
#include "gestureskipaction.h"
#include "gesturedoubletapskip.h"


#ifdef __MOBBLER_SENSOR_DATA_LOGGING__
// CSV Data file for sensor logging
// Enable with macro in MMP file.
_LIT(KSensorCsvFilePath, "c:\\mobbler_sensor.csv");

// It is important that the header line reflects the format
// specified in the formatting constant below.
_LIT8(KSensorCsvHeaderLine, "MicroSecFromStart,SensorId,iSensorData1,iSensorData2,iSensorData3\n");
_LIT8(KSensorCsvFormat, "%Ld,%d,%d,%d,%d\n");
#endif // __MOBBLER_SENSOR_DATA_LOGGING__

// CMobblerGestureObserver
CMobblerGestureObserver::~CMobblerGestureObserver()
	{
#ifdef __MOBBLER_SENSOR_DATA_LOGGING__
	iSensorCsvFile.Close();
	iLoggingFsSession.Close();
#endif // __MOBBLER_SENSOR_DATA_LOGGING__
	
	iActions.ResetAndDestroy();
	iGestureListeners.Close();
	}

void CMobblerGestureObserver::BaseConstructL()
	{
#ifdef __MOBBLER_SENSOR_DATA_LOGGING__
	// Setup CSV file for data logging.
	User::LeaveIfError(iLoggingFsSession.Connect());
	User::LeaveIfError(iSensorCsvFile.Replace(iLoggingFsSession, KSensorCsvFilePath(), EFileWrite));

	User::LeaveIfError(iSensorCsvFile.Write(KSensorCsvHeaderLine));
	
	// Record the start time.
	iStartTime.HomeTime();
#endif // __MOBBLER_SENSOR_DATA_LOGGING__
	
	// Load the action delegate objects
	LoadGesturesL();
	}

void CMobblerGestureObserver::LoadGesturesL()
	{
	// Skip (shake to the right)
	iActions.AppendL(CMobblerGestureSkipAction::NewL(iGestureListeners));
	
	// Tap skip - not reliable yet (removed from MMP)
	// iActions.AppendL(CMobblerGestureDoubleTapSkip::NewL(iGestureListeners));
	}

void CMobblerGestureObserver::ObserveGesturesL(MMobblerGestures& aNotify)
	{
	// Find out how many listeners we already had
	const TInt KPreviousNumberListeners(iGestureListeners.Count());
	
	// If the new listener is already in the list, return
	for (TInt i(0); i < KPreviousNumberListeners; ++i)
		{
		if (iGestureListeners[i] == &aNotify)
			{
			return;
			}
		}
	
	// Add the new listener
	iGestureListeners.AppendL(&aNotify);
	
	// If we didn't have any before, ask the implementation to open
	// its channel and start handling events
	if (KPreviousNumberListeners == 0)
		{
		DoStartObservingL();
		}
	}

void CMobblerGestureObserver::StopObserverL(MMobblerGestures& aNotify)
	{
	// Remove this listener from the list of listeners
	const TInt KNumberOfListeners(iGestureListeners.Count());
	for (TInt i(0); i < KNumberOfListeners; ++i)
		{
		if (iGestureListeners[i] == &aNotify)
			{
			iGestureListeners.Remove(i);
			break;
			}
		}
	
	// If that was the only one, ask the variant implementation to close
	// its accelerometer channel.
	if (iGestureListeners.Count() == 0)
		{
		DoStopObservingL();
		}
	}

void CMobblerGestureObserver::DelegateEvent(TMobblerGestureEvent& aEvent)
	{
#ifdef __MOBBLER_SENSOR_DATA_LOGGING__
	// Verbose accelerometer data logging in CSV format.
	
	// Calculate time from start
	TTime timeNow;
	timeNow.HomeTime();
	
	TTimeIntervalMicroSeconds timeFromStart(timeNow.MicroSecondsFrom(iStartTime));
	
	// Format the logging line.
	const TInt KMaxLogLine(64);
	TBuf8<KMaxLogLine> logLine;

	logLine.Format(KSensorCsvFormat, timeFromStart.Int64(), aEvent.iSensorId, 
			aEvent.iData1, aEvent.iData2, aEvent.iData3);
	
	// Write logging to file.
	iSensorCsvFile.Write(logLine);
	
#endif // __MOBBLER_SENSOR_DATA_LOGGING__
	
	const TInt KNumberOfActions(iActions.Count());
	for (TInt i(0); i < KNumberOfActions; ++i)
		{
		iActions[i]->Action(aEvent);
		}
	}


// End of file
