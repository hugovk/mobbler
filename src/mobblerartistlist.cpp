/*
mobblerartistlist.cpp

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

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerartistlist.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerwebserviceshelper.h"

_LIT8(KGetTopArtists, "gettopartists");

CMobblerArtistList::CMobblerArtistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerArtistList::ConstructL()
	{
    TRACER_AUTO;
    iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultArtistImage);
    
	iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
    
	switch (iType)
		{
		case EMobblerCommandUserTopArtists:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetTopArtists, iText1->String8(), *this);
			break;
		case EMobblerCommandRecommendedArtists:
			iAppUi.LastFmConnection().QueryLastFmL(iType, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, *this);
			break;
		case EMobblerCommandSimilarArtists:
			iAppUi.LastFmConnection().SimilarL(iType, iText1->String8(), KNullDesC8, *this);
			break;
		case EMobblerCommandTagTopArtists:
			iAppUi.LastFmConnection().WebServicesCallL(KTag, KGetTopArtists, iText1->String8(), *this);
			break;
		case EMobblerCommandSearchArtist:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KSearch, iText1->String8(), *this);
			break;
		default:
			break;
		}
	}

CMobblerArtistList::~CMobblerArtistList()
	{
    TRACER_AUTO;
	delete iWebServicesHelper;
	}

CMobblerListControl* CMobblerArtistList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandRadioStart:
			iAppUi.RadioStartL(EMobblerCommandRadioArtist, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandSimilarArtists:		// intentional fall-through
		case EMobblerCommandArtistEvents:		// intentional fall-through
		case EMobblerCommandArtistShoutbox:		// intentional fall-through
		case EMobblerCommandArtistTopAlbums:	// intentional fall-through
		case EMobblerCommandArtistTopTracks:	// intentional fall-through
		case EMobblerCommandArtistTopTags:		// intentional fall-through
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, aCommand, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistShare:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			iWebServicesHelper->ArtistShareL(*track);
			track->Release();
			}
			break;
		case EMobblerCommandArtistAddTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			iWebServicesHelper->AddTagL(*track, aCommand);
			track->Release();
			}
			break;
		case EMobblerCommandArtistRemoveTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse));
			iWebServicesHelper->ArtistRemoveTagL(*track);
			track->Release();
			}
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerArtistList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	aCommands.AppendL(EMobblerCommandRadioStart);
	
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandSimilarArtists);
	aCommands.AppendL(EMobblerCommandArtistEvents);
	aCommands.AppendL(EMobblerCommandArtistShoutbox);
	aCommands.AppendL(EMobblerCommandArtistTopAlbums);
	aCommands.AppendL(EMobblerCommandArtistTopTracks);
	aCommands.AppendL(EMobblerCommandArtistTopTags);
	
	aCommands.AppendL(EMobblerCommandTag);
	aCommands.AppendL(EMobblerCommandArtistAddTag);
	aCommands.AppendL(EMobblerCommandArtistRemoveTag);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	}

void CMobblerArtistList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, CMobblerLastFmConnection::TTransactionError /*aError*/)
	{
    TRACER_AUTO;
	}

void CMobblerArtistList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	switch (iType)
		{
		case EMobblerCommandUserTopArtists:
			CMobblerParser::ParseTopArtistsL(aXml, *this, iList);
			break;
		case EMobblerCommandRecommendedArtists:
			CMobblerParser::ParseRecommendedArtistsL(aXml, *this, iList);
			break;
		case EMobblerCommandSimilarArtists:
			CMobblerParser::ParseSimilarArtistsL(aXml, *this, iList);
			break;
		case EMobblerCommandTagTopArtists:
			CMobblerParser::ParseTopArtistsL(aXml, *this, iList);
			break;
		case EMobblerCommandSearchArtist:
			CMobblerParser::ParseSearchArtistL(aXml, *this, iList);
			break;
		default:
			break;
		}
	}

// End of file
