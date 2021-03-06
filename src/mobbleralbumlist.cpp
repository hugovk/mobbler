/*
mobbleralbumlist.cpp

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
#include "mobbleralbumlist.h"
#include "mobblerbitmapcollection.h"
#include "mobblerparser.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"

_LIT8(KGetTopAlbums, "gettopalbums");

CMobblerAlbumList::CMobblerAlbumList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerAlbumList::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultAlbumImage);
	
	iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
	
	switch (iType)
		{
		case EMobblerCommandUserTopAlbums:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetTopAlbums, iText1->String8(), *this);
			break;
		case EMobblerCommandArtistTopAlbums:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetTopAlbums, iText1->String8(), *this);
			break;
		case EMobblerCommandSearchAlbum:
			iAppUi.LastFmConnection().WebServicesCallL(KAlbum, KSearch, iText1->String8(), *this);
			break;
		default:
			break;
		}
	}

CMobblerAlbumList::~CMobblerAlbumList()
	{
    TRACER_AUTO;
	delete iWebServicesHelper;
	}

CMobblerListControl* CMobblerAlbumList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandOpen:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandPlaylistFetchAlbum, iList[iListBox->CurrentItemIndex()]->Title()->String8(), iList[iListBox->CurrentItemIndex()]->Id());
			break;
		case EMobblerCommandAlbumAddTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Description()->String8(), KNullDesC8, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse, EFalse));
			iWebServicesHelper->AddTagL(*track, aCommand);
			track->Release();
			}
			break;
		case EMobblerCommandAlbumRemoveTag:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Description()->String8(), KNullDesC8, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse, EFalse));
			iWebServicesHelper->AlbumRemoveTagL(*track);
			track->Release();
			}
			break;
		case EMobblerCommandAlbumShare:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Description()->String8(), KNullDesC8, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8, EFalse, EFalse));
			iWebServicesHelper->AlbumShareL(*track);
			track->Release();
			}
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerAlbumList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	aCommands.AppendL(EMobblerCommandOpen);
	
	aCommands.AppendL(EMobblerCommandTag);
	aCommands.AppendL(EMobblerCommandAlbumAddTag);
	aCommands.AppendL(EMobblerCommandAlbumRemoveTag);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandAlbumShare);
	}

void CMobblerAlbumList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, TInt /*aError*/)
	{
    TRACER_AUTO;
	}

TBool CMobblerAlbumList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	switch (iType)
		{
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandArtistTopAlbums:
			CMobblerParser::ParseTopAlbumsL(aXml, *this, iList);
			break;
		case EMobblerCommandSearchAlbum:
			CMobblerParser::ParseSearchAlbumL(aXml, *this, iList);
			break;
		default:
			break;
		}
	
	return ETrue;
	}

// End of file
