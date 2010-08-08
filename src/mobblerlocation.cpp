/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2010  Michael Coffey
Copyright (C) 2010  Hugo van Kemenade

http://code.google.com/p/mobbler

This file is part of Mobbler.

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sendomfragment.h>

#include "mobblerappui.h"
#include "mobblerliterals.h"
#include "mobblerlocation.h"
#include "mobblerlogging.h"
#include "mobblertracer.h"
#include "mobblerutility.h"

CMobblerLocation* CMobblerLocation::NewL(MMobblerLocationObserver& aObserver)
	{
	TRACER_AUTO;
	CMobblerLocation* self(new(ELeave) CMobblerLocation(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLocation::CMobblerLocation(MMobblerLocationObserver& aObserver)
	:CActive(CActive::EPriorityStandard), iNetworkInfoPckg(iNetworkInfo), iObserver(aObserver)
	{
	TRACER_AUTO;
	CActiveScheduler::Add(this);
	}

void CMobblerLocation::ConstructL()
	{
	TRACER_AUTO;
	iTelephony = CTelephony::NewL();
	}

CMobblerLocation::~CMobblerLocation()
	{
	TRACER_AUTO;
	Cancel();
	delete iTelephony;
	}

void CMobblerLocation::GetLocationL()
	{
	TRACER_AUTO;
	iTelephony->GetCurrentNetworkInfo(iStatus, iNetworkInfoPckg);
	SetActive();
	}

void CMobblerLocation::RunL()
	{
	TRACER_AUTO;
	if (iStatus.Int() == KErrNone)
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().GetLocationL(iNetworkInfoPckg(), *this);
		}
	}

void CMobblerLocation::DoCancel()
	{
	TRACER_AUTO;
	iTelephony->CancelAsync(CTelephony::EGetCurrentNetworkInfoCancel);
	}

void CMobblerLocation::DataL(const TDesC8& aData, TInt aTransactionError)
	{
	TRACER_AUTO;
	DUMPDATA(aData, _L("location.xml"));
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		// Parse the XML
		CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
		CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));

//		_LIT8(KAccuracy, "accuracy");
		_LIT8(KLatitude, "latitude");
		_LIT8(KLongitude, "longitude");
		
		iObserver.HandleLocationCompleteL(/*domFragment->AsElement().Element(KAccuracy)->Content()*/
											KNullDesC8,
											domFragment->AsElement().Element(KLatitude)->Content(),
											domFragment->AsElement().Element(KLongitude)->Content(),
											/*domFragment->AsElement().Element(KName)->Content()*/
											KNullDesC8);
		
		CleanupStack::PopAndDestroy(2);
		}
	else
		{
		LOG2(_L8("Transaction error"), aTransactionError);
		}
	}

// End of file
