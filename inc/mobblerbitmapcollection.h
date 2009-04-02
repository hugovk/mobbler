/*
mobblerbitmapcollection.h

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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

#ifndef __MOBBLERBITMAPCOLLECTION_H__
#define __MOBBLERBITMAPCOLLECTION_H__

#include <e32base.h>

class CMobblerBitmap;
class MMobblerBitmapObserver;

class CMobblerBitmapCollection : public CBase
	{
public:
	enum TBitmapID
		{
		EBitmapLastFM,
		EBitmapScrobble,
		EBitmapTrackIcon,
		EBitmapMore,
		EBitmapLove,
		EBitmapBan,
		EBitmapPlay,
		EBitmapNext,
		EBitmapStop,
		EBitmapSpeakerHigh,
		EBitmapSpeakerLow,
		EBitmapMusicApp,
		EBitmapMobblerApp
		};
	
private:
	class CBitmapCollectionItem : public CBase
		{
	public:
		static CBitmapCollectionItem* NewLC(CMobblerBitmap* aBitmap, TInt aBitmapId);
		~CBitmapCollectionItem();
		
		CMobblerBitmap& Bitmap() const;
		
		static TInt Compare(const CBitmapCollectionItem& aLeft, const CBitmapCollectionItem& aRight);
		static TInt Compare(const TInt* aKey, const CBitmapCollectionItem& aItem);
		
	private:
		CBitmapCollectionItem(TInt aBitmapId);
		void ConstructL(CMobblerBitmap* aBitmap);
		
	private:
		TInt iID;
		CMobblerBitmap* iBitmap;
		};
	
public:
	static CMobblerBitmapCollection* NewL();
	~CMobblerBitmapCollection();
	
	CMobblerBitmap& BitmapL(MMobblerBitmapObserver& aObserver, TInt aBitmap) const;
	
private:
	CMobblerBitmapCollection();
	void ConstructL();
	
private:
	mutable RPointerArray<CBitmapCollectionItem> iBitmaps;
	};

#endif

