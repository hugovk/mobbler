/*
mobblertracklist.cpp

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

#include <aknquerydialog.h>
#include <sendomfragment.h>
#include <senxmlutils.h> 

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"

_LIT8(KGetTopTracks, "gettoptracks");
const TInt KAverageTrackLength(217); // == 3:37, median track length from two users' libraries

CMobblerTrackList::CMobblerTrackList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerTrackList::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultTrackImage);
	
	iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);

	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetTopTracks, iText1->String8(), *this);
			break;
		case EMobblerCommandUserTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetTopTracks, iText1->String8(), *this);
			break;
		case EMobblerCommandRecentTracks:
			iAppUi.LastFmConnection().RecentTracksL(iText1->String8(), *this);
			break;
		case EMobblerCommandSimilarTracks:
			iAppUi.LastFmConnection().SimilarL(iType, iText1->String8(), iText2->String8(), *this);
			break;
		case EMobblerCommandPlaylistFetchUser:
			iAppUi.LastFmConnection().PlaylistFetchUserL(iText2->String8(), *this);
			break;
		case EMobblerCommandPlaylistFetchAlbum:
			if (iText2->String8().Length() > 10)
				{
				// This is a MusicBrainz ID so fetch the Last.fm ID before getting the playlist
				delete iAlbumInfoObserver;
				iAlbumInfoObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, EFalse);
				iAppUi.LastFmConnection().GetInfoL(EMobblerCommandAlbumGetInfo, KNullDesC8, KNullDesC8, KNullDesC8, iText2->String8(), *iAlbumInfoObserver);
				}
			else
				{
				// This is the Last.fm ID so just fetch the playlist using it
				iAppUi.LastFmConnection().PlaylistFetchAlbumL(iText2->String8(), *this);
				}
			break;
		case EMobblerCommandSearchTrack:
			iAppUi.LastFmConnection().WebServicesCallL(KTrack, KSearch, iText1->String8(), *this);
			break;
		case EMobblerCommandScrobbleLog:
			{
			iAsyncCallBack = new(ELeave) CAsyncCallBack(CActive::EPriorityStandard);
			iCallBack = TCallBack(CMobblerTrackList::ViewScrobbleLogCallBackL, this);
			iAsyncCallBack->Set(iCallBack);
			iAsyncCallBack->Call();
			}
			break;
		default:
			break;
		}
	}

CMobblerTrackList::~CMobblerTrackList()
	{
    TRACER_AUTO;
	delete iAsyncCallBack;
	delete iAlbumInfoObserver;
	delete iWebServicesHelper;
	delete iLoveObserver;
	delete iLyricsObserver;
	}

void CMobblerTrackList::GetTrackDetails(TPtrC8& aArtist, TPtrC8& aAlbum, TPtrC8& aTitle)
	{
    TRACER_AUTO;
    GetTrackDetails(iListBox->CurrentItemIndex(), aArtist, aAlbum, aTitle);
	}

void CMobblerTrackList::GetTrackDetails(const TInt aItemIndex, TPtrC8& aArtist, TPtrC8& aAlbum, TPtrC8& aTitle)
	{
    TRACER_AUTO;
	if (iType == EMobblerCommandScrobbleLog)
		{
		if (iAppUi.LastFmConnection().ScrobbleLogCount() > aItemIndex
				&& iAppUi.LastFmConnection().ScrobbleLogCount() > 0)
			{
			aArtist.Set(iAppUi.LastFmConnection().ScrobbleLogItem(aItemIndex).Artist().String8());
			aAlbum.Set(iAppUi.LastFmConnection().ScrobbleLogItem(aItemIndex).Album().String8());
			aTitle.Set(iAppUi.LastFmConnection().ScrobbleLogItem(aItemIndex).Title().String8());
			}
		}
	else
		{
		if (iList.Count() > 0)
			{
			aTitle.Set(iList[aItemIndex]->Title()->String8());
			
			if (iType == EMobblerCommandArtistTopTracks)
				{
				aArtist.Set(iText1->String8());
				}
			else
				{
				aArtist.Set(iList[aItemIndex]->Description()->String8());
				}
			}
		}
	}

CMobblerListControl* CMobblerTrackList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	TPtrC8 artist(KNullDesC8);
	TPtrC8 album(KNullDesC8);
	TPtrC8 title(KNullDesC8);
	
	GetTrackDetails(artist, album, title);
	
	switch(aCommand)
		{
		case EMobblerCommandTrackLove:
			delete iLoveObserver;
			iLoveObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().QueryLastFmL(aCommand, artist, KNullDesC8, title, KNullDesC8, *iLoveObserver);
			break;
		case EMobblerCommandTrackScrobble:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, album, KNullDesC8, KNullDesC8, KNullDesC8, KAverageTrackLength, KNullDesC8, EFalse));
			TTime now;
			now.UniversalTime();
			track->SetStartTimeUTC(now);
			iAppUi.LastFmConnection().ScrobbleTrackL(track);
			track->Release();
			}
			break;
		case EMobblerCommandAlbumScrobble:
			{
			// Get the list box items model
			MDesCArray* listArray(iListBox->Model()->ItemTextArray());
			CDesCArray* itemArray(static_cast<CDesCArray*>(listArray));
			
			// Number of items in the list
			const TInt KCount(itemArray->Count());
			
			// Set up the initial scrobble time
			TTime scrobbleTime;
			scrobbleTime.UniversalTime();
			scrobbleTime -= (TTimeIntervalSeconds)(KCount * KAverageTrackLength);
			
			// Get the album name
			TBuf8<KMaxMobblerTextSize> albumName;
			HBufC* albumBuf(NameL());
			albumName.Append(*albumBuf);
			delete albumBuf;
			
			// Scrobble each track
			for (TInt i(0); i < KCount; ++i)
				{
				GetTrackDetails(i, artist, album, title);
				CMobblerTrack* track(CMobblerTrack::NewL(artist, title, albumName, KNullDesC8, KNullDesC8, KNullDesC8, KAverageTrackLength, KNullDesC8, EFalse));
				
				scrobbleTime += (TTimeIntervalSeconds)KAverageTrackLength;
				track->SetStartTimeUTC(scrobbleTime);
				
				iAppUi.LastFmConnection().ScrobbleTrackL(track, i+1 == KCount);
				track->Release();
				}
			}
			break;
		case EMobblerCommandTrackAddTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			iWebServicesHelper->AddTagL(*track, aCommand);
			track->Release();
			}
			break;
		case EMobblerCommandTrackRemoveTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			iWebServicesHelper->TrackRemoveTagL(*track);
			track->Release();
			}
			break;
		case EMobblerCommandTrackLyrics:
			delete iLyricsObserver;
			iLyricsObserver = CMobblerFlatDataObserverHelper::NewL(
									iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().FetchLyricsL(artist, title, *iLyricsObserver);
			break;
		case EMobblerCommandTrackShare:
		case EMobblerCommandArtistShare:
		case EMobblerCommandPlaylistAddTrack:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, album, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			
			switch (aCommand)
				{
				case EMobblerCommandTrackShare: iWebServicesHelper->TrackShareL(*track); break;
				case EMobblerCommandArtistShare: iWebServicesHelper->ArtistShareL(*track); break;
				case EMobblerCommandPlaylistAddTrack: iWebServicesHelper->PlaylistAddL(*track); break;
				}
				
			track->Release();
			}
			break;
		case EMobblerCommandScrobbleLogRemove:
			{
			// Get the list box items model
			MDesCArray* listArray(iListBox->Model()->ItemTextArray());
			CDesCArray* itemArray(static_cast<CDesCArray*>(listArray));
			
			// Number of items in the list
			const TInt KCount(itemArray->Count());
			
			// Validate index then delete
			const TInt KIndex(iListBox->CurrentItemIndex());
			if (KIndex >= 0 && KIndex < KCount)
				{
				// Remove the item from the scrobble log
				iAppUi.LastFmConnection().RemoveScrobbleLogItemL(KIndex);
				
				// remove the item from out list box
				itemArray->Delete(KIndex, 1);
				AknListBoxUtils::HandleItemRemovalAndPositionHighlightL(iListBox, KIndex, ETrue);
				iListBox->DrawNow();
				}
			}
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerTrackList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
    
	TPtrC8 artist(KNullDesC8);
	TPtrC8 album(KNullDesC8);
	TPtrC8 title(KNullDesC8);
	
	GetTrackDetails(artist, album, title);
	
	aCommands.AppendL(EMobblerCommandTrackLove);
	aCommands.AppendL(EMobblerCommandTrackScrobble);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandTrackShare);
	if (album.Length() != 0)
		{
		// we know this tracks album so we can share it
		aCommands.AppendL(EMobblerCommandAlbumShare);
		}
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandTag);
	aCommands.AppendL(EMobblerCommandTrackAddTag);
	aCommands.AppendL(EMobblerCommandTrackRemoveTag);
	
	aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
	
	if (iType == EMobblerCommandScrobbleLog)
		{
		aCommands.AppendL(EMobblerCommandScrobbleLogRemove);
		}
	else if (iType == EMobblerCommandPlaylistFetchAlbum)
		{
		aCommands.AppendL(EMobblerCommandAlbumScrobble);
		}
	
	aCommands.AppendL(EMobblerCommandTrackLyrics);
	}

void CMobblerTrackList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aObserver == iAlbumInfoObserver)
		{
		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			// Parse the XML
			CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
			CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
			
			if (aObserver == iAlbumInfoObserver)
				{
				iAppUi.LastFmConnection().PlaylistFetchAlbumL(domFragment->AsElement().Element(KAlbum)->Element(KId)->Content(), *this);
				}
			
			CleanupStack::PopAndDestroy(2);
			}
		else
			{
			// TODO
			}
		}
	else if (aObserver == iLoveObserver)
		{
		// Do nothing
		}
	else if (aObserver == iLyricsObserver)
		{
		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			iAppUi.ShowLyricsL(aData);
			}
		}
	}

TInt CMobblerTrackList::ViewScrobbleLogCallBackL(TAny* aPtr)
	{
    TRACER_AUTO;
	static_cast<CMobblerTrackList*>(aPtr)->CMobblerListControl::DataL(KNullDesC8, CMobblerLastFmConnection::ETransactionErrorNone);
	return KErrNone;
	}

void CMobblerTrackList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			CMobblerParser::ParseArtistTopTracksL(aXml, *this, iList);
			break;
		case EMobblerCommandUserTopTracks:
			CMobblerParser::ParseUserTopTracksL(aXml, *this, iList);
			break;
		case EMobblerCommandRecentTracks:
			CMobblerParser::ParseRecentTracksL(aXml, *this, iList);
			break;
		case EMobblerCommandSimilarTracks:
			CMobblerParser::ParseSimilarTracksL(aXml, *this, iList);
			break;
		case EMobblerCommandPlaylistFetchUser:
		case EMobblerCommandPlaylistFetchAlbum:
			CMobblerParser::ParsePlaylistL(aXml, *this, iList);
			break;
		case EMobblerCommandSearchTrack:
			CMobblerParser::ParseSearchTrackL(aXml, *this, iList);
			break;
		case EMobblerCommandScrobbleLog:
			{
			const TInt KScrobbleLogCount(iAppUi.LastFmConnection().ScrobbleLogCount());
			for (TInt i(0); i < KScrobbleLogCount; ++i)
				{
				// Add an item to the list box
				CMobblerListItem* item(CMobblerListItem::NewL(*this,
																iAppUi.LastFmConnection().ScrobbleLogItem(i).Title().String8(),
																iAppUi.LastFmConnection().ScrobbleLogItem(i).Artist().String8(),
																KNullDesC8));

				CleanupStack::PushL(item);
				iList.AppendL(item);
				CleanupStack::Pop(item);
				}
			}
			break;
		default:
			break;
		}
	}

// End of file
