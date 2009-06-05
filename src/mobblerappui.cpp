/*
mobblerappui.cpp

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

#include <aknmessagequerydialog.h>
#include <akninfopopupnotecontroller.h>
#include <AknLists.h>
#include <aknnotewrappers.h>
#include <aknserverapp.h>	// MAknServerAppExitObserver
#include <aknsutils.h>
#include <bautils.h> 

#ifndef __WINS__
#include <browserlauncher.h>
#endif

#include <DocumentHandler.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>
#include <s32file.h>

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlogging.h"
#include "mobblermusiclistener.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstatusview.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"
#include "mobblerwebservicesview.h"

_LIT(KRadioFile, "C:radiostations.dat");

// Gesture interface
const TUid KGesturesInterfaceUid = {0xA000B6CF};

void CMobblerAppUi::ConstructL()
	{
	iResourceReader = CMobblerResourceReader::NewL();
	
	iBitmapCollection = CMobblerBitmapCollection::NewL();
	
	iVolumeUpCallBack = TCallBack(CMobblerAppUi::VolumeUpCallBackL, this);
	iVolumeDownCallBack = TCallBack(CMobblerAppUi::VolumeDownCallBackL, this);
	
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
	
#ifdef __S60_50__
	BaseConstructL(EAknTouchCompatible | EAknEnableSkin);
#else
	BaseConstructL(EAknEnableSkin);
#endif
	
	AknsUtils::InitSkinSupportL();
	
	// Create view object
	iSettingView = CMobblerSettingItemListView::NewL();
	iStatusView = CMobblerStatusView::NewL();
	
	iLastFMConnection = CMobblerLastFMConnection::NewL(*this, iSettingView->Username(), iSettingView->Password(), iSettingView->IapId());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFMConnection, iSettingView->BufferSize(), iSettingView->EqualizerIndex(), iSettingView->Volume());
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFMConnection);
	
	RProcess().SetPriority(EPriorityHigh);
	
#ifndef __WINS__
	iBrowserLauncher = CBrowserLauncher::NewL();
#endif
	LoadRadioStationsL();
	
	iMobblerDownload = CMobblerDownload::NewL(*this);

	iSleepTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
	iAlarmTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);

	iWebServicesView = CMobblerWebServicesView::NewL();
	
	iLastFMConnection->SetModeL(iSettingView->Mode());
	iLastFMConnection->LoadCurrentTrackL();

	if (iSettingView->AlarmOn())
		{
		// If the time has already passed, no problem, the timer will 
		// simply expire immediately with KErrUnderflow.
		iAlarmTimer->At(iSettingView->AlarmTime());
		}


	// Attempt to load gesture plug-in
	iGesturePlugin = NULL;
	TRAP_IGNORE(LoadGesturesPluginL());
	if (iGesturePlugin && iSettingView->AccelerometerGestures())
		{
		SetAccelerometerGesturesL(ETrue);
		}
	
	AddViewL(iWebServicesView);
    AddViewL(iSettingView);
    AddViewL(iStatusView);
    ActivateLocalViewL(iStatusView->Id());
	}

CMobblerAppUi::CMobblerAppUi()
	: iSleepAfterTrackStopped(EFalse)
	{
	}

CMobblerAppUi::~CMobblerAppUi()
	{
	if (iGesturePlugin)
		{
		REComSession::DestroyedImplementation(iGesturePluginDtorUid);
		delete iGesturePlugin;
		}

	delete iPreviousRadioArtist;
	delete iPreviousRadioTag;
	delete iPreviousRadioUser;
	delete iMusicListener;
	delete iRadioPlayer;
	delete iLastFMConnection;
	delete iMobblerDownload;
	delete iInterfaceSelector;
	delete iVolumeUpTimer;
	delete iVolumeDownTimer;
	
#ifndef __WINS__
	delete iBrowserLauncher;
#endif
	delete iResourceReader;
	delete iSleepTimer;
	delete iAlarmTimer;
	delete iBitmapCollection;
	
	delete iWebServicesHelper;
	
	delete iCheckForUpdatesObserver;
	}

TBool CMobblerAppUi::AccelerometerGesturesAvailable() const
	{
	return (iGesturePlugin != NULL);
	}

TInt CMobblerAppUi::VolumeUpCallBackL(TAny *aSelf)
	{
	CMobblerAppUi* self = static_cast<CMobblerAppUi*>(aSelf);
	
	self->iRadioPlayer->VolumeUp();
	
	if (self->iStatusView->StatusControl())
		{
		self->iStatusView->StatusControl()->VolumeChanged();
		}
	
	return KErrNone;
	}

TInt CMobblerAppUi::VolumeDownCallBackL(TAny *aSelf)
	{
	CMobblerAppUi* self = static_cast<CMobblerAppUi*>(aSelf);
	
	self->iRadioPlayer->VolumeDown();
	
	if (self->iStatusView->StatusControl())
		{
		self->iStatusView->StatusControl()->VolumeChanged();
		}
	
	return KErrNone;
	}

void CMobblerAppUi::MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct)
	{
	// don't bother if there's a current music player track
	if ((CurrentTrack() && 
		(CurrentTrack()->RadioAuth().Compare(KNullDesC8) == 0)))
		{
		return;
		}
		
	TRequestStatus status;
	TTimeIntervalMicroSeconds32 repeatDelay;
	TTimeIntervalMicroSeconds32 repeatInterval;
	
	switch (aOperationId)
		{
		case ERemConCoreApiStop:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				iRadioPlayer->Stop();
				}
			iCoreTarget->StopResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiForward:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				if (iRadioPlayer->CurrentTrack())
					{
					TRAP_IGNORE(iRadioPlayer->NextTrackL());
					}
				}
			iCoreTarget->ForwardResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiVolumeUp:
			{   
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					iRadioPlayer->VolumeUp();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					break;
				case ERemConCoreApiButtonPress:
					iRadioPlayer->VolumeUp();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					delete iVolumeUpTimer;
					iVolumeUpTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeUpTimer->Start(repeatDelay, repeatInterval, iVolumeUpCallBack);
					break;
				case ERemConCoreApiButtonRelease:
					delete iVolumeUpTimer;
					iVolumeUpTimer = NULL;
					break;
				default:
					break;
				}
			
			iCoreTarget->VolumeUpResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}   
		case ERemConCoreApiVolumeDown:
			{
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					iRadioPlayer->VolumeDown();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					break;
				case ERemConCoreApiButtonPress:
					iRadioPlayer->VolumeDown();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					delete iVolumeDownTimer;
					iVolumeDownTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeDownTimer->Start(TTimeIntervalMicroSeconds32(repeatDelay), TTimeIntervalMicroSeconds32(repeatInterval), iVolumeDownCallBack);
					break;
				case ERemConCoreApiButtonRelease:
					delete iVolumeDownTimer;
					iVolumeDownTimer = NULL;
					break;
				default:
					break;
				}
			
			iCoreTarget->VolumeDownResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}
		default:
			break;
		}
	}

void CMobblerAppUi::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	iLastFMConnection->SetDetailsL(aUsername, aPassword);
	}

void CMobblerAppUi::SetIapIDL(TUint32 aIapID)
	{
	iLastFMConnection->SetIapIDL(aIapID);
	}

void CMobblerAppUi::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	iRadioPlayer->SetPreBufferSize(aBufferSize);
	}

void CMobblerAppUi::SetAccelerometerGesturesL(TBool aAccelerometerGestures)
	{
	if (iGesturePlugin && aAccelerometerGestures)
		{
		iGesturePlugin->ObserveGesturesL(*this);
		}
	else if (iGesturePlugin)
		{
		iGesturePlugin->StopObserverL(*this);
		}
	}

const CMobblerTrack* CMobblerAppUi::CurrentTrack() const
	{
	const CMobblerTrack* track(iRadioPlayer->CurrentTrack());
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}

	return track;
	}

CMobblerTrack* CMobblerAppUi::CurrentTrack()
	{
	CMobblerTrack* track(iRadioPlayer->CurrentTrack());
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}

	return track;
	}

CMobblerLastFMConnection& CMobblerAppUi::LastFMConnection() const
	{
	return *iLastFMConnection;
	}

CMobblerRadioPlayer& CMobblerAppUi::RadioPlayer() const
	{
	return *iRadioPlayer;
	}

CMobblerMusicAppListener& CMobblerAppUi::MusicListener() const
	{
	return *iMusicListener;
	}

CMobblerSettingItemListView& CMobblerAppUi::SettingView() const
	{
	return *iSettingView;
	}

const TDesC& CMobblerAppUi::MusicAppNameL() const
	{
	return iMusicListener->MusicAppNameL();
	}

void CMobblerAppUi::HandleInstallStartedL()
	{
	RunAppShutter();
	}

void CMobblerAppUi::HandleCommandL(TInt aCommand)
	{
	TApaTask task(iEikonEnv->WsSession());
	const CMobblerTrack* const currentTrack(CurrentTrack());
	const CMobblerTrack* const currentRadioTrack(iRadioPlayer->CurrentTrack());
	
	TBuf<EMobblerMaxQueryDialogLength> tag;
	TBuf<EMobblerMaxQueryDialogLength> artist;
	TBuf<EMobblerMaxQueryDialogLength> user;

	// Don't bother going online to Last.fm if no user details entered
	if (aCommand >= EMobblerCommandOnline)
		{
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), KSettingsFile)
			== EFalse)
			{
			CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
			note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_NO_DETAILS));

			// bail from the function
			return;
			}
		}

	switch (aCommand)
		{
		case EAknSoftkeyExit:
			// Send application to the background to give the user
			// a sense of a really fast shutdown. Sometimes the thread
			// doesn't shut down instantly, so best to do this without
			// interferring with the user's ability to do something
			// else with their phone.
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			/// Check if scrobblable first and save queue
			iLastFMConnection->TrackStoppedL();
			iRadioPlayer->Stop();
			Exit();
			break;
		case EEikCmdExit:
		case EAknSoftkeyBack:
			task.SetWgId( CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			break;
		case EMobblerCommandOnline:
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
			iSettingView->SetModeL(CMobblerLastFMConnection::EOnline);
			break;
		case EMobblerCommandOffline:
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOffline);
			iSettingView->SetModeL(CMobblerLastFMConnection::EOffline);
			break;
		case EMobblerCommandFriends:			// intentional fall-through
		case EMobblerCommandUserTopArtists:		// intentional fall-through
		case EMobblerCommandRecommendedArtists:	// intentional fall-through
		case EMobblerCommandRecommendedEvents:	// intentional fall-through
		case EMobblerCommandUserTopAlbums:		// intentional fall-through
		case EMobblerCommandUserTopTracks:		// intentional fall-through
		case EMobblerCommandPlaylists:			// intentional fall-through
		case EMobblerCommandUserEvents:			// intentional fall-through
		case EMobblerCommandUserTopTags:		// intentional fall-through
		case EMobblerCommandRecentTracks:		// intentional fall-through
		case EMobblerCommandUserShoutbox:		// intentional fall-through
		
			if (iLastFMConnection->Mode() != CMobblerLastFMConnection::EOnline && GoOnlineL())
				{
				iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
				}
				
			if (iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
				{
				CMobblerString* username(CMobblerString::NewL(iSettingView->Username()));
				CleanupStack::PushL(username);
				ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), username->String8());
				CleanupStack::PopAndDestroy(username);
				}
				
			break;
		case EMobblerCommandSearchTrack:
		case EMobblerCommandSearchAlbum:
		case EMobblerCommandSearchArtist:
		case EMobblerCommandSearchTag:
			{
			TBuf<EMobblerMaxQueryDialogLength> search;
			CAknTextQueryDialog* userDialog(new(ELeave) CAknTextQueryDialog(search));
			userDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			userDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_SEARCH));
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				CMobblerString* searchString(CMobblerString::NewL(search));
				CleanupStack::PushL(searchString);
				ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), searchString->String8());
				CleanupStack::PopAndDestroy(searchString);
				}
			}
			break;
		case EMobblerCommandCheckForUpdates:
			{
			delete iCheckForUpdatesObserver;
			iCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFMConnection, *this, EFalse);
			iLastFMConnection->CheckForUpdateL(*iCheckForUpdatesObserver);
			}
			break;
		case EMobblerCommandEditSettings:
			ActivateLocalViewL(iSettingView->Id(), 
								TUid::Uid(CMobblerSettingItemListView::ENormalSettings), 
								KNullDesC8);
			break;
		case EMobblerCommandAbout:
			{
			// create the message text
			const TDesC& aboutText1(iResourceReader->ResourceL(R_MOBBLER_ABOUT_TEXT1));
			const TDesC& aboutText2(iResourceReader->ResourceL(R_MOBBLER_ABOUT_TEXT2));
			
			HBufC* msg(HBufC::NewLC(aboutText1.Length() + KVersion.Name().Length() + aboutText2.Length()));
			
			msg->Des().Append(aboutText1);
			msg->Des().Append(KVersion.Name());
			msg->Des().Append(aboutText2);
			
			// create the header text
			CAknMessageQueryDialog* dlg(new(ELeave) CAknMessageQueryDialog());
			
			// initialise the dialog
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(iResourceReader->ResourceL(R_ABOUT_DIALOG_TITLE));
			dlg->SetMessageTextL(*msg);
			
			dlg->RunLD();
			
			CleanupStack::PopAndDestroy(msg);
			}
			break;
		case EMobblerCommandResumeRadio:
			if (!RadioResumable())
				{
				break;
				}
			if (iRadioPlayer->HasPlaylist() && 
				iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
				{
				iRadioPlayer->NextTrackL();
				}
			else
				{
				switch (iPreviousRadioStation)
					{
					case EMobblerCommandRadioArtist:
						RadioStartL(iPreviousRadioStation, iPreviousRadioArtist, EFalse);
						break;
					case EMobblerCommandRadioTag:
						RadioStartL(iPreviousRadioStation, iPreviousRadioTag, EFalse);
						break;
					case EMobblerCommandRadioUser:
						RadioStartL(iPreviousRadioStation, iPreviousRadioUser, EFalse);
						break;
					case EMobblerCommandRadioRecommendations:	// intentional fall-through
					case EMobblerCommandRadioPersonal:			// intentional fall-through
					case EMobblerCommandRadioLoved:				// intentional fall-through
					case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
					default:
						RadioStartL(iPreviousRadioStation, NULL, EFalse);
						break;
					}
				}
			break;
		case EMobblerCommandRadioArtist:
			{
			if (!RadioStartableL())
				{
				break;
				}

			// ask the user for the artist name	
			if (iPreviousRadioArtist)
				{
				artist = iPreviousRadioArtist->String();
				}
			CAknTextQueryDialog* artistDialog(new(ELeave) CAknTextQueryDialog(artist));
			artistDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			artistDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_ARTIST));
			artistDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (artistDialog->RunLD())
				{
				CMobblerString* artistString(CMobblerString::NewL(artist));
				CleanupStack::PushL(artistString);
				RadioStartL(EMobblerCommandRadioArtist, artistString);
				CleanupStack::PopAndDestroy(artistString);
				}
			}
			break;
		case EMobblerCommandRadioTag:
			{
			if (!RadioStartableL())
				{
				break;
				}
			
			// ask the user for the tag
			if (iPreviousRadioTag)
				{
				tag = iPreviousRadioTag->String();
				}
			
			CAknTextQueryDialog* tagDialog(new(ELeave) CAknTextQueryDialog(tag));
			tagDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			tagDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_TAG));
			tagDialog->SetPredictiveTextInputPermitted(ETrue);

			if (tagDialog->RunLD())
				{
				CMobblerString* tagString(CMobblerString::NewL(tag));
				CleanupStack::PushL(tagString);
				RadioStartL(EMobblerCommandRadioTag, tagString);
				CleanupStack::PopAndDestroy(tagString);
				}
			}
			break;
		case EMobblerCommandRadioUser:
			{
			if (!RadioStartableL())
				{
				break;
				}
			
			// ask the user for the user
			if (iPreviousRadioUser)
				{
				user = iPreviousRadioUser->String();
				}
			
			CAknTextQueryDialog* userDialog(new(ELeave) CAknTextQueryDialog(user));
			userDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			userDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_USER));
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				CMobblerString* userString(CMobblerString::NewL(user));
				CleanupStack::PushL(userString);
				RadioStartL(EMobblerCommandRadioUser, userString);
				CleanupStack::PopAndDestroy(userString);
				}
			}
			break;
		case EMobblerCommandRadioRecommendations:	// intentional fall-through
		case EMobblerCommandRadioPersonal:			// intentional fall-through
		case EMobblerCommandRadioLoved:				// intentional fall-through
		case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
			RadioStartL(aCommand, NULL);
			break;
		case EMobblerCommandTrackLove:
			// you can love either radio or music player tracks
			
			if (currentTrack)
				{
				if (!currentTrack->Love())
					{
					// There is a current track and it is not already loved
					CAknQueryDialog* dlg(CAknQueryDialog::NewL());
					TBool love(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_LOVE_TRACK)));
					
					if (love)
						{
						// set love to true (if only it were this easy)
						CurrentTrack()->SetLove(ETrue);
						iLastFMConnection->TrackLoveL(currentTrack->Artist().String8(), currentTrack->Title().String8());
						}
					}
				}
			
			break;
		case EMobblerCommandTrackBan:
			// you should only be able to ban radio tracks
			if (currentRadioTrack)
				{
				// There is a current track and it is not already loved
				CAknQueryDialog* dlg(CAknQueryDialog::NewL());
				TBool ban(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_BAN_TRACK)));
				
				if (ban)
					{
					// send the web services API call
					iLastFMConnection->TrackBanL(currentRadioTrack->Artist().String8(), currentRadioTrack->Title().String8());
					iRadioPlayer->NextTrackL();
					}
				}
			
			break;
		case EMobblerCommandPlus:
			
			if (currentTrack)
				{
				CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
			    CleanupStack::PushL(list);
			     
			    CAknPopupList* popup = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);
			    CleanupStack::PushL(popup);
			    
			    list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);

			    popup->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_CURRENT_TRACK));
			    
			    list->CreateScrollBarFrameL(ETrue);
			    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
			    
			    CDesCArrayFlat* items = new(ELeave) CDesCArrayFlat(10);
			    CleanupStack::PushL(items);
			    
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SHARE_TRACK));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SHARE_ARTIST));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_PLAYLIST_ADD_TRACK));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SIMILAR_ARTISTS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SIMILAR_TRACKS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_EVENTS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_ARTIST_SHOUTBOX));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_ALBUMS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_TRACKS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_TAGS));


			    CleanupStack::Pop(items);
			    
			    list->Model()->SetItemTextArray(items);
			    list->Model()->SetOwnershipType(ELbmOwnsItemArray);
			    
			    CleanupStack::Pop(popup);
			    
			    if (popup->ExecuteLD())
			    	{
			    	if (iLastFMConnection->Mode() != CMobblerLastFMConnection::EOnline && GoOnlineL())
			    		{
			    		iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
			    		}
			    	
			    	if (iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
			    		{
				    	switch (list->CurrentItemIndex())
				    		{
				    		case 0:
				    		case 1:
				    		case 2:
				    			{
				    			if (CurrentTrack())
				    				{
				    				delete iWebServicesHelper;
				    				iWebServicesHelper = CMobblerWebServicesHelper::NewL(*this);
				    				switch (list->CurrentItemIndex())
				    					{
				    					case 0: iWebServicesHelper->TrackShareL(*CurrentTrack()); break;
				    					case 1: iWebServicesHelper->ArtistShareL(*CurrentTrack()); break;
				    					case 2: iWebServicesHelper->PlaylistAddL(*CurrentTrack()); break;
				    					}
				    				}
				    			else
				    				{
				    				// TODO: display an error
				    				}
				    			}
					    		break;
				    		case 3:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarArtists), currentTrack->Artist().String8());
				    			break;
				    		case 4:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarTracks), currentTrack->MbTrackId().String8());
				    			break;
				    		case 5:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistEvents), currentTrack->Artist().String8());
				    			break;
				    		case 6:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistShoutbox), currentTrack->Artist().String8());
				    			break;
				    		case 7:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopAlbums), currentTrack->Artist().String8());
				    			break;
				    		case 8:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTracks), currentTrack->Artist().String8());
				    			break;
				    		case 9:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTags), currentTrack->Artist().String8()); 	
				    			break;
				    		default:
				    			break;
				    		}
			    		}
			    	}
			     
			    CleanupStack::PopAndDestroy(list);
				}
			
			break;
		case EMobblerCommandVisitWebPage:
			{
			if (CurrentTrack())
				{
				CEikTextListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
				CleanupStack::PushL(list);
				CAknPopupList* popupList(CAknPopupList::NewL(list, 
										 R_AVKON_SOFTKEYS_SELECT_CANCEL,
										 AknPopupLayouts::EMenuWindow));
				CleanupStack::PushL(popupList);

				list->ConstructL(popupList, CEikListBox::ELeftDownInViewRect);
				list->CreateScrollBarFrameL(ETrue);
				list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
																CEikScrollBarFrame::EAuto);

				CDesCArrayFlat* items(new CDesCArrayFlat(4));
				CleanupStack::PushL(items);

				HBufC* action(CurrentTrack()->Title().String().AllocLC());
				items->AppendL(*action);
				CleanupStack::PopAndDestroy(action);

				action = CurrentTrack()->Artist().String().AllocLC();
				items->AppendL(*action);
				CleanupStack::PopAndDestroy(action);

				if (CurrentTrack()->Album().String().Length() > 0)
					{
					action = CurrentTrack()->Album().String().AllocLC();
					items->AppendL(*action);
					CleanupStack::PopAndDestroy(action);
					}

				items->AppendL(iResourceReader->ResourceL(R_MOBBLER_EVENTS));

				
				CTextListBoxModel* model(list->Model());
				model->SetItemTextArray(items);
				model->SetOwnershipType(ELbmOwnsItemArray);
				CleanupStack::Pop();

				popupList->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_VISIT_LASTFM));
		
				list->SetCurrentItemIndex(1);
				TInt popupOk(popupList->ExecuteLD());
				CleanupStack::Pop();
				
				if (popupOk)
					{
					switch (list->CurrentItemIndex())
						{
						case 0:
							GoToLastFmL(EMobblerCommandTrackWebPage);
							break;
						case 1:
							GoToLastFmL(EMobblerCommandArtistWebPage);
							break;
						case 2:
							GoToLastFmL(EMobblerCommandAlbumWebPage);
							break;
						case 3:
							GoToLastFmL(EMobblerCommandEventsWebPage);
							break;
						default:
							break;
						}
					}
			    CleanupStack::PopAndDestroy(list);
				}
			}
			break;
		case EMobblerCommandToggleScrobbling:
			iLastFMConnection->ToggleScrobblingL();
			iStatusView->DrawDeferred();
			break;
		case EMobblerCommandSleepTimer:
			ActivateLocalViewL(iSettingView->Id(),
								TUid::Uid(CMobblerSettingItemListView::ESleepTimer),
								KNullDesC8);
			break;
		case EMobblerCommandAlarm:
			CAknQueryDialog* disclaimerDlg(CAknQueryDialog::NewL());
			if(disclaimerDlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, 
					iResourceReader->ResourceL(R_MOBBLER_ALARM_DISCLAIMER)))
				{
				ActivateLocalViewL(iSettingView->Id(),
									TUid::Uid(CMobblerSettingItemListView::EAlarm),
									KNullDesC8);
				}
			break;
		case EMobblerCommandExportQueueToLogFile:
			{
			if (iTracksQueued == 0)
				{
				CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
				note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_EXPORT_EMPTY_QUEUE));
				}
			else
				{
				TBool okToReplaceLog(ETrue);
				
				if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), KLogFile))
					{
					CAknQueryDialog* dlg(CAknQueryDialog::NewL());
					okToReplaceLog = dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_CONFIRM_REPLACE_LOG));
					}
				
				if (okToReplaceLog)
					{
					TInt resourceId(R_MOBBLER_NOTE_QUEUE_EXPORTED);
					if (!iLastFMConnection->ExportQueueToLogFileL())
						{
						BaflUtils::DeleteFile(CCoeEnv::Static()->FsSession(), KLogFile);
						resourceId = R_MOBBLER_NOTE_QUEUE_NOT_EXPORTED;
						}
					CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(iResourceReader->ResourceL(resourceId));
					}
				}
			}
			break;
		default:
			if (aCommand >= EMobblerCommandEqualizerDefault && 
				aCommand <= EMobblerCommandEqualizerMaximum)
				{
				TInt index(aCommand - EMobblerCommandEqualizerDefault - 1);
				RadioPlayer().SetEqualizer(index);
				iSettingView->SetEqualizerIndexL(index);
				return;
				}
			break;
		}
	}

void CMobblerAppUi::RadioStartL(TInt aRadioStation, 
								const CMobblerString* aRadioOption, 
								TBool aSaveStations)
	{
	iPreviousRadioStation = aRadioStation;

	if (aSaveStations)
		{
		switch (iPreviousRadioStation)
			{
			case EMobblerCommandRadioArtist:
				delete iPreviousRadioArtist;
				iPreviousRadioArtist = CMobblerString::NewL(aRadioOption->String());
				break;
			case EMobblerCommandRadioTag:
				delete iPreviousRadioTag;
				iPreviousRadioTag = CMobblerString::NewL(aRadioOption->String());
				break;
			case EMobblerCommandRadioUser:
				delete iPreviousRadioUser;
				iPreviousRadioUser = CMobblerString::NewL(aRadioOption->String());
				break;
			default:
				break;
			}

		SaveRadioStationsL();
		}
	
	if (!RadioStartableL())
		{
		return;
		}
	
	CMobblerLastFMConnection::TRadioStation station(CMobblerLastFMConnection::EPersonal);
	switch (aRadioStation)
		{
		case EMobblerCommandRadioArtist:
			station = CMobblerLastFMConnection::EArtist;
			break;
		case EMobblerCommandRadioTag:
			station = CMobblerLastFMConnection::ETag;
			break;
		case EMobblerCommandRadioUser:
			station = CMobblerLastFMConnection::EPersonal;
			break;
		case EMobblerCommandRadioRecommendations:
			station = CMobblerLastFMConnection::ERecommendations;
			break;
		case EMobblerCommandRadioPersonal:
			station = CMobblerLastFMConnection::EPersonal;
			break;
		case EMobblerCommandRadioLoved:
			station = CMobblerLastFMConnection::ELovedTracks;
			break;
		case EMobblerCommandRadioNeighbourhood:
			station = CMobblerLastFMConnection::ENeighbourhood;
			break;
		case EMobblerCommandRadioPlaylist:
			station = CMobblerLastFMConnection::EPlaylist;
			break;
		default:
			station = CMobblerLastFMConnection::EPersonal;
			break;
		}
	
	iRadioPlayer->StartL(station, aRadioOption);
	}

TBool CMobblerAppUi::RadioStartableL() const
	{
	// Can start only if the music player isn't already playing.
	if (iMusicListener->IsPlaying())
		{
		// Tell the user that there was an error connecting
		CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_STOP_MUSIC_PLAYER));

		return EFalse;
		}
	else
		{
		return ETrue;
		}
	}

TBool CMobblerAppUi::RadioResumable() const
	{
	// Can resume only if the radio is not playing now,
	// and if the music player isn't currently playing (paused is ok),
	// and if a previous radio station is known.
	if (!iRadioPlayer->CurrentTrack() &&
		!iMusicListener->IsPlaying() &&
		iPreviousRadioStation != EMobblerCommandRadioUnknown)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

CMobblerLastFMConnection::TMode CMobblerAppUi::Mode() const
	{
	return iLastFMConnection->Mode();
	}

CMobblerLastFMConnection::TState CMobblerAppUi::State() const
	{
	return iLastFMConnection->State();
	}

TBool CMobblerAppUi::ScrobblingOn() const
	{
	return iLastFMConnection->ScrobblingOn();
	}

void CMobblerAppUi::HandleStatusPaneSizeChange()
	{
	}

void CMobblerAppUi::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFMConnection::TError aError)
	{
	if (aObserver == iCheckForUpdatesObserver)
		{
		if (aError == CMobblerLastFMConnection::EErrorNone)
			{
			// we have just sucessfully checked for updates
			// so don't do it again for another week
			TTime now;
			now.UniversalTime();
			now += TTimeIntervalDays(KUpdateIntervalDays);
			iSettingView->SetNextUpdateCheckL(now);
			
			TVersion version;
			TBuf8<255> location;
			TInt error(CMobblerParser::ParseUpdateResponseL(aData, version, location));
			
			if (error == KErrNone)
				{
				if ((version.iMajor > KVersion.iMajor)
					|| 
					(version.iMajor == KVersion.iMajor && 
					 version.iMinor > KVersion.iMinor)
					|| 
					(version.iMajor == KVersion.iMajor && 
					 version.iMinor == KVersion.iMinor && 
					 version.iBuild > KVersion.iBuild))
					{
					CAknQueryDialog* dlg(CAknQueryDialog::NewL());
					TBool yes( dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_UPDATE)));
									
					if (yes)
						{
						iMobblerDownload->DownloadL(location, iLastFMConnection->IapID());
						}
					}
				else
					{
					CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NO_UPDATE));
					}
				}
			}
		}
	}

void CMobblerAppUi::HandleConnectCompleteL(TInt aError)
	{
//	iStatusView->DrawDeferred();
	
	if (aError != KErrNone)
		{
		// Tell the user that there was an error connecting
		CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_COMMS_ERROR));
		}
	else
		{
		// check for updates?
		if (iSettingView->CheckForUpdates())
			{
			TTime now;
			now.UniversalTime();
		
			if (now > iSettingView->NextUpdateCheck())
				{
				// do an update check
				delete iCheckForUpdatesObserver;
				iCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFMConnection, *this, EFalse);
				iLastFMConnection->CheckForUpdateL(*iCheckForUpdatesObserver);
				}
			}
		}
	}
	
void CMobblerAppUi::HandleLastFMErrorL(CMobblerLastFMError& aError)
	{
	// iStatusView->DrawDeferred();
	
	CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
	note->ExecuteLD(aError.Text());
	}

void CMobblerAppUi::HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus)
	{
	// iStatusView->DrawDeferred();
	
	HBufC* noteText(HBufC::NewLC(255));

	noteText->Des().Append(iResourceReader->ResourceL(R_MOBBLER_NOTE_COMMS_ERROR));
	noteText->Des().Append(_L(" "));
	noteText->Des().AppendNum(aStatusCode);
	noteText->Des().Append(_L(" "));
	
	HBufC* status(HBufC::NewLC(aStatus.Length()));
	status->Des().Copy(aStatus);
	noteText->Des().Append(*status);
	CleanupStack::PopAndDestroy(status);
	
	CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
	note->ExecuteLD(*noteText);
	
	CleanupStack::PopAndDestroy(noteText);
	}

TInt CMobblerAppUi::Scrobbled() const
	{
	return iTracksSubmitted;
	}

TInt CMobblerAppUi::Queued() const
	{
	return iTracksQueued;
	}

void CMobblerAppUi::HandleTrackNowPlayingL(const CMobblerTrack& /*aTrack*/)
	{
	// Tell the status view that the track has changed
//	iStatusView->DrawDeferred();
	}

void CMobblerAppUi::HandleTrackSubmittedL(const CMobblerTrack& /*aTrack*/)
	{
//	iStatusView->DrawDeferred();
	++iTracksSubmitted;
	--iTracksQueued;
	}

void CMobblerAppUi::HandleTrackQueuedL(const CMobblerTrack& /*aTrack*/)
	{		
	if (iStatusView)
		{
//		iStatusView->DrawDeferred();
		}
	// update the track queued count and change the status bar text
	++iTracksQueued;
	}

void CMobblerAppUi::HandleTrackDequeued(const CMobblerTrack& /*aTrack*/)
	{
//	iStatusView->DrawDeferred();
	--iTracksQueued;
	}

TBool CMobblerAppUi::GoOnlineL()
	{
	// Ask if they would like to go online
	CAknQueryDialog* dlg(CAknQueryDialog::NewL());
	TBool goOnline(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_ASK_GO_ONLINE)));
	
	if (goOnline)
		{
		iSettingView->SetModeL(CMobblerLastFMConnection::EOnline);
		}
	
	return goOnline;
	}

void CMobblerAppUi::StatusDrawDeferred()
	{
	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}
	}

void CMobblerAppUi::HandleForegroundEventL(TBool aForeground)
	{
	CAknAppUi::HandleForegroundEventL(aForeground);
	iForeground = aForeground;
	}

TBool CMobblerAppUi::Foreground() const
	{
	return iForeground;
	}

TBool CMobblerAppUi::Backlight() const
	{
	return iSettingView->Backlight();
	}

TInt CMobblerAppUi::ScrobblePercent() const
	{
	return iSettingView->ScrobblePercent();
	}

TInt CMobblerAppUi::DownloadAlbumArt() const
	{
	return iSettingView->DownloadAlbumArt();
	}

void CMobblerAppUi::TrackStoppedL()
	{
	iSettingView->SetVolumeL(RadioPlayer().Volume());
	
	if (iSleepAfterTrackStopped)
		{
		iSleepAfterTrackStopped = EFalse;
		SleepL();
		}
	}

void CMobblerAppUi::LoadRadioStationsL()
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KRadioFile, EFileRead));

	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);

		iPreviousRadioStation = (CMobblerLastFMConnection::TRadioStation)readStream.ReadInt32L();
		if (iPreviousRadioStation <= EMobblerCommandRadioEnumFirst ||
			iPreviousRadioStation >= EMobblerCommandRadioEnumLast)
			{
			iPreviousRadioStation = EMobblerCommandRadioUnknown;
			}
		
		TBuf<255> radio;
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioArtist;
			iPreviousRadioArtist = CMobblerString::NewL(radio);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioTag;
			iPreviousRadioTag = CMobblerString::NewL(radio);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioUser;
			iPreviousRadioUser = CMobblerString::NewL(radio);
			}

		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		iPreviousRadioStation = EMobblerCommandRadioUnknown;
		}

	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerAppUi::SaveRadioStationsL()
	{
	CCoeEnv::Static()->FsSession().MkDirAll(KRadioFile);

	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KRadioFile, EFileWrite));

	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);

		writeStream.WriteInt32L(iPreviousRadioStation);

		if (iPreviousRadioArtist)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioArtist->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioTag)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioTag->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioUser)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioUser->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		CleanupStack::PopAndDestroy(&writeStream);
		}

	CleanupStack::PopAndDestroy(&file);
	}

CMobblerResourceReader& CMobblerAppUi::ResourceReader() const
	{
	return *iResourceReader;
	}

CMobblerBitmapCollection& CMobblerAppUi::BitmapCollection() const
	{
	return *iBitmapCollection;
	}

void CMobblerAppUi::SetSleepTimerL(const TInt aMinutes)
	{
	LOG(_L8("CMobblerAppUi::SetSleepTimerL"));
	LOG(aMinutes);
	
	TInt sleepMinutes(aMinutes);
	if (iSleepTimer->IsActive())
		{
		TTime now;
		now.UniversalTime();
#ifdef __WINS__
		TTimeIntervalSeconds minutes(0);
		iTimeToSleep.SecondsFrom(now, minutes);
#else
		TTimeIntervalMinutes minutes(0);
		iTimeToSleep.MinutesFrom(now, minutes);
#endif
		sleepMinutes = minutes.Int();
		}

	iSettingView->SetSleepTimerMinutesL(sleepMinutes);
#ifdef __WINS__
	TTimeIntervalSeconds delay(sleepMinutes);
#else
	TTimeIntervalMinutes delay(sleepMinutes);
#endif
	iTimeToSleep.UniversalTime();
	iTimeToSleep += delay;
	iSleepTimer->AtUTC(iTimeToSleep);

#ifdef _DEBUG
	CEikonEnv::Static()->InfoMsg(_L("Timer set"));
#endif
	}

void CMobblerAppUi::SetAlarmTimerL(const TTime aTime)
	{
	TDateTime alarmDateTime(aTime.DateTime());
	TTime now;
	now.HomeTime();

	// Set the date to today, keep the time
	alarmDateTime.SetYear(now.DateTime().Year());
	alarmDateTime.SetMonth(now.DateTime().Month());
	alarmDateTime.SetDay(now.DateTime().Day());

	// TTime from TDateTime
	TTime alarmTime(alarmDateTime);

	// If the time was earlier today, it must be for tomorrow
	if (alarmTime < now)
		{
		alarmTime += (TTimeIntervalDays)1;
		}

	iSettingView->SetAlarmL(alarmTime);
	iAlarmTimer->At(alarmTime);
	CEikonEnv::Static()->InfoMsg(_L("Alarm set"));

	TTimeIntervalMinutes minutesInterval;
	alarmTime.MinutesFrom(now, minutesInterval);
	TInt hours(minutesInterval.Int() / 60);
	TInt minutes(minutesInterval.Int() - (hours * 60));
	CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));

	TInt resourceId(R_MOBBLER_ALARM_HOURS_MINUTES);
	if (hours == 1 && minutes == 1)
		{
		resourceId = R_MOBBLER_ALARM_HOUR_MINUTE;
		}
	else if (hours == 1 && minutes != 1)
		{
		resourceId = R_MOBBLER_ALARM_HOUR_MINUTES;
		}
	else if (hours != 1 && minutes == 1)
		{
		resourceId = R_MOBBLER_ALARM_HOURS_MINUTE;
		}

	TBuf<256> confirmationText;
	confirmationText.Format(iResourceReader->ResourceL(resourceId), hours, minutes);
	note->ExecuteLD(confirmationText);
	}

void CMobblerAppUi::TimerExpiredL(TAny* aTimer, TInt aError)
	{
#ifdef _DEBUG
	CEikonEnv::Static()->InfoMsg(_L("Timer expired!"));
	LOG(_L8("CMobblerAppUi::TimerExpiredL"));
	LOG(aError);
#endif
	if (aTimer == iSleepTimer && aError == KErrNone)
		{
		LOG(iSettingView->SleepTimerImmediacy());
		
		if (CurrentTrack() && 
			iSettingView->SleepTimerImmediacy() == CMobblerSettingItemListSettings::EEndOfTrack)
			{
			iSleepAfterTrackStopped = ETrue;
			}
		else // no track, or sleep immediately
			{
			SleepL();
			}
		}

	// When the system time changes, At() timers will complete immediately with
	// KErrAbort. This can happen either if the user changes the time, or if 
	// the phone is set to auto-update with the network operator time.
	else if (aTimer == iSleepTimer && aError == KErrAbort)
		{
		// Reset the timer
		iSleepTimer->AtUTC(iTimeToSleep);
		}	
	else if (aTimer == iAlarmTimer && aError == KErrNone)
		{
		iSettingView->SetAlarmL(EFalse);
		User::ResetInactivityTime();

		if (iLastFMConnection->IapID() != iSettingView->AlarmIapId())
			{
			HandleCommandL(EMobblerCommandOffline);
			SetIapIDL(iSettingView->AlarmIapId());
			}
		
		HandleCommandL(EMobblerCommandOnline);

		TApaTask task(iEikonEnv->WsSession());
		task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
		task.BringToForeground();

		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_ALARM_EXPIRED));

		//User::After(5000000);
		HandleCommandL(EMobblerCommandResumeRadio);
		}
	else if (aTimer == iAlarmTimer && aError == KErrAbort)
		{
		iAlarmTimer->At(iSettingView->AlarmTime());
		}
	else if (aTimer == iAlarmTimer && aError == KErrUnderflow)
		{
		iSettingView->SetAlarmL(EFalse);
		}
	}

void CMobblerAppUi::SleepL()
	{
	LOG(_L8("CMobblerAppUi::SleepL()"));
	// Do this for all actions, it gives Mobbler a chance to scrobble
	// the newly stopped song to Last.fm whilst displaying the dialog
	iLastFMConnection->TrackStoppedL();
	iRadioPlayer->Stop();

#ifdef _DEBUG
	CEikonEnv::Static()->InfoMsg(_L("Sleep!"));
#endif
	CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
	note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_EXPIRED));

	LOG(iSettingView->SleepTimerAction());
	switch (iSettingView->SleepTimerAction())
		{
		case CMobblerSettingItemListSettings::EStopPlaying:
			break;
		case CMobblerSettingItemListSettings::EGoOffline:
			HandleCommandL(EMobblerCommandOffline);
			break;
		case CMobblerSettingItemListSettings::ExitMobber:
			HandleCommandL(EAknSoftkeyExit);
			break;
		default:
			break;
		}
	}

void CMobblerAppUi::RemoveSleepTimerL()
	{
	if (iSleepTimer->IsActive())
		{
		iSleepTimer->Cancel();
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_REMOVED));
		}
	}

void CMobblerAppUi::RemoveAlarmL()
	{
	if (iAlarmTimer->IsActive())
		{
		iAlarmTimer->Cancel();
		iSettingView->SetAlarmL(EFalse);
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_ALARM_REMOVED));
		}
	}

// Gesture plug-in functions
void CMobblerAppUi::LoadGesturesPluginL()
	{
	// Finding implementations of the gesture plug-in interface.
	// Preferably, we should load the 5th edition plug-in, as it provides
	// extra functionality.
	// Failing this, load the 3rd edition plugin.
	// Otherwise, accelerometer support is not available.
	
	const TUid KMobblerGesturePlugin5xUid = {0xA000B6C2};
	
	RImplInfoPtrArray implInfoPtrArray;
	CleanupClosePushL(implInfoPtrArray);
	
	REComSession::ListImplementationsL(KGesturesInterfaceUid, implInfoPtrArray);
	
	const TInt KImplCount(implInfoPtrArray.Count());
	if (KImplCount < 1)
		{		
		// Plug-in not found.
		User::Leave(KErrNotFound);
		}
	
	TUid dtorIdKey;
	CMobblerGesturesInterface* mobblerGestures(NULL);

	// Search for the preferred plug-in implementation
	TBool fifthEditionPluginLoaded(EFalse);
	for (TInt i(0); i < KImplCount; ++i)
		{
		TUid currentImplUid(implInfoPtrArray[i]->ImplementationUid());	
		if (currentImplUid == KMobblerGesturePlugin5xUid)
			{
			// Found it, attempt to load it
			TRAPD(error, mobblerGestures = static_cast<CMobblerGesturesInterface*>(REComSession::CreateImplementationL(currentImplUid, dtorIdKey)));
			if (error == KErrNone)
				{
				fifthEditionPluginLoaded = ETrue;
				iGesturePlugin = mobblerGestures;
				iGesturePluginDtorUid = dtorIdKey;
				}
			else
				{
				REComSession::DestroyedImplementation(dtorIdKey);
				}
			}
		}
	
	// If we didn't load the preferred plug-in, try all other plug-ins
	if (! fifthEditionPluginLoaded)
		{
		for (TInt i(0); i < KImplCount; ++i)
			{
			TUid currentImplUid(implInfoPtrArray[i]->ImplementationUid());
			if (currentImplUid != KMobblerGesturePlugin5xUid)
				{
				TRAPD(error, mobblerGestures = static_cast<CMobblerGesturesInterface*>(REComSession::CreateImplementationL(currentImplUid, dtorIdKey)));
				if (error == KErrNone)
					{
					iGesturePlugin = mobblerGestures;
					iGesturePluginDtorUid = dtorIdKey;
					}
				else
					{
					REComSession::DestroyedImplementation(dtorIdKey);
					}
				}
			}
		}
	
	implInfoPtrArray.ResetAndDestroy();
	CleanupStack::PopAndDestroy(&implInfoPtrArray);
	}

void CMobblerAppUi::HandleSingleShakeL(TMobblerShakeGestureDirection aDirection)
	{
	switch(aDirection)
		{
		case EShakeRight:
			// Using shake to the right for skip gesture
			if (RadioPlayer().CurrentTrack())
					{
					RadioPlayer().NextTrackL();
					}
			break;
		default:
			// Other directions currently do nothing.
			break;
		}
	}

void CMobblerAppUi::LaunchFileEmbeddedL(const TDesC& aFilename)
	{
	CDocumentHandler* docHandler(CDocumentHandler::NewL(CEikonEnv::Static()->Process()));

	// Set the exit observer so HandleServerAppExit will be called
	docHandler->SetExitObserver(this);

	TDataType emptyDataType = TDataType();
	docHandler->OpenFileEmbeddedL(aFilename, emptyDataType);

	delete docHandler;
	}
 
void CMobblerAppUi::HandleServerAppExit(TInt aReason)
	{
	// Handle closing the handler application
	MAknServerAppExitObserver::HandleServerAppExit(aReason);
	}

void CMobblerAppUi::GoToLastFmL(TInt aCommand)
	{
	CMobblerTrack* currentTrack(CurrentTrack());
	
	if (currentTrack)
		{
		_LIT(KMusicSlash, "music/");
		_LIT(KSlash, "/");
		_LIT(KUnderscoreSlash, "_/");
		_LIT(KPlusEvents, "+events");

		TBuf<255> url(MobblerUtility::LocalLastFmDomainL());
		url.Append(KMusicSlash);
		url.Append(currentTrack->Artist().String());
		url.Append(KSlash);

		switch (aCommand)
			{
			case EMobblerCommandArtistWebPage:
				break;
			case EMobblerCommandAlbumWebPage:
				url.Append(currentTrack->Album().String());
				break;
			case EMobblerCommandTrackWebPage:
				url.Append(KUnderscoreSlash);
				url.Append(currentTrack->Title().String());
				break;
			case EMobblerCommandEventsWebPage:
				url.Append(KPlusEvents);
				break;

			default:
				break;
			}

		// replace space with '+' in the artist name for the URL
		TInt position(url.Find(_L(" ")));
		while (position != KErrNotFound)
			{
			url.Replace(position, 1, _L("+"));
			position = url.Find(_L(" "));
			}
		
#ifndef __WINS__
		iBrowserLauncher->LaunchBrowserEmbeddedL(url);
#endif
		}
	}
// End of File
