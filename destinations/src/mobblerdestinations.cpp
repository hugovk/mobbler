/*
mobblerdestinations.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#include <aknsettingitemlist.h>
#include <cmdestination.h>
#include <ecom/implementationproxy.h>

#include "mobblerdestinations.h"

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x20026621};
#else
const TInt KImplementationUid = {0xA000BEB6};
#endif

const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerDestinations::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerDestinations* CMobblerDestinations::NewL()
	{
	CMobblerDestinations* self(new(ELeave) CMobblerDestinations());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerDestinations::CMobblerDestinations()
	{
	}

void CMobblerDestinations::ConstructL()
	{
	iCmManager.OpenL();
	}

CMobblerDestinations::~CMobblerDestinations()
	{
	delete iMobility;
	iCmManager.Close();
	}

void CMobblerDestinations::RegisterMobilityL(RConnection& aConnection, MMobblerDestinationsInterfaceObserver* aObserver)
	{
	iMobilityObserver = aObserver;
	
	delete iMobility;
	iMobility = CActiveCommsMobilityApiExt::NewL(aConnection, *this);
	}

void CMobblerDestinations::PreferredCarrierAvailable(TAccessPointInfo /*aOldAPInfo*/, TAccessPointInfo /*aNewAPInfo*/, TBool aIsUpgrade, TBool aIsSeamless)
	{
	// aOldAPInfo contains the current IAP used by the connection.
	// aNewAPInfo contains the newly available IAP that can be used by the connection.

	if (aIsSeamless)
		{
		User::InfoPrint(_L("Seamless start"));	// TODO localise?
		// It is seamless e.g. mobile IP enabled.
		}
	else
		{
		aIsUpgrade ?
			User::InfoPrint(_L("Non-Seamless upgrade")):	// TODO localise?
			User::InfoPrint(_L("Non-Seamless downgrade"));	// TODO localise?
			
		if (aIsUpgrade) 
			{
			// sockets used by the connection should be closed here.
			// We ask to migrate to the Preferred Carrier.
			iMobilityObserver->PreferredCarrierAvailable();
			iMobility->MigrateToPreferredCarrier();
			}
		}
	}

void CMobblerDestinations::NewCarrierActive(TAccessPointInfo /*aNewAPInfo*/, TBool aIsSeamless)
	{
	// aNewAPInfo contains the newly started IAP used now by the connection.
	if (aIsSeamless)
		{
		// It is seamless e.g. mobile IP enabled.
		User::InfoPrint(_L("Seamless active")); // TODO localise?
		}
	else
		{
		User::InfoPrint(_L("Non-Seamless active")); // TODO localise?
		// Sockets used by the connection should be reopened here.
		// We accept the new IAP.
		iMobilityObserver->NewCarrierActive();
		iMobility->NewCarrierAccepted();
		}
	}

void CMobblerDestinations::Error(TInt /*aError*/)
	{
	}

TInt CMobblerDestinations::LoadDestinationListL(CArrayPtr<CAknEnumeratedText>& aDestinations)
	{
	TInt firstIapId(KErrNotFound);
	
	RArray<TUint32> destinations;
	CleanupClosePushL(destinations);
	iCmManager.AllDestinationsL(destinations);
	
	const TInt KDestinationCount(destinations.Count());
	for (TInt i(0); i < KDestinationCount; ++i)
		{
		RCmDestination destination(iCmManager.DestinationL(destinations[i]));
		
		HBufC* name(destination.NameLC());
		
		CAknEnumeratedText* enumText(new(ELeave) CAknEnumeratedText(destinations[i], name));
		CleanupStack::Pop(name);
		CleanupStack::PushL(enumText);
		aDestinations.AppendL(enumText);
		CleanupStack::Pop(enumText);
		
		if (firstIapId == KErrNotFound)
			{
			firstIapId = destinations[i];
			}
		}
	
	CleanupStack::PopAndDestroy(&destinations);
	
	return firstIapId;
	}

// End of file
