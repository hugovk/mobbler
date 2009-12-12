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

#include <aknglobalconfirmationquery.h>
#include <akninfopopupnotecontroller.h>
#include <aknlists.h>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <aknsutils.h>
#include <bautils.h> 
#include <DocumentHandler.h>
#include <EscapeUtils.h>
#include <s32file.h>
#include <sendomfragment.h>

#ifdef __SYMBIAN_SIGNED__
#include <aknswallpaperutils.h>
#include <apgcli.h>

// SW Installer Launcher API
#include <SWInstApi.h>
#include <SWInstDefs.h>

#else // !__SYMBIAN_SIGNED__

#ifndef __WINS__
#include <browserlauncher.h>
#endif // __WINS__

#endif // __SYMBIAN_SIGNED__

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerbrowserview.h"
#include "mobblerliterals.h"
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
#ifdef __SYMBIAN_SIGNED__
const TUid KGesturesInterfaceUid = {0x20026567};
const TUid KDestinationImplUid = {0x20026621};
const TUid KMobblerGesturePlugin5xUid = {0x2002656A};
#else
const TUid KGesturesInterfaceUid = {0xA000B6CF};
const TUid KDestinationImplUid = {0xA000BEB6};
const TUid KMobblerGesturePlugin5xUid = {0xA000B6C2};
#endif

_LIT(KSpace, " ");

CMobblerSystemCloseGlobalQuery* CMobblerSystemCloseGlobalQuery::NewL()
	{
	CMobblerSystemCloseGlobalQuery* self(new(ELeave) CMobblerSystemCloseGlobalQuery);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerSystemCloseGlobalQuery::CMobblerSystemCloseGlobalQuery()
	:CActive(CActive::EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerSystemCloseGlobalQuery::ConstructL()
	{
	iGlobalConfirmationQuery = CAknGlobalConfirmationQuery::NewL();
	iMessage = static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_CLOSE_QUERY).AllocL();
	iGlobalConfirmationQuery->ShowConfirmationQueryL(iStatus, *iMessage, R_AVKON_SOFTKEYS_YES_NO, R_QGN_NOTE_INFO_ANIM);
	SetActive();
	}

CMobblerSystemCloseGlobalQuery::~CMobblerSystemCloseGlobalQuery()
	{
	delete iGlobalConfirmationQuery;
	delete iMessage;
	}

void CMobblerSystemCloseGlobalQuery::RunL()
	{
	if (iStatus >= 0)
		{
		CActiveScheduler::Stop();
		}
	}

void CMobblerSystemCloseGlobalQuery::DoCancel()
	{
	iGlobalConfirmationQuery->CancelConfirmationQuery();
	}

// CMobblerAppUi

void CMobblerAppUi::ConstructL()
	{
	iResourceReader = CMobblerResourceReader::NewL();
	iBitmapCollection = CMobblerBitmapCollection::NewL();
	
	iVolumeUpCallBack = TCallBack(CMobblerAppUi::VolumeUpCallBack, this);
	iVolumeDownCallBack = TCallBack(CMobblerAppUi::VolumeDownCallBack, this);
	
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
	
	TRAP_IGNORE(iDestinations = static_cast<CMobblerDestinationsInterface*>(REComSession::CreateImplementationL(KDestinationImplUid, iDestinationsDtorUid)));
	
#ifdef __S60_50__
	BaseConstructL(EAknTouchCompatible | EAknEnableSkin);
#else
	BaseConstructL(EAknEnableSkin);
#endif
	
	AknsUtils::InitSkinSupportL();
	
	// Create view object
	iSettingView = CMobblerSettingItemListView::NewL();
	iStatusView = CMobblerStatusView::NewL();
	
	iLastFmConnection = CMobblerLastFmConnection::NewL(*this, iSettingView->Username(), iSettingView->Password(), iSettingView->IapId(), iSettingView->BitRate());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFmConnection, 
											 iSettingView->BufferSize(), 
											 iSettingView->EqualizerIndex(), 
											 iSettingView->Volume(), 
											 iSettingView->BitRate());
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFmConnection);
	
	RProcess().SetPriority(EPriorityHigh);
	
#if !defined(__SYMBIAN_SIGNED__) && !defined(__WINS__)
	iBrowserLauncher = CBrowserLauncher::NewL();
#endif
	LoadRadioStationsL();
	
	iMobblerDownload = CMobblerDownload::NewL(*this);
	
	iSleepTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
	iAlarmTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
	
	iWebServicesView = CMobblerWebServicesView::NewL();
	iBrowserView = CMobblerBrowserView::NewL();

	iLastFmConnection->SetModeL(iSettingView->Mode());
	iLastFmConnection->LoadCurrentTrackL();
	
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
	AddViewL(iBrowserView);
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
		delete iGesturePlugin;
		REComSession::DestroyedImplementation(iGesturePluginDtorUid);
		}
	
	if (iDestinations)
		{
		delete iDestinations;
		REComSession::DestroyedImplementation(iDestinationsDtorUid);
		}
	
#if !defined(__SYMBIAN_SIGNED__) && !defined(__WINS__)
	delete iBrowserLauncher;
#endif
	delete iAlarmTimer;
	delete iBitmapCollection;
	delete iCheckForUpdatesObserver;
	delete iDocHandler;
//	delete iFetchLyricsObserver;
	delete iInterfaceSelector;
	delete iLastFmConnection;
	delete iMobblerDownload;
	delete iMusicListener;
	delete iPreviousRadioArtist;
	delete iPreviousRadioPlaylistId;
	delete iPreviousRadioTag;
	delete iPreviousRadioUser;
	delete iRadioPlayer;
	delete iResourceReader;
	delete iSleepTimer;
	delete iSystemCloseGlobalQuery;
	delete iVolumeDownTimer;
	delete iVolumeUpTimer;
	delete iWebServicesHelper;
	}

TBool CMobblerAppUi::AccelerometerGesturesAvailable() const
	{
	return (iGesturePlugin != NULL);
	}

TInt CMobblerAppUi::VolumeUpCallBack(TAny *aSelf)
	{
	CMobblerAppUi* self(static_cast<CMobblerAppUi*>(aSelf));
	
	self->iRadioPlayer->VolumeUp();
	
	if (self->iStatusView->StatusControl())
		{
		self->iStatusView->StatusControl()->VolumeChanged();
		}
	
	return KErrNone;
	}

TInt CMobblerAppUi::VolumeDownCallBack(TAny *aSelf)
	{
	CMobblerAppUi* self(static_cast<CMobblerAppUi*>(aSelf));
	
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
				iRadioPlayer->StopL();
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
					TRAP_IGNORE(iRadioPlayer->SkipTrackL());
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

void CMobblerAppUi::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword, TBool aAndSaveToSettings)
	{
	iLastFmConnection->SetDetailsL(aUsername, aPassword);
	if (aAndSaveToSettings)
		{
		iSettingView->SetDetailsL(aUsername, aPassword);
		}
	}

void CMobblerAppUi::SetIapIDL(TUint32 aIapId)
	{
	iLastFmConnection->SetIapIdL(aIapId);
	}

void CMobblerAppUi::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	iRadioPlayer->SetPreBufferSize(aBufferSize);
	}

void CMobblerAppUi::SetBitRateL(TInt aBitRate)
	{
	iLastFmConnection->SetBitRate(aBitRate);
	iRadioPlayer->SetBitRateL(aBitRate);
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

CMobblerLastFmConnection& CMobblerAppUi::LastFmConnection() const
	{
	return *iLastFmConnection;
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

CMobblerDestinationsInterface* CMobblerAppUi::Destinations() const
	{
	return iDestinations;
	}

HBufC* CMobblerAppUi::MusicAppNameL() const
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
	
	TBuf<KMobblerMaxQueryDialogLength> tag;
	TBuf<KMobblerMaxQueryDialogLength> artist;
	TBuf<KMobblerMaxQueryDialogLength> user;
	
	switch (aCommand)
		{
		case EAknSoftkeyExit:
		case EEikCmdExit:
			// Send application to the background to give the user
			// a sense of a really fast shutdown. Sometimes the thread
			// doesn't shut down instantly, so best to do this without
			// interferring with the user's ability to do something
			// else with their phone.
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			/// Check if scrobblable first and save queue
			iLastFmConnection->TrackStoppedL(currentTrack);
			iRadioPlayer->StopL();
			Exit();
			break;
		case EAknSoftkeyBack:
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			break;
		case EMobblerCommandOnline:
			iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOnline);
			iSettingView->SetModeL(CMobblerLastFmConnection::EOnline);
			break;
		case EMobblerCommandOffline:
			iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOffline);
			iSettingView->SetModeL(CMobblerLastFmConnection::EOffline);
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
			
			if (iLastFmConnection->Mode() != CMobblerLastFmConnection::EOnline && GoOnlineL())
				{
				iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOnline);
				}
				
			if (iLastFmConnection->Mode() == CMobblerLastFmConnection::EOnline)
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
			TBuf<KMobblerMaxQueryDialogLength> search;
			CAknTextQueryDialog* userDialog(new(ELeave) CAknTextQueryDialog(search));
			userDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			TInt resourceId;
			switch (aCommand)
				{
				case EMobblerCommandSearchTrack:
					resourceId = R_MOBBLER_SEARCH_TRACK_PROMPT;
					break;
				case EMobblerCommandSearchAlbum:
					resourceId = R_MOBBLER_SEARCH_ALBUM_PROMPT;
					break;
				case EMobblerCommandSearchArtist:
					resourceId = R_MOBBLER_RADIO_ENTER_ARTIST;
					break;
				case EMobblerCommandSearchTag:
					resourceId = R_MOBBLER_RADIO_ENTER_TAG;
					break;
				default:
					resourceId = R_MOBBLER_SEARCH;
					break;
				}
			userDialog->SetPromptL(iResourceReader->ResourceL(resourceId));
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
			iCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, EFalse);
			iLastFmConnection->CheckForUpdateL(*iCheckForUpdatesObserver);
			}
			break;
/*		case EMobblerCommandFetchLyrics:
			{
			if (currentTrack)
				{
				delete iFetchLyricsObserver;
				iFetchLyricsObserver = CMobblerFlatDataObserverHelper::NewL(
											*iLastFmConnection, *this, ETrue);
				iLastFmConnection->FetchLyricsL(currentTrack->Artist().String8(), 
												currentTrack->Title().String8(), 
												*iFetchLyricsObserver);
				}
			}
			break;
*/
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
				iLastFmConnection->Mode() == CMobblerLastFmConnection::EOnline)
				{
				iRadioPlayer->SkipTrackL();
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
					case EMobblerCommandRadioPlaylist:
						RadioStartL(iPreviousRadioStation, iPreviousRadioPlaylistId, EFalse);
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
						iLastFmConnection->TrackLoveL(currentTrack->Artist().String8(), currentTrack->Title().String8());
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
					iLastFmConnection->TrackBanL(currentRadioTrack->Artist().String8(), currentRadioTrack->Title().String8());
					iRadioPlayer->SkipTrackL();
					}
				}
			
			break;
		case EMobblerCommandPlus:
			
			if (currentTrack)
				{
				CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
				CleanupStack::PushL(list);
				
				CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
				CleanupStack::PushL(popup);
				
				list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
				
				popup->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_CURRENT_TRACK));
				
				list->CreateScrollBarFrameL(ETrue);
				list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
				
				CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(11));
				CleanupStack::PushL(items);
				
				// Add the first menu item and append the shortcut key 
				HBufC* menuText(iResourceReader->ResourceL(R_MOBBLER_VISIT_LASTFM_MENU).AllocLC());
				const TInt KTextLimit(CEikMenuPaneItem::SData::ENominalTextLength);
				_LIT(KShortcut0, " (0)");
				TBuf<KTextLimit> newText(menuText->Left(KTextLimit - KShortcut0().Length()));
				newText.Append(KShortcut0);
				CleanupStack::PopAndDestroy(menuText);
				menuText = newText.AllocLC();
				items->AppendL(*menuText);
				CleanupStack::PopAndDestroy(menuText);
				
				// Add the other menu items
				items->AppendL(iResourceReader->ResourceL(R_MOBBLER_VIEW_ARTIST_BIO));
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
					if (iLastFmConnection->Mode() != CMobblerLastFmConnection::EOnline && GoOnlineL())
						{
						iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOnline);
						}
					
					if (iLastFmConnection->Mode() == CMobblerLastFmConnection::EOnline)
						{
						switch (list->CurrentItemIndex())
							{
							case EPlusOptionVisitLastFm:
								HandleCommandL(EMobblerCommandVisitWebPage);
								break;
							case EPlusOptionViewArtistBio:
								{
								ActivateLocalViewL(iBrowserView->Id(), TUid::Uid(EMobblerCommandArtistBio), currentTrack->Artist().String8());
								}
								break;
							case EPlusOptionShareTrack:
							case EPlusOptionShareArtist:
							case EPlusOptionPlaylistAddTrack:
								{
								if (CurrentTrack())
									{
									delete iWebServicesHelper;
									iWebServicesHelper = CMobblerWebServicesHelper::NewL(*this);
									switch (list->CurrentItemIndex())
										{
										case EPlusOptionShareTrack: iWebServicesHelper->TrackShareL(*CurrentTrack()); break;
										case EPlusOptionShareArtist: iWebServicesHelper->ArtistShareL(*CurrentTrack()); break;
										case EPlusOptionPlaylistAddTrack: iWebServicesHelper->PlaylistAddL(*CurrentTrack()); break;
										}
									}
								else
									{
									// TODO: display an error
									}
								}
								break;
							case EPlusOptionSimilarArtists:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarArtists), currentTrack->Artist().String8());
								break;
							case EPlusOptionSimilarTracks:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarTracks), currentTrack->MbTrackId().String8());
								break;
							case EPlusOptionEvents:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistEvents), currentTrack->Artist().String8());
								break;
							case EPlusOptionArtistShoutbox:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistShoutbox), currentTrack->Artist().String8());
								break;
							case EPlusOptionTopAlbums:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopAlbums), currentTrack->Artist().String8());
								break;
							case EPlusOptionTopTracks:
								ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTracks), currentTrack->Artist().String8());
								break;
							case EPlusOptionTopTags:
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
				CleanupStack::Pop(items);
				
				popupList->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_VISIT_LASTFM));
				
				list->SetCurrentItemIndex(1);
				TInt popupOk(popupList->ExecuteLD());
				CleanupStack::Pop(); // popupList->ExecuteLD()
				
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
							if (CurrentTrack()->Album().String().Length() > 0)
								{
								GoToLastFmL(EMobblerCommandAlbumWebPage);
								break;
								}
							// else no album so it's an event; 
							// intentional fall-through
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
			iLastFmConnection->ToggleScrobblingL();
			iStatusView->DrawDeferred();
			break;
#ifdef __SYMBIAN_SIGNED__
		case EMobblerCommandSetAsWallpaper:
			{
			TInt error(SetAlbumArtAsWallpaper());
			TInt resourceId(R_MOBBLER_NOTE_WALLPAPER_SET);
			if (error != KErrNone)
				{
				resourceId = R_MOBBLER_NOTE_WALLPAPER_NOT_SET;
				}
			CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
			note->ExecuteLD(iResourceReader->ResourceL(resourceId));
			}
			break;
#endif
		case EMobblerCommandSleepTimer:
			ActivateLocalViewL(iSettingView->Id(),
								TUid::Uid(CMobblerSettingItemListView::ESleepTimer),
								KNullDesC8);
			break;
		case EMobblerCommandAlarm:
			CAknQueryDialog* disclaimerDlg(CAknQueryDialog::NewL());
			if(disclaimerDlg->ExecuteLD(R_MOBBLER_OK_CANCEL_QUERY_DIALOG, 
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
					if (!iLastFmConnection->ExportQueueToLogFileL())
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
		case EMobblerCommandLanguagePatches:
			_LIT(KLanguagePatchesUrl, "http://code.google.com/p/mobbler/downloads/list?can=2&q=Type-Language&sort=summary&colspec=Filename+Uploaded");
			OpenWebBrowserL(KLanguagePatchesUrl);
			break;
		case EMobblerCommandQrCode:
			_LIT(KQrCodeFile, "C:Mobbler.png");
			LaunchFileEmbeddedL(KQrCodeFile);
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
			case EMobblerCommandRadioPlaylist:
				delete iPreviousRadioPlaylistId;
				iPreviousRadioPlaylistId = CMobblerString::NewL(aRadioOption->String());
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
	
	CMobblerLastFmConnection::TRadioStation station(CMobblerLastFmConnection::EPersonal);
	switch (aRadioStation)
		{
		case EMobblerCommandRadioArtist:
			station = CMobblerLastFmConnection::EArtist;
			break;
		case EMobblerCommandRadioTag:
			station = CMobblerLastFmConnection::ETag;
			break;
		case EMobblerCommandRadioUser:
			station = CMobblerLastFmConnection::EPersonal;
			break;
		case EMobblerCommandRadioRecommendations:
			station = CMobblerLastFmConnection::ERecommendations;
			break;
		case EMobblerCommandRadioPersonal:
			station = CMobblerLastFmConnection::EPersonal;
			break;
		case EMobblerCommandRadioLoved:
			station = CMobblerLastFmConnection::ELovedTracks;
			break;
		case EMobblerCommandRadioNeighbourhood:
			station = CMobblerLastFmConnection::ENeighbourhood;
			break;
		case EMobblerCommandRadioPlaylist:
			station = CMobblerLastFmConnection::EPlaylist;
			break;
		default:
			station = CMobblerLastFmConnection::EPersonal;
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

CMobblerLastFmConnection::TMode CMobblerAppUi::Mode() const
	{
	return iLastFmConnection->Mode();
	}

CMobblerLastFmConnection::TState CMobblerAppUi::State() const
	{
	return iLastFmConnection->State();
	}

TBool CMobblerAppUi::ScrobblingOn() const
	{
	return iLastFmConnection->ScrobblingOn();
	}

void CMobblerAppUi::HandleStatusPaneSizeChange()
	{
	}

void CMobblerAppUi::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if (aObserver == iCheckForUpdatesObserver)
			{
			// we have just sucessfully checked for updates
			// so don't do it again for another week
			TTime now;
			now.UniversalTime();
			now += TTimeIntervalDays(KUpdateIntervalDays);
			iSettingView->SetNextUpdateCheckL(now);
			
			TVersion version;
			TBuf8<KMaxMobblerTextSize> location;
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
					TBool yes(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_UPDATE)));
					
					if (yes)
						{
						iMobblerDownload->DownloadL(location, iLastFmConnection->IapId());
						}
					}
				else
					{
					CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NO_UPDATE));
					}
				}
			}
/*		else if (aObserver == iFetchLyricsObserver)
			{
			DUMPDATA(aData, _L("lyricsdata.txt"));
			_LIT(KLyricsFilename, "c:\\mobblerlyrics.txt");
			//_LIT8(KElementStart, "start");
			//_LIT8(KElementSg, "sq");
			_LIT8(KElementTx, "tx");
			_LIT8(KElement200, "200");
			_LIT8(KElement300, "300");
			
			RFileWriteStream file;
			CleanupClosePushL(file);
			file.Replace(CCoeEnv::Static()->FsSession(), KLyricsFilename, EFileWrite);
			
			// Create the XML reader and DOM fragement and associate them with each other
			CSenXmlReader* xmlReader(CSenXmlReader::NewL());
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment(CSenDomFragment::NewL());
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			xmlReader->ParseL(aData);
			
			// Get the error code
			const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));
			
			if (statusText && (statusText->CompareF(KElement200) == 0))
				{
				LOG(_L8("200 - ok"));
				const TDesC8* lyricsText(domFragment->AsElement().AttrValue(KElementTx));
				file.WriteL(*lyricsText);
				}
			else if (statusText && (statusText->CompareF(KElement300) == 0))
				{
				LOG(_L8("300 - TESTING LIMITED"));
				const TDesC8* lyricsText(domFragment->AsElement().AttrValue(KElementTx));
				file.WriteL(*lyricsText);
				}
			else if (statusText)
				{
				LOG(_L8("statusText"));
				file.WriteL(*statusText);
				}
			else
				{
				// error!
				LOG(_L8("error!"));
				file.WriteL(aData);
				}
			CleanupStack::PopAndDestroy(2);
			
			CleanupStack::PopAndDestroy(&file);
			
			LaunchFileEmbeddedL(KLyricsFilename);
			}
*/
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
				iCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, EFalse);
				iLastFmConnection->CheckForUpdateL(*iCheckForUpdatesObserver);
				}
			}
		
		// See if there's better album art online
		if (CurrentTrack())
			{
			CurrentTrack()->DownloadAlbumArtL();
			}
		}
	}

void CMobblerAppUi::HandleLastFmErrorL(CMobblerLastFmError& aError)
	{
	// iStatusView->DrawDeferred();
	
	CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
	note->ExecuteLD(aError.Text());
	}

void CMobblerAppUi::HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus)
	{
	// iStatusView->DrawDeferred();
	
	HBufC* noteText(HBufC::NewLC(KMaxMobblerTextSize));

	noteText->Des().Append(iResourceReader->ResourceL(R_MOBBLER_NOTE_COMMS_ERROR));
	noteText->Des().Append(KSpace);
	noteText->Des().AppendNum(aStatusCode);
	noteText->Des().Append(KSpace);
	
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

void CMobblerAppUi::HandleTrackNowPlayingL(const CMobblerTrackBase& /*aTrack*/)
	{
	// Tell the status view that the track has changed
//	iStatusView->DrawDeferred();

#ifdef __SYMBIAN_SIGNED__
	iWallpaperSet = EFalse;
	SetAlbumArtAsWallpaper(ETrue);
#endif
	}

void CMobblerAppUi::HandleTrackSubmitted(const CMobblerTrackBase& /*aTrack*/)
	{
	iStatusView->DrawDeferred();
	++iTracksSubmitted;
	--iTracksQueued;
	}

void CMobblerAppUi::HandleTrackQueuedL(const CMobblerTrackBase& /*aTrack*/)
	{
/*	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}*/
	// update the track queued count and change the status bar text
	++iTracksQueued;
	}

void CMobblerAppUi::HandleTrackDequeued(const CMobblerTrackBase& /*aTrack*/)
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
		iSettingView->SetModeL(CMobblerLastFmConnection::EOnline);
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

void CMobblerAppUi::StatusDrawNow()
	{
	if (iStatusView)
		{
		iStatusView->DrawNow();
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
		
		iPreviousRadioStation = readStream.ReadInt32L();
		if (iPreviousRadioStation <= EMobblerCommandRadioEnumFirst ||
			iPreviousRadioStation >= EMobblerCommandRadioEnumLast)
			{
			iPreviousRadioStation = EMobblerCommandRadioPersonal;
			}
		
		TBuf<KMaxMobblerTextSize> radio;
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
		TBool loadIt(EFalse);
		TRAP_IGNORE(loadIt = readStream.ReadInt8L());
		if (loadIt)
			{
			readStream >> radio;
			delete iPreviousRadioPlaylistId;
			iPreviousRadioPlaylistId = CMobblerString::NewL(radio);
			}
		
		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		iPreviousRadioStation = EMobblerCommandRadioPersonal;
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
		
		if (iPreviousRadioPlaylistId)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioPlaylistId->String();
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
	
	if (sleepMinutes == 0)
		{
		 // Use a one second delay
		TTimeIntervalSeconds oneSecond(1);
		iTimeToSleep += oneSecond;
		}
	
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

		if (iLastFmConnection->IapId() != iSettingView->AlarmIapId())
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
		
		iRadioPlayer->SetVolume(iSettingView->AlarmVolume());

		TInt station(iSettingView->AlarmStation() + EMobblerCommandRadioArtist);

		switch (station)
			{
			case EMobblerCommandRadioArtist:			// intentional fall-through
			case EMobblerCommandRadioTag:				// intentional fall-through
			case EMobblerCommandRadioUser:				// intentional fall-through
				CMobblerString* option(CMobblerString::NewL(iSettingView->AlarmOption()));
				CleanupStack::PushL(option);
				RadioStartL(station, option);
				CleanupStack::PopAndDestroy(option);
				break;
			case EMobblerCommandRadioRecommendations:	// intentional fall-through
			case EMobblerCommandRadioPersonal:			// intentional fall-through
			case EMobblerCommandRadioLoved:				// intentional fall-through
			case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
			default:
				RadioStartL(station, NULL);
				break;
			}
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
	iLastFmConnection->TrackStoppedL(CurrentTrack());
	iRadioPlayer->StopL();
	
#ifdef _DEBUG
	CEikonEnv::Static()->InfoMsg(_L("Sleep!"));
#endif
	CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
	note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_EXPIRED));
	
	switch (iSettingView->SleepTimerAction())
		{
		case CMobblerSettingItemListSettings::EStopPlaying:
			break;
		case CMobblerSettingItemListSettings::EGoOffline:
			HandleCommandL(EMobblerCommandOffline);
			break;
		case CMobblerSettingItemListSettings::ExitMobber:
			iAvkonAppUi->RunAppShutter();
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
					RadioPlayer().SkipTrackL();
					}
			break;
		default:
			// Other directions currently do nothing.
			break;
		}
	}

void CMobblerAppUi::LaunchFileEmbeddedL(const TDesC& aFilename)
	{
	if (!iDocHandler)
		{
		iDocHandler = CDocumentHandler::NewL(CEikonEnv::Static()->Process());
		}
	
	// Set the exit observer so HandleServerAppExit will be called
	iDocHandler->SetExitObserver(this);
	
	TDataType emptyDataType = TDataType();
	iDocHandler->OpenFileEmbeddedL(aFilename, emptyDataType);
	}
 
void CMobblerAppUi::HandleServerAppExit(TInt aReason)
	{
	// Handle closing the handler application
	MAknServerAppExitObserver::HandleServerAppExit(aReason);
	}

void CMobblerAppUi::GoToLastFmL(TInt aCommand, const TDesC8& aEventId)
	{
	CMobblerTrack* currentTrack(CurrentTrack());
	TBuf<KMaxMobblerTextSize> url(MobblerUtility::LocalLastFmDomainL());

	if (aCommand == EMobblerCommandEventWebPage)
		{
		_LIT(KEventSlash, "event/");
		
		url.Append(KEventSlash);
		TBuf<KMaxMobblerTextSize> eventId;
		eventId.Copy(aEventId);
		url.Append(eventId);
		
		OpenWebBrowserL(url);
		}
	else if (currentTrack) // for artist/album/track/events webpage
		{
		_LIT(KMusicSlash, "music/");
		_LIT(KSlash, "/");
		_LIT(KUnderscoreSlash, "_/");
		_LIT(KPlusEvents, "+events");
		_LIT(KPlus, "+");
		
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
		
		// Replace space with '+' in the artist name for the URL
		TInt position(url.Find(KSpace));
		while (position != KErrNotFound)
			{
			url.Replace(position, 1, KPlus);
			position = url.Find(KSpace);
			}
		
		OpenWebBrowserL(url);
		}
	}

void CMobblerAppUi::GoToMapL(const TDesC8& aName, const TDesC8& aLatitude, const TDesC8& aLongitude)
	{
	_LIT(KMapKmlFilename, "c:\\mobblermap.kml");
	
	_LIT8(KMapKmlFormat,	"<kml xmlns=\"http://earth.google.com/kml/2.0\">\r\n"
							"\t<Placemark>\r\n "
							"\t\t<name>%S</name>\r\n"
							"\t\t<description>Created by Mobbler</description>\r\n" 
							"\t\t<Point>\r\n "
							"\t\t\t<coordinates>%S,%S</coordinates>\r\n"
							"\t\t</Point>\r\n"
							"\t</Placemark>\r\n"
							"</kml>\r\n");
	
	HBufC8* kmlFileContents(HBufC8::NewLC(KMapKmlFormat().Length() + aName.Length() + aLongitude.Length() + aLatitude.Length()));
	
	kmlFileContents->Des().Format(KMapKmlFormat, &aName, &aLongitude, &aLatitude);
	
	RFile file;
	CleanupClosePushL(file);
	file.Replace(CCoeEnv::Static()->FsSession(), KMapKmlFilename, EFileWrite);
	file.Write(*kmlFileContents);
	CleanupStack::PopAndDestroy(&file);
	
	CleanupStack::PopAndDestroy(kmlFileContents);
	
	CDocumentHandler* docHandler(CDocumentHandler::NewL(CEikonEnv::Static()->Process()));
	CleanupStack::PushL(docHandler);
	TDataType emptyDataType = TDataType();
	TInt error(docHandler->OpenFileEmbeddedL(KMapKmlFilename, emptyDataType));
	CleanupStack::PopAndDestroy(docHandler);
	
	if (error != KErrNone)
		{
		_LIT(KMapURLFormat, "http://maptwits.com/displaymap.php?lat=%S&long=%S");
		
		CMobblerString* longitude(CMobblerString::NewL(aLongitude));
		CleanupStack::PushL(longitude);
		CMobblerString* latitude(CMobblerString::NewL(aLatitude));
		CleanupStack::PushL(latitude);
		CMobblerString* name(CMobblerString::NewL(aName));
		CleanupStack::PushL(name);
		
		HBufC* url(HBufC::NewLC(KMapURLFormat().Length() + longitude->String().Length() + latitude->String().Length()));
		
		url->Des().Format(KMapURLFormat, &latitude->String(), &longitude->String());
		
		OpenWebBrowserL(*url);
		
		CleanupStack::PopAndDestroy(4);
		}
	}

void CMobblerAppUi::OpenWebBrowserL(const TDesC& aUrl)
	{
	TBuf<KMaxMobblerTextSize> url(aUrl);
	
	// Convert to UTF-8
	HBufC8* utf8(EscapeUtils::ConvertFromUnicodeToUtf8L(url));
	url.Copy(*utf8);
	delete utf8;
	
	// Escape encode things like Ä and ö
	HBufC16* encode(EscapeUtils::EscapeEncodeL(url, EscapeUtils::EEscapeNormal));
	CleanupStack::PushL(encode);
	
#ifdef __SYMBIAN_SIGNED__
	const TUid KBrowserUid = {0x10008D39};
	TApaTaskList taskList(CEikonEnv::Static()->WsSession());
	TApaTask task(taskList.FindApp(KBrowserUid));
	if(task.Exists())
		{
		task.BringToForeground();
		HBufC8* param8(HBufC8::NewLC(encode->Length()));
		param8->Des().Append(*encode);
		task.SendMessage(TUid::Uid(0), *param8); // UID not used
		CleanupStack::PopAndDestroy(param8);
		}
	else
		{
		RApaLsSession apaLsSession;
		CleanupClosePushL(apaLsSession);
		if(!apaLsSession.Handle())
			{
			User::LeaveIfError(apaLsSession.Connect());
			}
		TThreadId thread;
		User::LeaveIfError(apaLsSession.StartDocument(*encode, KBrowserUid, thread));
		CleanupStack::PopAndDestroy(&apaLsSession);
		}
#else // !__SYMBIAN_SIGNED__
#ifndef __WINS__
	iBrowserLauncher->LaunchBrowserEmbeddedL(url);
#endif
#endif // __SYMBIAN_SIGNED__
	
	CleanupStack::PopAndDestroy(encode);
	}

void CMobblerAppUi::HandleSystemEventL(const TWsEvent& aEvent)
	{
	switch (*(TApaSystemEvent*)(aEvent.EventData()))
		{
		case EApaSystemEventShutdown:
			{
			if (!iSystemCloseGlobalQuery)
				{
				iSystemCloseGlobalQuery = CMobblerSystemCloseGlobalQuery::NewL();
				CActiveScheduler::Start();
				
				switch (iSystemCloseGlobalQuery->iStatus.Int())
					{
					case EAknSoftkeyYes:
						{
						CAknAppUi::HandleSystemEventL(aEvent);
						}
						break;
					default:
						break;
					}
				
				delete iSystemCloseGlobalQuery;
				iSystemCloseGlobalQuery = NULL;
				}
			}
			break;
		default:
			CAknAppUi::HandleSystemEventL(aEvent);
			break;
		}
	}

void CMobblerAppUi::HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination)
	{
	if (aEvent.Type() == KAknUidValueEndKeyCloseEvent)
		{
		// Do nothing for the red end key, 
		// so Mobbler is minimised but still running
		}
	else
		{
		CAknViewAppUi::HandleWsEventL(aEvent, aDestination);
		}
	}

#ifdef __SYMBIAN_SIGNED__
TInt CMobblerAppUi::SetAlbumArtAsWallpaper(TBool aAutomatically)
	{
	TInt error(KErrUnknown);
	_LIT(KWallpaperFile, "C:\\System\\Data\\Mobbler\\wallpaperimage.mbm");
	
	if (!aAutomatically || iSettingView->AutomaticWallpaper())
		{
		LOG(_L8("Set as wallpaper"));
		if (!iWallpaperSet &&
			CurrentTrack() && 
			CurrentTrack()->AlbumArt() && 
			CurrentTrack()->AlbumArt()->Bitmap())
			{
			// The current track has album art and it has finished loading
			CCoeEnv::Static()->FsSession().MkDirAll(KWallpaperFile);
			error = CurrentTrack()->AlbumArt()->Bitmap(ETrue)->Save(KWallpaperFile);
			if (error == KErrNone)
				{
				error = AknsWallpaperUtils::SetIdleWallpaper(KWallpaperFile, NULL);
				LOG2(_L8("Set as wallpaper"), error);
				if (error == KErrNone)
					{
					iWallpaperSet = ETrue;
					}
				}
			}
		}
	return error;
	}
#endif

TBool CMobblerAppUi::DetailsNeeded()
	{
	if ((iSettingView->Username().Compare(iResourceReader->ResourceL(R_MOBBLER_USERNAME)) == 0) &&
		(iSettingView->Password().Compare(_L("password")) == 0))
		{
		return ETrue;
		}
	return EFalse;
	}

// End of file
