/*
mobblerlistitem.cpp

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

#include "mobblerlistcontrol.h"
#include "mobblerlistitem.h"
#include "mobblerstring.h"

CMobblerListItem* CMobblerListItem::NewL(CMobblerListControl& aObserver, const TDesC8& aTitle, const TDesC8& aDescription, const TDesC8& aImageLocation)
	{
	CMobblerListItem* self(new(ELeave) CMobblerListItem(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aTitle, aDescription, aImageLocation);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerListItem::CMobblerListItem(CMobblerListControl& aObserver)
	:iObserver(aObserver)
	{
	}

void CMobblerListItem::ConstructL(const TDesC8& aTitle, const TDesC8& aDescription, const TDesC8& aImageLocation)
	{
	iDescription = CMobblerString::NewL(aDescription);
	iTitle = CMobblerString::NewL(aTitle);
	iImageLocation = aImageLocation.AllocL();
	}

CMobblerListItem::~CMobblerListItem()
	{
	delete iDescription;
	delete iTitle;
	delete iImageLocation;
	iImage->Close();
	delete iId;
	delete iLatitude;
	delete iLongitude;
	}

void CMobblerListItem::SetIdL(const TDesC8& aId)
	{
	iId = aId.AllocL();
	}

const TDesC8& CMobblerListItem::Id() const
	{
	return *iId;
	}

void CMobblerListItem::SetTimeL(const TDesC8& aUTS)
	{
	if (aUTS.Compare(KNullDesC8) == 0)
		{
		iLocalTime = Time::NullTTime();
		}
	else
		{
		TLex8 lex(aUTS);
		TInt uts;
		User::LeaveIfError(lex.Val(uts));
		
		TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
		
		iLocalTime = epoch + TTimeIntervalSeconds(uts) + User::UTCOffset();
		}
	}

TTime CMobblerListItem::TimeLocal() const
	{
	return iLocalTime;
	}

const CMobblerString* CMobblerListItem::Title() const
	{
	return iTitle;
	}

const CMobblerString* CMobblerListItem::Description() const
	{
	return iDescription;
	}

void CMobblerListItem::SetImageLocationL(const TDesC8& aImageLocation)
	{
	delete iImageLocation;
	iImageLocation = aImageLocation.AllocL();
	}

const TDesC8& CMobblerListItem::ImageLocation() const
	{
	return *iImageLocation;
	}

CMobblerBitmap* CMobblerListItem::Image() const
	{
	return iImage;
	}

void CMobblerListItem::SetImageRequested(TBool aRequested)
	{
	iImageRequested = aRequested;
	}

TBool CMobblerListItem::ImageRequested() const
	{
	return iImageRequested;
	}

void CMobblerListItem::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		iImage = CMobblerBitmap::NewL(*this, aData);
		}
	}

void CMobblerListItem::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	iImage->ScaleL(TSize(40, 40));
	}

void CMobblerListItem::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	iObserver.HandleLoadedL();
	}

void CMobblerListItem::SetLatitudeL(const TDesC8& aLatitude)
	{
	delete iLatitude;
	iLatitude = aLatitude.AllocL();
	}

const TDesC8& CMobblerListItem::Latitude() const
	{
	if (iLatitude)
		{
		return *iLatitude;
		}
	
	return KNullDesC8;
	}

void CMobblerListItem::SetLongitudeL(const TDesC8& aLongitude)
	{
	delete iLongitude;
	iLongitude = aLongitude.AllocL();
	}

const TDesC8& CMobblerListItem::Longitude() const
	{
	if (iLongitude)
		{
		return *iLongitude;
		}
	
	return KNullDesC8;
	}

// End of file
