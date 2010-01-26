/*
mobblerbitmapcollection.h

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

#ifndef __MOBBLERBITMAPCOLLECTION_H__
#define __MOBBLERBITMAPCOLLECTION_H__

#include <e32base.h>

#include "mobblerbitmap.h"

class CMobblerBitmapCollection : public CBase, public MMobblerBitmapObserver
	{
public:
	enum TBitmapID
		{
		EBitmapLastFm,
		EBitmapScrobble,
		EBitmapTrackIcon,
		EBitmapAlarmIcon,
		EBitmapHarddiskIcon,
		EBitmapMore,
		EBitmapLove,
		EBitmapBan,
		EBitmapPlay,
		EBitmapNext,
		EBitmapStop,
		EBitmapSpeakerHigh,
		EBitmapSpeakerLow,
		EBitmapMusicApp,
		EBitmapMobblerApp,
		EBitmapDefaultAlbumImage,
		EBitmapDefaultArtistImage,
		EBitmapDefaultEventImage,
		EBitmapDefaultUserImage,
		EBitmapDefaultPlaylistImage,
		EBitmapDefaultTagImage,
		EBitmapDefaultTrackImage
		};
	
private:
	class CBitmapCollectionItem : public CBase
		{
	public:
		static CBitmapCollectionItem* NewLC(CMobblerBitmap* aBitmap, TInt aBitmapId);
		~CBitmapCollectionItem();
		
		CMobblerBitmap* Bitmap() const;
		
		static TInt Compare(const CBitmapCollectionItem& aLeft, const CBitmapCollectionItem& aRight);
		static TInt Compare(const TInt* aKey, const CBitmapCollectionItem& aItem);
		
	private:
		CBitmapCollectionItem(TInt aBitmapId);
		void ConstructL(CMobblerBitmap* aBitmap);
		
	private:
		TInt iId;
		CMobblerBitmap* iBitmap;
		};
	
public:
	static CMobblerBitmapCollection* NewL();
	~CMobblerBitmapCollection();
	
	CMobblerBitmap* BitmapL(MMobblerBitmapObserver& aObserver, TInt aBitmap) const;
	void Cancel(CMobblerBitmap* aBitmap) const;
	
private:
	CMobblerBitmapCollection();
	void ConstructL();
	
private: // from MMobblerBitmapObserver
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
private:
	mutable RPointerArray<CBitmapCollectionItem> iBitmaps;
	};

#endif

// End of file
