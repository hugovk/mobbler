/*
mobblerfriendlist.cpp

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
#include "mobblerfriendlist.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerplaylistlist.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_user.png");

CMobblerFriendList::CMobblerFriendList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerFriendList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getfriends"), iText1->String8(), *this);
	}

CMobblerFriendList::~CMobblerFriendList()
	{
	}

CMobblerListControl* CMobblerFriendList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandFriends:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandFriends, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandTrackShare:
		case EMobblerCommandArtistShare:
			{
	    	TBuf<255> message;
	    	
	    	CAknTextQueryDialog* shareDialog = new(ELeave) CAknTextQueryDialog(message);
	    	shareDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
	    	shareDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHARE));
	    	shareDialog->SetPredictiveTextInputPermitted(ETrue);

	    	if (shareDialog->RunLD())
	    		{
	    		CMobblerString* messageString = CMobblerString::NewL(message);
	    		CleanupStack::PushL(messageString);
	    		
	    		if (iAppUi.CurrentTrack())
					{
					if (aCommand == EMobblerCommandTrackShare)
						{
						iAppUi.LastFMConnection().TrackShareL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), iAppUi.CurrentTrack()->Artist().String8(), iAppUi.CurrentTrack()->Title().String8(), messageString->String8());
						}
					else
						{	
						iAppUi.LastFMConnection().ArtistShareL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), iAppUi.CurrentTrack()->Artist().String8(), messageString->String8());
						}
					}
	    		
	    		CleanupStack::PopAndDestroy(messageString);
	    		}
			}
			break;
		case EMobblerCommandRadioPersonal:
			iAppUi.RadioStartL(CMobblerLastFMConnection::EPersonal, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandRadioNeighbourhood:
			iAppUi.RadioStartL(CMobblerLastFMConnection::ENeighbourhood, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandRadioLoved:
			iAppUi.RadioStartL(CMobblerLastFMConnection::ELovedTracks, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandUserEvents:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandUserEvents, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandPlaylists:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandPlaylists, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandRecentTracks:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandRecentTracks, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandUserShoutbox:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandUserShoutbox, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerFriendList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandFriends);
	aCommands.AppendL(EMobblerCommandUserEvents);
	aCommands.AppendL(EMobblerCommandPlaylists);
	aCommands.AppendL(EMobblerCommandRecentTracks);
	aCommands.AppendL(EMobblerCommandUserShoutbox);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandTrackShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandRadio);
	aCommands.AppendL(EMobblerCommandRadioPersonal);
	aCommands.AppendL(EMobblerCommandRadioNeighbourhood);
	aCommands.AppendL(EMobblerCommandRadioLoved);
	}

void CMobblerFriendList::ParseL(const TDesC8& aXML)
	{
	CMobblerParser::ParseFriendListL(aXML, *this, iList);
	}

// End of file