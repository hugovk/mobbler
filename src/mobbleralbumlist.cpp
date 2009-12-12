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
#include "mobblerutility.h"

_LIT8(KGetTopAlbums, "gettopalbums");

CMobblerAlbumList::CMobblerAlbumList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerAlbumList::ConstructL()
	{
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultAlbumImage);
	
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
	}

CMobblerListControl* CMobblerAlbumList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandOpen:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandPlaylistFetchAlbum, iList[iListBox->CurrentItemIndex()]->Title()->String8(), iList[iListBox->CurrentItemIndex()]->Id());
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerAlbumList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandOpen);
	}

void CMobblerAlbumList::ParseL(const TDesC8& aXml)
	{
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
	}

// End of file
