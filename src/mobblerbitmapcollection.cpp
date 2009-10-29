/*
mobblerbitmapcontainer.cpp

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

#include <icl/imagecodecdata.h>

#include <mobbler.mbg>

#include "mobblerappui.h"
#include "mobblerbitmap.h"
#include "mobblerbitmapcollection.h"

#if defined(__SYMBIAN_SIGNED__) && !defined(__WINS__)
_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler_0x2002655A.mif");
_LIT(KPngScrobble, "\\resource\\apps\\mobbler_0x2002655A\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler_0x2002655A\\icon_track.png");
_LIT(KPngAlarmIcon, "\\resource\\apps\\mobbler_0x2002655A\\icon_alarm.png");
_LIT(KPngLastFm, "\\resource\\apps\\mobbler_0x2002655A\\lastfm.png");

_LIT(KDefaultAlbumImage, "\\resource\\apps\\mobbler_0x2002655A\\default_album.gif");
_LIT(KDefaultArtistImage, "\\resource\\apps\\mobbler_0x2002655A\\default_artist.png");
_LIT(KDefaultEventImage, "\\resource\\apps\\mobbler_0x2002655A\\default_event.png");
_LIT(KDefaultUserImage, "\\resource\\apps\\mobbler_0x2002655A\\default_user.png");
_LIT(KDefaultPlaylistImage, "\\resource\\apps\\mobbler_0x2002655A\\default_playlist.png");
_LIT(KDefaultTagImage, "\\resource\\apps\\mobbler_0x2002655A\\default_tag.png");
_LIT(KDefaultTrackImage, "\\resource\\apps\\mobbler_0x2002655A\\default_track.png");
#else
_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler.mif");
_LIT(KPngScrobble, "\\resource\\apps\\mobbler\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler\\icon_track.png");
_LIT(KPngAlarmIcon, "\\resource\\apps\\mobbler\\icon_alarm.png");
_LIT(KPngLastFm, "\\resource\\apps\\mobbler\\lastfm.png");

_LIT(KDefaultAlbumImage, "\\resource\\apps\\mobbler\\default_album.gif");
_LIT(KDefaultArtistImage, "\\resource\\apps\\mobbler\\default_artist.png");
_LIT(KDefaultEventImage, "\\resource\\apps\\mobbler\\default_event.png");
_LIT(KDefaultUserImage, "\\resource\\apps\\mobbler\\default_user.png");
_LIT(KDefaultPlaylistImage, "\\resource\\apps\\mobbler\\default_playlist.png");
_LIT(KDefaultTagImage, "\\resource\\apps\\mobbler\\default_tag.png");
_LIT(KDefaultTrackImage, "\\resource\\apps\\mobbler\\default_track.png");
#endif

const TUid KMusicAppUid = {0x102072C3};

CMobblerBitmapCollection::CBitmapCollectionItem* CMobblerBitmapCollection::CBitmapCollectionItem::NewLC(CMobblerBitmap* aBitmap, TInt aBitmapId)
	{
	CBitmapCollectionItem* self(new(ELeave) CBitmapCollectionItem(aBitmapId));
	CleanupStack::PushL(self);
	self->ConstructL(aBitmap);
	//CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmapCollection::CBitmapCollectionItem::CBitmapCollectionItem(TInt aBitmapId)
	:iId(aBitmapId)
	{
	}

void CMobblerBitmapCollection::CBitmapCollectionItem::ConstructL(CMobblerBitmap* aBitmap)
	{
	iBitmap = aBitmap;
	}

CMobblerBitmapCollection::CBitmapCollectionItem::~CBitmapCollectionItem()
	{
	iBitmap->Close();
	}
		
TInt CMobblerBitmapCollection::CBitmapCollectionItem::Compare(const CBitmapCollectionItem& aLeft, const CBitmapCollectionItem& aRight)
	{
	return Compare(&aLeft.iId, aRight);
	}

TInt CMobblerBitmapCollection::CBitmapCollectionItem::Compare(const TInt* aKey, const CBitmapCollectionItem& aItem)
	{
	return *aKey - aItem.iId;
	}

CMobblerBitmap* CMobblerBitmapCollection::CBitmapCollectionItem::Bitmap() const
	{
	return iBitmap;
	}
	
CMobblerBitmapCollection* CMobblerBitmapCollection::NewL()
	{
	CMobblerBitmapCollection* self(new(ELeave) CMobblerBitmapCollection());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmapCollection::CMobblerBitmapCollection()
	{
	}

void CMobblerBitmapCollection::ConstructL()
	{
	}

CMobblerBitmapCollection::~CMobblerBitmapCollection()
	{
	iBitmaps.ResetAndDestroy();
	}
	
CMobblerBitmap* CMobblerBitmapCollection::BitmapL(MMobblerBitmapObserver& aObserver, TInt aId) const
	{
	CMobblerBitmap* bitmap(NULL);
	
	TInt position(iBitmaps.FindInOrder(aId, CBitmapCollectionItem::Compare));
	
	if (position == KErrNotFound)
		{
		// The bitmap was not found so load it
		switch (aId)
			{
			case EBitmapLastFm:
				bitmap = CMobblerBitmap::NewL(aObserver, KPngLastFm, KImageTypePNGUid);
				break;
			case EBitmapScrobble:
				bitmap = CMobblerBitmap::NewL(aObserver, KPngScrobble, KImageTypePNGUid);
				break;
			case EBitmapTrackIcon:
				bitmap = CMobblerBitmap::NewL(aObserver, KPngTrackIcon, KImageTypePNGUid);
				break;
			case EBitmapAlarmIcon:
				bitmap = CMobblerBitmap::NewL(aObserver, KPngAlarmIcon, KImageTypePNGUid);
				break;
			case EBitmapMore:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerMore, EMbmMobblerMore_mask);
				break;
			case EBitmapLove:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerLove, EMbmMobblerLove_mask);
				break;
			case EBitmapBan:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerBan, EMbmMobblerBan_mask);
				break;
			case EBitmapPlay:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerPlay, EMbmMobblerPlay_mask);
				break;
			case EBitmapNext:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerNext, EMbmMobblerNext_mask);
				break;
			case EBitmapStop:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerStop, EMbmMobblerStop_mask);
				break;
			case EBitmapSpeakerHigh:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerSpeaker_high, EMbmMobblerSpeaker_high_mask);
				break;
			case EBitmapSpeakerLow:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerSpeaker_low, EMbmMobblerSpeaker_low_mask);
				break;
			case EBitmapMusicApp:
				bitmap = CMobblerBitmap::NewL(aObserver, KMusicAppUid);
				break;
			case EBitmapMobblerApp:
				bitmap = CMobblerBitmap::NewL(aObserver, TUid::Uid(KMobblerAppUid));
				break;
			case EBitmapDefaultAlbumImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultAlbumImage);
				break;
			case EBitmapDefaultArtistImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultArtistImage);
				break;
			case EBitmapDefaultEventImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultEventImage);
				break;
			case EBitmapDefaultUserImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultUserImage);
				break;
			case EBitmapDefaultPlaylistImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultPlaylistImage);
				break;
			case EBitmapDefaultTagImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultTagImage);
				break;
			case EBitmapDefaultTrackImage:
				bitmap = CMobblerBitmap::NewL(aObserver, KDefaultTrackImage);
				break;
			default:
				break;
			}
		
		CBitmapCollectionItem* item(CBitmapCollectionItem::NewLC(bitmap, aId));
		TLinearOrder<CBitmapCollectionItem> linearOrder(CBitmapCollectionItem::Compare);
		iBitmaps.InsertInOrderL(item, linearOrder);
		CleanupStack::Pop(item);
		}
	else
		{
		// it has already been created so just return it
		bitmap = iBitmaps[position]->Bitmap();
		bitmap->SetCallbackCancelled(EFalse);
		bitmap->SetObserver(aObserver);
		}
	
	return bitmap;
	}

void CMobblerBitmapCollection::Cancel(CMobblerBitmap* aBitmap) const
	{
	// Cancel all the callbacks for this observer
	const TInt KBitmapCount(iBitmaps.Count());
	for (TInt i(0); i < KBitmapCount; ++i)
		{
		if (iBitmaps[i]->Bitmap() == aBitmap)
			{
			iBitmaps[i]->Bitmap()->SetCallbackCancelled(ETrue);
			break;
			}
		}
	}

// End of file
