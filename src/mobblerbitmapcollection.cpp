/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010, 2012  Hugo van Kemenade

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

#include <icl/imagecodecdata.h>

#include "mobblerappui.h"
#include "mobblerbitmap.h"
#include "mobblerbitmapcollection.h"
#include "mobblertracer.h"

#include <mobblerbuttons.mbg>

#ifdef __SYMBIAN_SIGNED__
#include <mobbler.mbg>
#ifdef __WINS__
_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler.mif");
_LIT(KMobblerButtonsMifFile, "\\resource\\apps\\mobblerbuttons_5th.mif");
#else
_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler_0x20038513.mif");
_LIT(KMobblerButtonsMifFile, "\\resource\\apps\\mobblerbuttons_0x20038513.mif");
#endif // __WINS__
#else // !__SYMBIAN_SIGNED__
#include <ssmobbler.mbg>
#ifdef __WINS__
_LIT(KMobblerMifFile, "\\resource\\apps\\ssmobbler.mif");
_LIT(KMobblerButtonsMifFile, "\\resource\\apps\\mobblerbuttons_5th.mif");
#else
_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler_0xA0007648.mif");
_LIT(KMobblerButtonsMifFile, "\\resource\\apps\\mobblerbuttons_0xA0007648.mif");
#endif // __WINS__
#endif // __SYMBIAN_SIGNED__

#if defined (__SYMBIAN_SIGNED__) && !defined(__WINS__)
_LIT(KPngScrobble, "\\resource\\apps\\mobbler_0x20038513\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler_0x20038513\\icon_track.png");
_LIT(KPngAlarmIcon, "\\resource\\apps\\mobbler_0x20038513\\icon_alarm.png");
_LIT(KPngLastFm, "\\resource\\apps\\mobbler_0x20038513\\lastfm.png");
_LIT(KPngSubscriberIcon, "\\resource\\apps\\mobbler_0x20038513\\icon_subscriber.png");

_LIT(KDefaultAlbumImage, "\\resource\\apps\\mobbler_0x20038513\\default_album.gif");
_LIT(KDefaultArtistImage, "\\resource\\apps\\mobbler_0x20038513\\default_artist.png");
_LIT(KDefaultEventImage, "\\resource\\apps\\mobbler_0x20038513\\default_event.png");
_LIT(KDefaultUserImage, "\\resource\\apps\\mobbler_0x20038513\\default_user.png");
_LIT(KDefaultPlaylistImage, "\\resource\\apps\\mobbler_0x20038513\\default_playlist.png");
_LIT(KDefaultTagImage, "\\resource\\apps\\mobbler_0x20038513\\default_tag.png");
_LIT(KDefaultTrackImage, "\\resource\\apps\\mobbler_0x20038513\\default_track.png");
#else
_LIT(KPngScrobble, "\\resource\\apps\\mobbler\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler\\icon_track.png");
_LIT(KPngAlarmIcon, "\\resource\\apps\\mobbler\\icon_alarm.png");
_LIT(KPngLastFm, "\\resource\\apps\\mobbler\\lastfm.png");
_LIT(KPngSubscriberIcon, "\\resource\\apps\\mobbler\\icon_subscriber.png");

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
    TRACER_AUTO;
	CMobblerBitmapCollection* self(new(ELeave) CMobblerBitmapCollection());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmapCollection::CMobblerBitmapCollection()
	{
    TRACER_AUTO;
	}

void CMobblerBitmapCollection::ConstructL()
	{
    TRACER_AUTO;
	// load the list box default images because if we
	// populate the list box before the image decodes
	// the phone will hang.
	BitmapL(*this, EBitmapDefaultAlbumImage);
	BitmapL(*this, EBitmapDefaultArtistImage);
	BitmapL(*this, EBitmapDefaultEventImage);
	BitmapL(*this, EBitmapDefaultUserImage);
	BitmapL(*this, EBitmapDefaultPlaylistImage);
	BitmapL(*this, EBitmapDefaultTagImage);
	BitmapL(*this, EBitmapDefaultTrackImage);
	}

CMobblerBitmapCollection::~CMobblerBitmapCollection()
	{
    TRACER_AUTO;
    
    for (TInt i(0); i < iBitmaps.Count(); ++i)
    	{
		iBitmaps[i]->Bitmap()->RemoveObserver(this);
    	}
    
	iBitmaps.ResetAndDestroy();
	}

CMobblerBitmap* CMobblerBitmapCollection::BitmapL(MMobblerBitmapObserver& aObserver, TInt aId) const
	{
    TRACER_AUTO;
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
			case EBitmapSubscriberIcon:
				bitmap = CMobblerBitmap::NewL(aObserver, KPngSubscriberIcon, KImageTypePNGUid);
				break;
			case EBitmapHarddiskIcon:
#ifdef __SYMBIAN_SIGNED__
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerDrive_harddisk, EMbmMobblerDrive_harddisk_mask);
#else
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmSsmobblerDrive_harddisk, EMbmSsmobblerDrive_harddisk_mask);
#endif
				break;
			case EBitmapOnTour:
#ifdef __SYMBIAN_SIGNED__
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerOn_tour, EMbmMobblerOn_tour_mask);
#else
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmSsmobblerOn_tour, EMbmSsmobblerOn_tour_mask);
#endif
				break;
			case EBitmapMore:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsMore, EMbmMobblerbuttonsMore_mask);
				break;
			case EBitmapLove:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsLove, EMbmMobblerbuttonsLove_mask);
				break;
			case EBitmapBan:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsBan, EMbmMobblerbuttonsBan_mask);
				break;
			case EBitmapPlay:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsPlay, EMbmMobblerbuttonsPlay_mask);
				break;
			case EBitmapPause:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsPause, EMbmMobblerbuttonsPause_mask);
				break;
			case EBitmapNext:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsNext, EMbmMobblerbuttonsNext_mask);
				break;
			case EBitmapStop:
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerButtonsMifFile, EMbmMobblerbuttonsStop, EMbmMobblerbuttonsStop_mask);
				break;
			case EBitmapSpeakerHigh:
#ifdef __SYMBIAN_SIGNED__
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerSpeaker_high, EMbmMobblerSpeaker_high_mask);
#else
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmSsmobblerSpeaker_high, EMbmSsmobblerSpeaker_high_mask);
#endif
				break;
			case EBitmapSpeakerLow:
#ifdef __SYMBIAN_SIGNED__
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmMobblerSpeaker_low, EMbmMobblerSpeaker_low_mask);
#else
				bitmap = CMobblerBitmap::NewL(aObserver, KMobblerMifFile, EMbmSsmobblerSpeaker_low, EMbmSsmobblerSpeaker_low_mask);
#endif
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
		bitmap->AddObserver(&aObserver);
		}
	
	return bitmap;
	}

void CMobblerBitmapCollection::Cancel(CMobblerBitmap* aBitmap) const
	{
    TRACER_AUTO;
	// Cancel all the callbacks for this observer
	const TInt KBitmapCount(iBitmaps.Count());
	for (TInt i(0); i < KBitmapCount; ++i)
		{
		if (iBitmaps[i]->Bitmap() == aBitmap)
			{
			iBitmaps[i]->Bitmap()->RemoveAllObservers();
			break;
			}
		}
	}

void CMobblerBitmapCollection::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	}

void CMobblerBitmapCollection::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	}

// End of file
