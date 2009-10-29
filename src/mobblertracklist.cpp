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

#include <sendomfragment.h>
#include <senxmlutils.h> 

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"
#include "mobblerwebserviceshelper.h"

CMobblerTrackList::CMobblerTrackList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerTrackList::ConstructL()
	{
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultTrackImage);

	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(_L8("artist"), _L8("gettoptracks"), iText1->String8(), *this);
			break;
		case EMobblerCommandUserTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("gettoptracks"), iText1->String8(), *this);
			break;
		case EMobblerCommandRecentTracks:
			iAppUi.LastFmConnection().RecentTracksL(iText1->String8(), *this);
			break;
		case EMobblerCommandSimilarTracks:
			iAppUi.LastFmConnection().SimilarTracksL(iText1->String8(), iText2->String8(), *this);
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
				iAppUi.LastFmConnection().AlbumGetInfoL(iText2->String8(), *iAlbumInfoObserver);
				}
			else
				{
				// This is the Last.fm ID so just fetch the playlist using it
				iAppUi.LastFmConnection().PlaylistFetchAlbumL(iText2->String8(), *this);
				}
			break;
		case EMobblerCommandSearchTrack:
			iAppUi.LastFmConnection().WebServicesCallL(_L8("track"), _L8("search"), iText1->String8(), *this);
			break;
		default:
			break;
		}
	}

CMobblerTrackList::~CMobblerTrackList()
	{
	delete iAlbumInfoObserver;
	delete iWebServicesHelper;
	}

CMobblerListControl* CMobblerTrackList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	TPtrC8 artist(KNullDesC8);
	TPtrC8 title(KNullDesC8);
	
	if (iList.Count() > 0)
		{
		title.Set(iList[iListBox->CurrentItemIndex()]->Title()->String8());
		
		if (iType == EMobblerCommandArtistTopTracks)
			{
			artist.Set(iText1->String8());
			}
		else
			{
			artist.Set(iList[iListBox->CurrentItemIndex()]->Description()->String8());
			}
		}
	
	switch(aCommand)
		{
		case EMobblerCommandTrackLove:
			iAppUi.LastFmConnection().TrackLoveL(artist, title);
			break;
		case EMobblerCommandTrackShare:
		case EMobblerCommandArtistShare:
		case EMobblerCommandPlaylistAddTrack:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8));
			
			delete iWebServicesHelper;
			iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
			
			switch (aCommand)
				{
				case EMobblerCommandTrackShare: iWebServicesHelper->TrackShareL(*track); break;
				case EMobblerCommandArtistShare: iWebServicesHelper->ArtistShareL(*track); break;
				case EMobblerCommandPlaylistAddTrack: iWebServicesHelper->PlaylistAddL(*track); break;
				}
				
			track->Release();
			}
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerTrackList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandTrackLove);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandTrackShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
	}

void CMobblerTrackList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if (aObserver == iAlbumInfoObserver)
			{
			// Create the XML reader and DOM fragment and associate them with each other
			CSenXmlReader* xmlReader(CSenXmlReader::NewL());
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment(CSenDomFragment::NewL());
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			// Parse the XML into the DOM fragment
			xmlReader->ParseL(aData);
			
			iAppUi.LastFmConnection().PlaylistFetchAlbumL(domFragment->AsElement().Element(_L8("album"))->Element(_L8("id"))->Content(), *this);
			
			CleanupStack::PopAndDestroy(2);
			}
		}
	else
		{
		// TODO
		}
	}

void CMobblerTrackList::ParseL(const TDesC8& aXML)
	{
	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			CMobblerParser::ParseArtistTopTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandUserTopTracks:
			CMobblerParser::ParseUserTopTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandRecentTracks:
			CMobblerParser::ParseRecentTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandSimilarTracks:
			CMobblerParser::ParseSimilarTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandPlaylistFetchUser:
		case EMobblerCommandPlaylistFetchAlbum:
			CMobblerParser::ParsePlaylistL(aXML, *this, iList);
			break;
		case EMobblerCommandSearchTrack:
			CMobblerParser::ParseSearchTrackL(aXML, *this, iList);
			break;
		default:
			break;
		}
	}

// End of file
