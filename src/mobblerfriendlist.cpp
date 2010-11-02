/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade

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

#include <aknquerydialog.h>
#include <aknmessagequerydialog.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerfriendlist.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerplaylistlist.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"

CMobblerFriendList::CMobblerFriendList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerFriendList::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultUserImage);
	
	iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, iText1->String8(), *this);
	}

CMobblerFriendList::~CMobblerFriendList()
	{
    TRACER_AUTO;
	delete iShareObserver;
	}

CMobblerListControl* CMobblerFriendList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandOpen:
			{
			// Show the details in a dialog box
			CAknMessageQueryDialog* dlg(new(ELeave) CAknMessageQueryDialog());
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(iList[iListBox->CurrentItemIndex()]->Title()->String());
			dlg->SetMessageTextL(iList[iListBox->CurrentItemIndex()]->Description()->String());
			dlg->RunLD();
			}
			break;
		case EMobblerCommandFriends:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, 
					EMobblerCommandFriends, 
					iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
					KNullDesC8);
			break;
		case EMobblerCommandTrackShare:
		case EMobblerCommandArtistShare:
			{
			TBuf<KMobblerMaxQueryDialogLength> message;
			
			CAknTextQueryDialog* shareDialog(new(ELeave) CAknTextQueryDialog(message));
			shareDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			shareDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_MESSAGE_PROMPT));
			shareDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (shareDialog->RunLD())
				{
				CMobblerString* messageString(CMobblerString::NewLC(message));
				
				if (iAppUi.CurrentTrack())
					{
					delete iShareObserver;
					iShareObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
					iAppUi.LastFmConnection().ShareL(aCommand, 
						iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
						iAppUi.CurrentTrack()->Artist().String8(), 
						iAppUi.CurrentTrack()->Album().String8(), 
						iAppUi.CurrentTrack()->Title().String8(), 
						KNullDesC8, 
						messageString->String8(), 
						*iShareObserver);
					}
				
				CleanupStack::PopAndDestroy(messageString);
				}
			}
			break;
		case EMobblerCommandRadioStart:
			iAppUi.RadioStartL(EMobblerCommandRadioUser, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandUserEvents:				// intentional fall-through
		case EMobblerCommandPlaylists:				// intentional fall-through
		case EMobblerCommandRecentTracks:			// intentional fall-through
		case EMobblerCommandUserShoutbox:			// intentional fall-through
		case EMobblerCommandUserTopArtists:			// intentional fall-through
		case EMobblerCommandUserTopTracks:			// intentional fall-through
		case EMobblerCommandUserTopAlbums:			// intentional fall-through
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, aCommand, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerFriendList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandFriends);
	aCommands.AppendL(EMobblerCommandUserEvents);
	aCommands.AppendL(EMobblerCommandPlaylists);
	aCommands.AppendL(EMobblerCommandRecentTracks);
	aCommands.AppendL(EMobblerCommandUserShoutbox);
	aCommands.AppendL(EMobblerCommandUserTopArtists);
	aCommands.AppendL(EMobblerCommandUserTopTracks);
	aCommands.AppendL(EMobblerCommandUserTopAlbums);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandTrackShare);
	aCommands.AppendL(EMobblerCommandAlbumShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandRadioStart);
	}

void CMobblerFriendList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, TInt /*aError*/)
	{
    TRACER_AUTO;
	}

TBool CMobblerFriendList::ParseL(const TDesC8& aXml)
	{
	TRACER_AUTO;

	TBool finished(ETrue);

	TInt total;
	TInt page;
	TInt perPage;
	TInt totalPages;
	CMobblerParser::ParseFriendListL(aXml, *this, iList, total, page, perPage, totalPages);
	
	if (page != totalPages)
		{
		iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, iText1->String8(), *this, page + 1, perPage);
		finished = EFalse;
		}
	
	return finished;
	}

// End of file
