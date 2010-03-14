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
					if (aCommand == EMobblerCommandTrackShare)
						{
						delete iShareObserver;
						iShareObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
						iAppUi.LastFmConnection().ShareL(aCommand, 
							iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
							iAppUi.CurrentTrack()->Artist().String8(), 
							iAppUi.CurrentTrack()->Title().String8(), 
							KNullDesC8, 
							messageString->String8(), 
							*iShareObserver);
						}
					else
						{
						delete iShareObserver;
						iShareObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
						iAppUi.LastFmConnection().ShareL(aCommand, 
							iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
							iAppUi.CurrentTrack()->Artist().String8(), 
							KNullDesC8, 
							KNullDesC8, 
							messageString->String8(), 
							*iShareObserver);
						}
					}
				
				CleanupStack::PopAndDestroy(messageString);
				}
			}
			break;
		case EMobblerCommandRadioPersonal:			// intentional fall-through
		case EMobblerCommandRadioLoved:				// intentional fall-through
		case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
			iAppUi.RadioStartL(aCommand, iList[iListBox->CurrentItemIndex()]->Title());
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
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandRadio);
	aCommands.AppendL(EMobblerCommandRadioPersonal);
	aCommands.AppendL(EMobblerCommandRadioNeighbourhood);
	aCommands.AppendL(EMobblerCommandRadioLoved);
	}

void CMobblerFriendList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, TInt /*aError*/)
	{
    TRACER_AUTO;
	
	}

void CMobblerFriendList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	CMobblerParser::ParseFriendListL(aXml, *this, iList);
	}

// End of file
