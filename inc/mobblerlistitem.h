/*
mobblerlistitem.h

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

#ifndef __MOBBLERLISTITEM_H__
#define __MOBBLERLISTITEM_H__

#include <e32base.h>

#include "mobblerbitmap.h"

class CMobblerListControl;
class CMobblerString;

class CMobblerListItem : public CBase, public MMobblerFlatDataObserver, public MMobblerBitmapObserver
	{
public:
	static CMobblerListItem* NewL(CMobblerListControl& aObserver, const TDesC8& aTitle, const TDesC8& aDescription, const TDesC8& aImageLocation);
	~CMobblerListItem();
	
	const CMobblerString* Title() const;
	const CMobblerString* Description() const;
	
	void SetImageLocationL(const TDesC8& aImageLocation);
	const TDesC8& ImageLocation() const;
	
	void SetIdL(const TDesC8& aId);
	const TDesC8& Id() const;
	
	void SetTimeL(const TDesC8& aUTS);
	TTime TimeLocal() const;

	void SetImageRequested(TBool aRequested);
	TBool ImageRequested() const;
	
	CMobblerBitmap* Image() const;
	
	void SetLatitudeL(const TDesC8& aLatitude);
	const TDesC8& Latitude() const;
	
	void SetLongitudeL(const TDesC8& aLongitude);
	const TDesC8& Longitude() const;
	
private:
	void DataL(const TDesC8& aData, TInt aTransactionError);
	
private:
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
private:
	CMobblerListItem(CMobblerListControl& aObserver);
	void ConstructL(const TDesC8& aTitle, const TDesC8& aDescription, const TDesC8& aImageLocation);
	
private:
	CMobblerListControl& iObserver;
	
	CMobblerString* iTitle;
	CMobblerString* iDescription;
	HBufC8* iImageLocation;
	HBufC8* iId;
	
	TBool iImageRequested;
	CMobblerBitmap* iImage;
	
	TTime iLocalTime;
	
	HBufC8* iLatitude;
	HBufC8* iLongitude;
	};

#endif // __MOBBLERLISTITEM_H__

// End of file
