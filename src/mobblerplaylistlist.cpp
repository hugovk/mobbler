/*
mobblerplaylistlist.cpp

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

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerparser.h"
#include "mobblerplaylistlist.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_playlist.png");
_LIT8(KPlaylist, "playlist");

CMobblerPlaylistList::CMobblerPlaylistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerPlaylistList::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultPlaylistImage);
	
	iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetPlaylists, iText1->String8(), *this);
	}

CMobblerPlaylistList::~CMobblerPlaylistList()
	{
    TRACER_AUTO;
	delete iPlaylistCreateObserver;
	delete iPlaylistAddObserver;
	}

CMobblerListControl* CMobblerPlaylistList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandRadioStart:
			CMobblerString* playlistId(CMobblerString::NewLC(iList[iListBox->CurrentItemIndex()]->Id()));
			iAppUi.RadioStartL(EMobblerCommandRadioPlaylist, playlistId);
			CleanupStack::PopAndDestroy(playlistId);
			break;
		case EMobblerCommandOpen:
			{
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, 
					EMobblerCommandPlaylistFetchUser, 
					iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
					iList[iListBox->CurrentItemIndex()]->Id());
			}
			break;
		case EMobblerCommandPlaylistAddTrack:
			if (iAppUi.CurrentTrack())
				{
				delete iPlaylistAddObserver;
				iPlaylistAddObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
				iAppUi.LastFmConnection().PlaylistAddTrackL(iList[iListBox->CurrentItemIndex()]->Id(),
																iAppUi.CurrentTrack()->Artist().String8(),
																iAppUi.CurrentTrack()->Title().String8(),
																*iPlaylistAddObserver);
				}
			break;
		case EMobblerCommandPlaylistCreate:
			{
			// ask for title and description
			TBuf<KMobblerMaxQueryDialogLength> title;
			TBuf<KMobblerMaxQueryDialogLength> description;
			
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
					iPlaylistCreateObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
					
					iAppUi.LastFmConnection().PlaylistCreateL(title, description, *iPlaylistCreateObserver);
					
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

void CMobblerPlaylistList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError /*aError*/)
	{
    TRACER_AUTO;
	if (aObserver == iPlaylistCreateObserver)
		{
		// refresh the playlist list
		// Parse the XML
		CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
		CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
		
		CSenElement* playlist(domFragment->AsElement().Element(KPlaylists)->Element(KPlaylist));
		
		CMobblerListItem* item(CMobblerListItem::NewL(*this,
													playlist->Element(KTitle)->Content(),
													playlist->Element(KDescription)->Content(),
													playlist->Element(KImage)->Content()));
		
		item->SetIdL(playlist->Element(KId)->Content());
		
		iList.InsertL(item, 0);
		
		CleanupStack::PopAndDestroy(2);
		}
	else if (aObserver == iPlaylistAddObserver)
		{
		// handle adding a track to the playlist
		}
	}

void CMobblerPlaylistList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	if (iAppUi.CurrentTrack())
		{
		aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
		}
	
	aCommands.AppendL(EMobblerCommandRadioStart);
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandPlaylistCreate);
	}


void CMobblerPlaylistList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	CMobblerParser::ParsePlaylistsL(aXml, *this, iList);
	}

// End of file
