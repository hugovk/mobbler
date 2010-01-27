/*
mobblerwebservicesview.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008  Michael Coffey

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
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerlistcontrol.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerwebservicescontrol.h"
#include "mobblerwebservicesview.h"

CMobblerWebServicesView* CMobblerWebServicesView::NewL()
	{
	CMobblerWebServicesView* self(new(ELeave) CMobblerWebServicesView);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesView::CMobblerWebServicesView()
	{
	}

CMobblerWebServicesView::~CMobblerWebServicesView()
	{
	delete iWebServicesControl;
	}

void CMobblerWebServicesView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_WEBSERVICES_VIEW);
	}

void CMobblerWebServicesView::SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, TInt aCommandId)
	{
	HBufC* menuText(static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(aResourceId).AllocLC());

	const TInt KTextLimit(CEikMenuPaneItem::SData::ENominalTextLength);
	if (menuText->Length() > KTextLimit)
		{
		TBuf<KTextLimit> newText(menuText->Left(KTextLimit));
		CleanupStack::PopAndDestroy(menuText);
		menuText = newText.AllocLC();
		}

	aMenuPane->SetItemTextL(aCommandId, *menuText);
	CleanupStack::PopAndDestroy(menuText);
	}

void CMobblerWebServicesView::FilterMenuItemL(CEikMenuPane* aMenuPane, TInt aIndex, const RArray<TInt>& aSupportedCommands)
	{
	TInt position(0);
	if (aMenuPane->MenuItemExists(aIndex, position))
		{
		TBool commandNotSupported(aSupportedCommands.Find(aIndex) == KErrNotFound);
		
		aMenuPane->SetItemDimmed(aIndex, commandNotSupported || iWebServicesControl->TopControl()->Count() == 0);
		
		if (!commandNotSupported &&
				(aIndex == EMobblerCommandShout || aIndex == EMobblerCommandPlaylistCreate))
			{
			// Display these commands when they are supported even if there are no items
			aMenuPane->SetItemDimmed(aIndex, EFalse);
			}
		}
	}

void CMobblerWebServicesView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
	{
	// First load the menu text so as not to confuse any dimming logic
	if (aResourceId == R_MOBBLER_WEBSERVICES_MENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_OPEN,					EMobblerCommandOpen);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO,				EMobblerCommandRadioStart);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_START,			EMobblerCommandRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHARE,				EMobblerCommandShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ATTENDANCE,			EMobblerCommandAttendance);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VIEW,					EMobblerCommandView);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUT,				EMobblerCommandShout);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXIT,					EAknSoftkeyExit);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_LOVE_TRACK_OPTION,	EMobblerCommandTrackLove);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLIST_CREATE,		EMobblerCommandPlaylistCreate);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLIST_ADD_TRACK,	EMobblerCommandPlaylistAddTrack);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_MAP,					EMobblerCommandMaps);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SCROBBLE_LOG_REMOVE,	EMobblerCommandScrobbleLogRemove);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TAG,					EMobblerCommandTag);
		}
	else if (aResourceId == R_MOBBLER_WEBSERVICES_RADIO_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_PERSONAL,		EMobblerCommandRadioPersonal);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_NEIGHBOURHOOD,	EMobblerCommandRadioNeighbourhood);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_LOVED,			EMobblerCommandRadioLoved);
		}
	else if (aResourceId == R_MOBBLER_ATTENDANCE_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ATTENDANCE_YES,		EMobblerCommandAttendanceYes);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ATTENDANCE_MAYBE,		EMobblerCommandAttendanceMaybe);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ATTENDANCE_NO,		EMobblerCommandAttendanceNo);
		}
	else if (aResourceId == R_MOBBLER_SHARE_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK,			EMobblerCommandTrackShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST,			EMobblerCommandArtistShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENT,			EMobblerCommandEventShare);
		}
	else if (aResourceId == R_MOBBLER_VIEW_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_FRIENDS,				EMobblerCommandFriends);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ARTISTS,			EMobblerCommandUserTopArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,			EMobblerCommandUserTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,			EMobblerCommandUserTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLISTS,			EMobblerCommandPlaylists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,				EMobblerCommandUserEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,				EMobblerCommandArtistEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,				EMobblerCommandUserTopTags);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RECENT_TRACKS,		EMobblerCommandRecentTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,				EMobblerCommandUserShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,				EMobblerCommandArtistShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,				EMobblerCommandEventShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_ARTISTS,		EMobblerCommandSimilarArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_TRACKS,		EMobblerCommandSimilarTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,			EMobblerCommandArtistTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,			EMobblerCommandArtistTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,				EMobblerCommandArtistTopTags);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VISIT_LASTFM_MENU,	EMobblerCommandEventWebPage);
		}
	else if (aResourceId == R_MOBBLER_MAP_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VISIT_MAP,			EMobblerCommandVisitMap);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_FOURSQUARE,			EMobblerCommandFoursquare);
		}
	else if (aResourceId == R_MOBBLER_TAG_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK_ADD_TAG,			EMobblerCommandTrackAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK_REMOVE_TAG,			EMobblerCommandTrackRemoveTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ALBUM_ADD_TAG,			EMobblerCommandAlbumAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ALBUM_REMOVE_TAG,			EMobblerCommandAlbumRemoveTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST_ADD_TAG,			EMobblerCommandArtistAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST_REMOVE_TAG,		EMobblerCommandArtistRemoveTag);
		}
	
	// Now the menu text is set, dimming logic is next
	RArray<TInt> supportedCommands;
	CleanupClosePushL(supportedCommands);
	
	// Always support the exit command
	supportedCommands.Append(EAknSoftkeyExit);
	
	// Find the commands that this list box supports
	iWebServicesControl->TopControl()->SupportedCommandsL(supportedCommands);
	
	// filter the offline commands
	for (TInt i(EMobblerCommandOffline); i < EMobblerCommandOfflineLast; ++i)
		{
		FilterMenuItemL(aMenuPane, i, supportedCommands);
		}
	
	// filter the online commands
	for (TInt i(EMobblerCommandOnline); i < EMobblerCommandLast; ++i)
		{
		FilterMenuItemL(aMenuPane, i, supportedCommands);
		}
	
	CleanupStack::PopAndDestroy(&supportedCommands);
	
	if (aResourceId == R_MOBBLER_WEBSERVICES_MENU_PANE)
		{
		if (iWebServicesControl->TopControl()->Type() == EMobblerCommandFriends
				&& !static_cast<CMobblerAppUi*>(AppUi())->CurrentTrack())
			{
			// Do the share option for friends list even if there is no current track
			aMenuPane->SetItemDimmed(EMobblerCommandShare, ETrue);
			}
		}
	else if (aResourceId == R_MOBBLER_SHOUT_SUBMENU_PANE)
		{
		TBool shoutsExist(ETrue);
		if (iWebServicesControl->TopControl()->Count() == 0)
			{
			shoutsExist = EFalse;
			}
		
		// This must be a shoutbox
		CMobblerShoutbox* shoutbox(static_cast<CMobblerShoutbox*>(iWebServicesControl->TopControl()));
		
		HBufC* shoutTextUser(NULL);
		if (shoutsExist)
			{
			shoutTextUser = shoutbox->ShoutAtTextUserLC();
			}
		HBufC* shoutTextOwner(shoutbox->ShoutAtTextOwnerLC());

		aMenuPane->SetItemDimmed(EMobblerCommandShoutUser, !shoutsExist);
		aMenuPane->SetItemDimmed(EMobblerCommandShoutOwner, EFalse);
		aMenuPane->SetItemTextL(EMobblerCommandShoutOwner, *shoutTextOwner);

		if (shoutsExist)
			{
			if (shoutTextUser->CompareF(*shoutTextOwner) == 0)
				{
				aMenuPane->SetItemTextL(EMobblerCommandShoutOwner, *shoutTextUser);
				aMenuPane->SetItemDimmed(EMobblerCommandShoutUser, ETrue);
				}
			else
				{
				aMenuPane->SetItemTextL(EMobblerCommandShoutUser, *shoutTextUser);
				}
			}
		
		CleanupStack::PopAndDestroy(shoutTextOwner);
		if (shoutsExist)
			{
			CleanupStack::PopAndDestroy(shoutTextUser);
			}
		}
	}

TUid CMobblerWebServicesView::Id() const
	{
	return TUid::Uid(KMobblerWebServicesViewUid);
	}

void CMobblerWebServicesView::HandleCommandL(TInt aCommand)
	{
	iWebServicesControl->HandleListCommandL(aCommand);
	
	// let the app ui handle the event
	switch (aCommand)
		{
		case EAknSoftkeyBack:
			iWebServicesControl->BackL();
			break;
		case EAknSoftkeyExit:
			AppUi()->HandleCommandL(aCommand);
			break;
		case EMobblerCommandRadioStart:
		case EMobblerCommandRadioPersonal:
		case EMobblerCommandRadioNeighbourhood:
		case EMobblerCommandRadioLoved:
			// we have started a radio station so switch back to the status view
			AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid)); 
			break;
		default:
			// do nothing
			break;
		}
	}

void CMobblerWebServicesView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	if (!iWebServicesControl)
		{
		// create the first view
		iWebServicesControl = CMobblerWebServicesControl::NewL(*static_cast<CMobblerAppUi*>(AppUi()), AppUi()->ClientRect(), aCustomMessageId, aCustomMessage);
		iWebServicesControl->SetMopParent(AppUi());
		
		iWebServicesControl->ActivateL();
		// activate the top control in order
		AppUi()->AddToStackL(*this, iWebServicesControl);
		}
	}

void CMobblerWebServicesView::DoDeactivate()
	{
	if (iWebServicesControl)
		{
		// deactivate the top control
		AppUi()->RemoveFromStack(iWebServicesControl);
		delete iWebServicesControl;
		iWebServicesControl = NULL;
		}
	}

void CMobblerWebServicesView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	}

// End of file
