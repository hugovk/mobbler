/*
gestureobserver5x.cpp

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
#include "gestureevent.h"
#include "gestureobserver5x.h"
#include <sensrvaccelerometersensor.h>

// Required for ECOM plugin
const TImplementationProxy ImplementationTable[] =
    {
    {{0xA000B6C2}, TProxyNewLPtr( CMobblerGestureObserver5x::NewL )}
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
    }

// S60 5th edition implementation

// Constants to control IPC and accelerometer callback performance overheads.
// Unfortunately, this system only works when using the provided timestamps
// on the sensor readings, which does not lend itself well to the RRSensor
// implementation, which is less accurate and generally inferior.
// Using less than ideal settings here to allow for simpler code assessing
// gesture actions.
const TInt KEventsPerCallback(8);
const TInt KMaxTimeToCallbackMilliSeconds(40);
const TInt KFilterValue(108);

// Accelerometer sensor UID, can be used to inform common gesture action
// objects that the event data is of the expected format.
const TInt KAccelerometerSensorUID = 0x10273024;

// Constants for Transform() (see comments above function definition)
const TReal KCorrectionFactor(5.3125); // (680 / 128)
const TInt KMaxMagnitude(680);

CMobblerGestureObserver5x::~CMobblerGestureObserver5x()
	{
	if (iSensrvChannel)
		{
		DoStopObservingL();
		iSensrvChannel->CloseChannel();
		}
	delete iSensrvChannel;
	delete iConditions;
	}

CMobblerGestureObserver* CMobblerGestureObserver5x::NewL()
	{
	CMobblerGestureObserver5x* self = new(ELeave) CMobblerGestureObserver5x;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerGestureObserver5x::CMobblerGestureObserver5x()
	{
	}

void CMobblerGestureObserver5x::ConstructL()
	{
	BaseConstructL();
	
	// Construct the channel
	TSensrvChannelInfo accChannel(GetChannelL());
	iSensrvChannel = CSensrvChannel::NewL(accChannel);
	iSensrvChannel->OpenChannelL();	
	
	// Set up conditional monitoring
	SetupConditionSetL();
	iSensrvChannel->AddConditionL(*iConditions);
	}

void CMobblerGestureObserver5x::SetupConditionSetL()
	{
	// Using an OR set for upper and lower bounds
	iConditions = CSensrvChannelConditionSet::NewL(ESensrvOrConditionSet);
	
	// Create the low pass filter for the high value
	TSensrvAccelerometerAxisData highValLowerBound;
	highValLowerBound.iTimeStamp = 0;
	highValLowerBound.iAxisX = KFilterValue;
	
	TPckgC<TSensrvAccelerometerAxisData> highValLowerBoundPckg(highValLowerBound);
	CSensrvChannelCondition* highFilter =  CSensrvChannelCondition::NewLC(ESensrvSingleLimitCondition, 
			ESensrvOperatorGreaterThan, TSensrvAccelerometerAxisData::EAxisX, highValLowerBoundPckg);
	
	iConditions->AddChannelConditionL(highFilter);
	CleanupStack::Pop(highFilter);
	
	// Create low pass filter condition for low value end.
	TSensrvAccelerometerAxisData lowValUpperBound;
	lowValUpperBound.iTimeStamp = 0;
	lowValUpperBound.iAxisX = (-1 * KFilterValue);
	
	TPckgC<TSensrvAccelerometerAxisData> lowValUpperBoundPckg(lowValUpperBound);
	CSensrvChannelCondition* lowFilter =  CSensrvChannelCondition::NewLC(ESensrvSingleLimitCondition, 
			ESensrvOperatorLessThan, TSensrvAccelerometerAxisData::EAxisX, lowValUpperBoundPckg);
	
	iConditions->AddChannelConditionL(lowFilter);
	CleanupStack::Pop(lowFilter);
	}

TSensrvChannelInfo CMobblerGestureObserver5x::GetChannelL()
	{
	// Create search criteria
	TSensrvChannelInfo accelerometerXyzSearch;
	accelerometerXyzSearch.iChannelType = KSensrvChannelTypeIdAccelerometerXYZAxisData;
	
	// Create a channel finder and list for matched channels.
	CSensrvChannelFinder* sensrvChannelFinder = CSensrvChannelFinder::NewLC(); 
	RSensrvChannelInfoList channelInfoList;
	CleanupClosePushL(channelInfoList);
	
	// Search for the accelerometer channel
	sensrvChannelFinder->FindChannelsL(channelInfoList, accelerometerXyzSearch);
	
	// If there are any matches in the list, use the first match.
	TSensrvChannelInfo matchedChannel;
	const TInt KNumberOfChannels(channelInfoList.Count());
	if (KNumberOfChannels > 0)
		{
		// Found the accelerometer channel
		matchedChannel = channelInfoList[0];
		}
	else
		{
		// Not matched, leave and abort plug-in construction
		User::Leave(KErrNotSupported);
		}
	
	CleanupStack::PopAndDestroy(2, sensrvChannelFinder); // channelInfoList
	return matchedChannel;
	}

void CMobblerGestureObserver5x::DoStartObservingL()
	{
	if (iSensrvChannel)
		{
		// Start listening to data
		iSensrvChannel->StartConditionListeningL(this, KEventsPerCallback, KMaxTimeToCallbackMilliSeconds);
		}
	else
		{
		User::Leave(KErrNotReady);
		}
	}

void CMobblerGestureObserver5x::DoStopObservingL()
	{
	if (iSensrvChannel)
		{
		User::LeaveIfError(iSensrvChannel->StopConditionListening());
		}
	}

	
void CMobblerGestureObserver5x::ConditionMet(CSensrvChannel& aChannel, CSensrvChannelConditionSet& aChannelConditionSet, 
    		TDesC8& aValue)
	{
	if (aChannel.GetChannelInfo().iChannelType == KSensrvChannelTypeIdAccelerometerXYZAxisData)
		{
		// Checking the size because the descriptor stuff looks dodgy
		if (sizeof(TSensrvAccelerometerAxisData) == aValue.Size())
			{
			// Copy into a TPckgBuf
			TPckgBuf<TSensrvAccelerometerAxisData> dataBuffer;
			dataBuffer.Copy(aValue);
	
			TSensrvAccelerometerAxisData accData = dataBuffer();
	
			// Instantiate an event and deliver it to action delegates
			TMobblerGestureEvent event(KAccelerometerSensorUID, Transform(accData.iAxisY),
					Transform(accData.iAxisX), Transform(accData.iAxisZ));
	
			DelegateEvent(event);
			}
		if (iSensrvChannel)
			{
			// For some reason, you have to re-add this after every callback.
			iSensrvChannel->AddConditionL(*iConditions);
			}
		}
	}

void CMobblerGestureObserver5x::ConditionError(CSensrvChannel& /*aChannel*/, 
		TSensrvErrorSeverity /*aError*/)
	{
	// May have missed an event, but let's not worry about it.
	}

void CMobblerGestureObserver5x::GetChannelConditionListenerInterfaceL(TUid /*aInterfaceUid*/, 
		TAny*& /*aInterface*/)
	{
	// Currently unused by sensor service.
	}

/*
 * Transform a Sensrv accelerometer value to an approximation of
 * the RRSensor values so that action objects are able to compare
 * the same values without requiring platform specific parameters.
 * 
 * Sensrv values range from -128 and 128.
 * RRSensor values range from -680 to 680.
 * 
 * Therefore, multiply Sensrv values by KCorrectionFactor to get them almost equivalent.
 */

TInt CMobblerGestureObserver5x::Transform(TInt aValue)
	{
	TReal transformedValue(aValue * KCorrectionFactor);
	
	if (transformedValue > KMaxMagnitude)
		{
		transformedValue = KMaxMagnitude;
		}
	else if (transformedValue < (-1 * KMaxMagnitude))
		{
		transformedValue = (-1 * KMaxMagnitude);
		}
	
	return static_cast<TInt>(transformedValue);
	}

// End of file
