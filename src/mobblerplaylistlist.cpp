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

#include <aknquerydialog.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerplaylistlist.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_playlist.png");

CMobblerPlaylistList::CMobblerPlaylistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerPlaylistList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getplaylists"), iText1->String8(), *this);
	}

CMobblerPlaylistList::~CMobblerPlaylistList()
	{
	delete iPlaylistCreateObserver;
	delete iPlaylistAddObserver;
	}

CMobblerListControl* CMobblerPlaylistList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandOpen:
			{
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandPlaylistFetch, iList[iListBox->CurrentItemIndex()]->Title()->String8(), iList[iListBox->CurrentItemIndex()]->Id());
			}
			break;
		case EMobblerCommandPlaylistAddTrack:
			if (iAppUi.CurrentTrack())
				{
				delete iPlaylistAddObserver;
				iPlaylistAddObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFMConnection(), *this);
				iAppUi.LastFMConnection().PlaylistAddTrackL(iList[iListBox->CurrentItemIndex()]->Id(),
																iAppUi.CurrentTrack()->Artist().String8(),
																iAppUi.CurrentTrack()->Title().String8(),
																*iPlaylistAddObserver);
				}
			break;
		case EMobblerCommandPlaylistCreate:
			{
			// ask for title and description
			TBuf<255> title;
			TBuf<255> description;
			
			CAknTextQueryDialog* titleDialog(new(ELeave) CAknTextQueryDialog(title));
			titleDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			titleDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_TITLE));
			titleDialog->SetPredictiveTextInputPermitted(ETrue);

			if (titleDialog->RunLD())
				{
				CAknTextQueryDialog* descriptionDialog(new(ELeave) CAknTextQueryDialog(description));
				descriptionDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
				descriptionDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_DESCRIPTION));
				descriptionDialog->SetPredictiveTextInputPermitted(ETrue);
				
				if (descriptionDialog->RunLD())
					{
					delete iPlaylistCreateObserver;
					iPlaylistCreateObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFMConnection(), *this);
					
					iAppUi.LastFMConnection().PlaylistCreateL(title, description, *iPlaylistCreateObserver);
					
					// Add this playlist to the list
					}
				}
			}
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerPlaylistList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFMConnection::TError aError)
	{
	if (aObserver == iPlaylistCreateObserver)
		{
		// refresh the playlist list
		iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getplaylists"), iText1->String8(), *this);
		}
	else if (aObserver == iPlaylistAddObserver)
		{
		// handle adding a track to the playlist
		}
	}

void CMobblerPlaylistList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	if (iAppUi.CurrentTrack())
		{
		aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
		}
	
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandPlaylistCreate);
	}


void CMobblerPlaylistList::ParseL(const TDesC8& aXML)
	{
	iList.ResetAndDestroy();
	iListBoxItems->Reset();
	CMobblerParser::ParsePlaylistsL(aXML, *this, iList);
	}

// End of file
