/*
mobblerlocation.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2010  Michael Coffey

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

void CMobblerLocation::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		// Parse the XML
		CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
		CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));

		_LIT8(KAccuracy, "accuracy");
		_LIT8(KLatitude, "latitude");
		_LIT8(KLongitude, "longitude");
		
		iObserver.HandleLocationCompleteL(domFragment->AsElement().Element(KAccuracy)->Content(),
											domFragment->AsElement().Element(KLatitude)->Content(),
											domFragment->AsElement().Element(KLongitude)->Content(),
											domFragment->AsElement().Element(KName)->Content());
		
		CleanupStack::PopAndDestroy(2);
		}
	else
		{
		LOG2(_L8("Transaction error"), aTransactionError);
		}
	}

// End of file
