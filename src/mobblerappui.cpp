/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade
Copyright (C) 2008, 2009  Steve Punter
Copyright (C) 2009  James Aley
Copyright (C) 2010  gw111zz

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
#include <senxmlutils.h>

#ifdef __SYMBIAN_SIGNED__
#include <aknswallpaperutils.h>
#include <apgcli.h>

#else // !__SYMBIAN_SIGNED__

#ifndef __WINS__
#include <browserlauncher.h>
#endif // __WINS__

#endif // __SYMBIAN_SIGNED__

#include <mobbler/mobblercontentlistinginterface.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerbrowserview.h"
#include "mobblerhtmltemplates.h"
#include "mobblerliterals.h"
#ifdef __SYMBIAN_SIGNED__
#include "mobblerlocation.h"
#endif
#include "mobblerlogging.h"
#include "mobblermusiclistener.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstatusview.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"
#include "mobblerwebservicesview.h"

_LIT(KRadioFile, "C:radiostations.dat");
_LIT(KSearchFile, "C:searchterms.dat");
_LIT(KSpace, " ");

// Gesture interface
#ifdef __SYMBIAN_SIGNED__
const TUid KGesturesInterfaceUid = {0x20039AFD};
const TUid KDestinationImplUid = {0x20039B08};
const TUid KMobblerGesturePlugin5xUid = {0x20039B00};
const TUid KContentListingImplUid = {0x20039B05};
#else
const TUid KGesturesInterfaceUid = {0xA000B6CF};
const TUid KDestinationImplUid = {0xA000BEB6};
const TUid KMobblerGesturePlugin5xUid = {0xA000B6C2};
const TUid KContentListingImplUid = {0xA000BEB3};
#endif

CMobblerGlobalQuery* CMobblerGlobalQuery::NewL(TInt aResourceId)
	{
	TRACER_AUTO;
	CMobblerGlobalQuery* self(new(ELeave) CMobblerGlobalQuery());
	CleanupStack::PushL(self);
	self->ConstructL(aResourceId);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerGlobalQuery::CMobblerGlobalQuery()
	:CActive(CActive::EPriorityStandard)
	{
	TRACER_AUTO;
	CActiveScheduler::Add(this);
	}

void CMobblerGlobalQuery::ConstructL(TInt aResourceId)
	{
	TRACER_AUTO;
	TInt softkeys(aResourceId == R_MOBBLER_CLOSE_QUERY ? 
								 R_AVKON_SOFTKEYS_YES_NO : 
								 R_AVKON_SOFTKEYS_OK_EMPTY);
	iGlobalConfirmationQuery = CAknGlobalConfirmationQuery::NewL();
	iMessage = static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aResourceId).AllocL();
	iGlobalConfirmationQuery->ShowConfirmationQueryL(iStatus, *iMessage, softkeys, R_QGN_NOTE_WARNING_ANIM);
	SetActive();
	}

CMobblerGlobalQuery::~CMobblerGlobalQuery()
	{
	TRACER_AUTO;
	delete iGlobalConfirmationQuery;
	delete iMessage;
	}

void CMobblerGlobalQuery::RunL()
	{
	TRACER_AUTO;
	CActiveScheduler::Stop();
	}

void CMobblerGlobalQuery::DoCancel()
	{
	TRACER_AUTO;
	iGlobalConfirmationQuery->CancelConfirmationQuery();
	}

// CMobblerAppUi

void CMobblerAppUi::ConstructL()
	{
	TRACER_AUTO;
	iWebServicesHelper = CMobblerWebServicesHelper::NewL(*this);
	
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
	
	iLastFmConnection = CMobblerLastFmConnection::NewL(*this, iSettingView->Settings().Username(), iSettingView->Settings().Password(), iSettingView->Settings().IapId(), iSettingView->Settings().BitRate());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFmConnection, 
											 iSettingView->Settings().BufferSize(), 
											 iSettingView->Settings().EqualizerIndex(), 
											 iSettingView->Settings().Volume(), 
											 iSettingView->Settings().BitRate());
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFmConnection);
	
	TRAP_IGNORE(iContentListing = static_cast<CMobblerContentListingInterface*>(REComSession::CreateImplementationL(KContentListingImplUid, iContentListingDtorUid)));
	
	RProcess().SetPriority(EPriorityHigh);
	
	LoadRadioStationsL();
	
	iWebServicesView = CMobblerWebServicesView::NewL();
	iBrowserView = CMobblerBrowserView::NewL();

	iLastFmConnection->SetModeL(iSettingView->Settings().Mode());
	iLastFmConnection->LoadCurrentTrackL();
	
	if (iSettingView->Settings().AlarmOn())
		{
		// If the time has already passed, no problem, the timer will 
		// simply expire immediately with KErrUnderflow.
		if (!iAlarmTimer)
			{
			iAlarmTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
			}
		iAlarmTimer->At(iSettingView->Settings().AlarmTime());
		}
	
	// Attempt to load gesture plug-in
	iGesturePlugin = NULL;
	TRAP_IGNORE(LoadGesturesPluginL());
	UpdateAccelerometerGesturesL();
	
	AddViewL(iWebServicesView);
	AddViewL(iBrowserView);
	AddViewL(iSettingView);
	AddViewL(iStatusView);
	ActivateLocalViewL(iStatusView->Id());
	}

CMobblerAppUi::CMobblerAppUi()
	: iSleepAfterTrackStopped(EFalse)
	{
	TRACER_AUTO;
	}

CMobblerAppUi::~CMobblerAppUi()
	{
	TRACER_AUTO;
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
	delete iArtistBiographyObserver;
	delete iBitmapCollection;
	delete iAutoCheckForUpdatesObserver;
	delete iManualCheckForUpdatesObserver;
	delete iDocHandler;
	delete iLyricsObserver;
	delete iInterfaceSelector;
	delete iLastFmConnection;
	delete iMobblerDownload;
	delete iMusicListener;
	delete iPreviousRadioArtist;
	delete iPreviousRadioCustom;
	delete iPreviousRadioGroup;
	delete iPreviousRadioTag;
	delete iPreviousRadioUser;
	delete iPreviousSearchTrack;
	delete iPreviousSearchAlbum;
	delete iPreviousSearchArtist;
	delete iPreviousSearchTag;
	delete iRadioPlayer;
	delete iResourceReader;
	delete iSleepTimer;
	delete iSystemCloseGlobalQuery;
	delete iOldScrobbleGlobalQuery;
	delete iVolumeDownTimer;
	delete iVolumeUpTimer;
	delete iWebServicesHelper;
#ifdef __SYMBIAN_SIGNED__
	delete iLocation;
	delete iLocalEventsObserver;
#endif
	delete iTwitterAuthObserver;
	delete iTwitterFollowObserver;
	
	if (iContentListing)
		{
		delete iContentListing;
		REComSession::DestroyedImplementation(iContentListingDtorUid);
		}
	}

TBool CMobblerAppUi::AccelerometerGesturesAvailable() const
	{
	TRACER_AUTO;
	return (iGesturePlugin != NULL);
	}

TInt CMobblerAppUi::VolumeUpCallBack(TAny *aSelf)
	{
	TRACER_AUTO;
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
	TRACER_AUTO;
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
	TRACER_AUTO;
	// don't bother if there's a current music player track
	if ((CurrentTrack() && 
		(CurrentTrack()->StreamId().Compare(KNullDesC8) == 0)))
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
	TRACER_AUTO;
	iLastFmConnection->SetDetailsL(aUsername, aPassword);
	if (aAndSaveToSettings)
		{
		iSettingView->Settings().SetUsername(aUsername);
		iSettingView->Settings().SetPassword(aPassword);
		}
	}

void CMobblerAppUi::SetIapIDL(TUint32 aIapId)
	{
	TRACER_AUTO;
	iLastFmConnection->SetIapIdL(aIapId);
	}

void CMobblerAppUi::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	TRACER_AUTO;
	iRadioPlayer->SetPreBufferSize(aBufferSize);
	}

void CMobblerAppUi::SetBitRateL(TInt aBitRate)
	{
	TRACER_AUTO;
	iLastFmConnection->SetBitRate(aBitRate);
	iRadioPlayer->SetBitRateL(aBitRate);
	}

void CMobblerAppUi::UpdateAccelerometerGesturesL()
	{
	TRACER_AUTO;
	// If the radio is playing and the setting is on
	if (iGesturePlugin && 
		iRadioPlayer->CurrentTrack() && 
		iSettingView->Settings().AccelerometerGestures())
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
//	TRACER_AUTO;
	const CMobblerTrack* track(iRadioPlayer->CurrentTrack());
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}
	
	return track;
	}

CMobblerTrack* CMobblerAppUi::CurrentTrack()
	{
	TRACER_AUTO;
	CMobblerTrack* track(iRadioPlayer->CurrentTrack());
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}

	return track;
	}

CMobblerLastFmConnection& CMobblerAppUi::LastFmConnection() const
	{
//	TRACER_AUTO;
	return *iLastFmConnection;
	}

CMobblerRadioPlayer& CMobblerAppUi::RadioPlayer() const
	{
//	TRACER_AUTO;
	return *iRadioPlayer;
	}

CMobblerMusicAppListener& CMobblerAppUi::MusicListener() const
	{
//	TRACER_AUTO;
	return *iMusicListener;
	}

CMobblerSettingItemListView& CMobblerAppUi::SettingView() const
	{
//	TRACER_AUTO;
	return *iSettingView;
	}

CMobblerBrowserView& CMobblerAppUi::BrowserView() const
	{
//	TRACER_AUTO;
	return *iBrowserView;
	}

CMobblerDestinationsInterface* CMobblerAppUi::Destinations() const
	{
//	TRACER_AUTO;
	return iDestinations;
	}

CMobblerContentListingInterface* CMobblerAppUi::ContentListing() const
	{
//	TRACER_AUTO;
	return iContentListing;
	}

HBufC* CMobblerAppUi::MusicAppNameL() const
	{
//	TRACER_AUTO;
	return iMusicListener->MusicAppNameL();
	}

void CMobblerAppUi::HandleInstallStartedL()
	{
	TRACER_AUTO;
	RunAppShutter();
	}

void CMobblerAppUi::HandleCommandL(TInt aCommand)
	{
	TRACER_AUTO;
	const CMobblerTrack* const currentTrack(CurrentTrack());
	const CMobblerTrack* const currentRadioTrack(iRadioPlayer->CurrentTrack());
	
	switch (aCommand)
		{
		case EAknSoftkeyExit:
		case EEikCmdExit:
			{
			// Send application to the background to give the user
			// a sense of a really fast shutdown. Sometimes the thread
			// doesn't shut down instantly, so best to do this without
			// interferring with the user's ability to do something
			// else with their phone.
			TApaTask task(iEikonEnv->WsSession());
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			/// Check if scrobblable first and save queue
			iLastFmConnection->TrackStoppedL(currentTrack);
			iRadioPlayer->StopL();
			Exit();
			}
			break;
		case EAknSoftkeyBack:
			{
			TApaTask task(iEikonEnv->WsSession());
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			}
			break;
		case EMobblerCommandOnline:
			iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOnline);
			iSettingView->Settings().SetMode(CMobblerLastFmConnection::EOnline);
			break;
		case EMobblerCommandOffline:
			iLastFmConnection->SetModeL(CMobblerLastFmConnection::EOffline);
			iSettingView->Settings().SetMode(CMobblerLastFmConnection::EOffline);
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
				CMobblerString* username(CMobblerString::NewLC(iSettingView->Settings().Username()));
				ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), username->String8());
				CleanupStack::PopAndDestroy(username);
				}
			
			break;
#ifdef __SYMBIAN_SIGNED__
		case EMobblerCommandLocalEvents:
			if (!iLocation)
				{
				iLocation = CMobblerLocation::NewL(*this);
				}
			iLocation->GetLocationL();
			break;
#endif
		case EMobblerCommandSearchTrack:
		case EMobblerCommandSearchAlbum:
		case EMobblerCommandSearchArtist:
		case EMobblerCommandSearchTag:
			{
			LoadSearchTermsL();
			TBuf<KMobblerMaxQueryDialogLength> search;
			TInt resourceId;
			switch (aCommand)
				{
				case EMobblerCommandSearchTrack:
					resourceId = R_MOBBLER_SEARCH_TRACK_PROMPT;
					if (iPreviousSearchTrack)
						{
						search = iPreviousSearchTrack->String();
						}
					break;
				case EMobblerCommandSearchAlbum:
					resourceId = R_MOBBLER_SEARCH_ALBUM_PROMPT;
					if (iPreviousSearchAlbum)
						{
						search = iPreviousSearchAlbum->String();
						}
					break;
				case EMobblerCommandSearchArtist:
					resourceId = R_MOBBLER_RADIO_ENTER_ARTIST;
					if (iPreviousSearchArtist)
						{
						search = iPreviousSearchArtist->String();
						}
					break;
				case EMobblerCommandSearchTag:
					resourceId = R_MOBBLER_RADIO_ENTER_TAG;
					if (iPreviousSearchTag)
						{
						search = iPreviousSearchTag->String();
						}
					break;
				default:
					resourceId = R_MOBBLER_SEARCH;
					break;
				}
			CAknTextQueryDialog* userDialog(new(ELeave) CAknTextQueryDialog(search));
			userDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			userDialog->SetPromptL(iResourceReader->ResourceL(resourceId));
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				// Save the search term
				switch (aCommand)
					{
					case EMobblerCommandSearchTrack:
						delete iPreviousSearchTrack;
						iPreviousSearchTrack = CMobblerString::NewL(search);
						break;
					case EMobblerCommandSearchAlbum:
						delete iPreviousSearchAlbum;
						iPreviousSearchAlbum = CMobblerString::NewL(search);
						break;
					case EMobblerCommandSearchArtist:
						delete iPreviousSearchArtist;
						iPreviousSearchArtist = CMobblerString::NewL(search);
						break;
					case EMobblerCommandSearchTag:
						delete iPreviousSearchTag;
						iPreviousSearchTag = CMobblerString::NewL(search);
						break;
					default:
						break;
					}
				SaveSearchTermsL();
				
				// Do the search
				CMobblerString* searchString(CMobblerString::NewLC(search));
				ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), searchString->String8());
				CleanupStack::PopAndDestroy(searchString);
				}
			}
			break;
		case EMobblerCommandScrobbleLog:
			{
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), KNullDesC8);
			}
			break;
		case EMobblerCommandCheckForUpdates:
			{
			delete iManualCheckForUpdatesObserver;
			iManualCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, EFalse);
			iLastFmConnection->CheckForUpdateL(*iManualCheckForUpdatesObserver);
			}
			break;
		case EMobblerCommandEditSettings:
			ActivateLocalViewL(iSettingView->Id(), 
								TUid::Uid(CMobblerSettingItemListView::ENormalSettings), 
								KNullDesC8);
			break;
		case EMobblerCommandTwitterChange:
			delete iTwitterAuthObserver;
			iTwitterAuthObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, ETrue);
			if (!iLastFmConnection->QueryTwitterL(CMobblerLastFmConnection::EAccessToken, *iTwitterAuthObserver))
				{
				delete iTwitterAuthObserver;
				iTwitterAuthObserver = NULL;
				}
			break;
		case EMobblerCommandTwitterRemove:
			SettingView().Settings().SetTwitterAuthToken(KNullDesC8);
			SettingView().Settings().SetTwitterAuthTokenSecret(KNullDesC8);
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
					case EMobblerCommandRadioGroup:
						RadioStartL(iPreviousRadioStation, iPreviousRadioGroup, EFalse);
						break;
					case EMobblerCommandRadioCustom:
						RadioStartL(iPreviousRadioStation, iPreviousRadioCustom, EFalse);
						break;
					case EMobblerCommandRadioRecommendations:	// intentional fall-through
					case EMobblerCommandRadioPersonal:			// intentional fall-through
					case EMobblerCommandRadioMix:				// intentional fall-through
					case EMobblerCommandRadioFriends:				// intentional fall-through
					case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
					default:
						RadioStartL(iPreviousRadioStation, NULL, EFalse);
						break;
					}
				}
			break;
		case EMobblerCommandRadioArtist:
		case EMobblerCommandRadioTag:
		case EMobblerCommandRadioUser:
		case EMobblerCommandRadioGroup:
		case EMobblerCommandRadioCustom:
			{
			if (!RadioStartableL())
				{
				break;
				}

			TInt resourceId(R_MOBBLER_RADIO_ENTER_ARTIST);
			TBuf<KMobblerMaxQueryDialogLength> station;
			switch (aCommand)
				{
				case EMobblerCommandRadioArtist:
					{
					if (iPreviousRadioArtist)
						{
						station = iPreviousRadioArtist->String();
						}
					else
						{
						_LIT(KDefault, "Emmy the Great");
						station = KDefault();
						}
					}
					break;
				case EMobblerCommandRadioTag:
					{
					resourceId = R_MOBBLER_RADIO_ENTER_TAG;
					if (iPreviousRadioTag)
						{
						station = iPreviousRadioTag->String();
						}
					else
						{
						_LIT(KDefault, "ukulele");
						station = KDefault();
						}
					}
					break;
				case EMobblerCommandRadioUser:
					{
					resourceId = R_MOBBLER_RADIO_ENTER_USER;
					if (iPreviousRadioUser)
						{
						station = iPreviousRadioUser->String();
						}
					else
						{
						_LIT(KDefault, "eartle");
						station = KDefault();
						}
					}
					break;
				case EMobblerCommandRadioGroup:
					{
					resourceId = R_MOBBLER_RADIO_ENTER_GROUP;
					if (iPreviousRadioGroup)
						{
						station = iPreviousRadioGroup->String();
						}
					else
						{
						_LIT(KDefault, "Mobbler Users");
						station = KDefault();
						}
					}
					break;
				case EMobblerCommandRadioCustom:
					{
					resourceId = R_MOBBLER_RADIO_ENTER_CUSTOM;
					if (iPreviousRadioCustom)
						{
						station = iPreviousRadioCustom->String();
						}
					else
						{
						_LIT(KDefault, "lastfm://group/Mobbler+Users");
						station = KDefault();
						}
					}
					break;
				default:
					break;
				}

			// Ask the user for the new station text
			CAknTextQueryDialog* dialog(new(ELeave) CAknTextQueryDialog(station));
			dialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			dialog->SetPromptL(iResourceReader->ResourceL(resourceId));
			dialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (dialog->RunLD())
				{
				CMobblerString* stationString(CMobblerString::NewLC(station));
				RadioStartL(aCommand, stationString);
				CleanupStack::PopAndDestroy(stationString);
				}
			}
			break;
		case EMobblerCommandRadioRecommendations:	// intentional fall-through
		case EMobblerCommandRadioPersonal:			// intentional fall-through
		case EMobblerCommandRadioMix:				// intentional fall-through
		case EMobblerCommandRadioFriends:			// intentional fall-through
		case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
			RadioStartL(aCommand, NULL);
			break;
		case EMobblerCommandTrackLove:
			// you can love either radio or music player tracks
			
			if (currentTrack)
				{
				if (currentTrack->Love() == CMobblerTrack::ENoLove)
					{
					// There is a current track and it is not already loved
					CAknQueryDialog* dlg(CAknQueryDialog::NewL());
					TBool love(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_LOVE_TRACK)));
					
					if (love)
						{
						// set love to true (if only it were this easy)
						CurrentTrack()->LoveTrackL();
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
				iStatusView->DisplayPlusMenuL();
				}

			break;
		case EMobblerCommandPlusVisitLastFm:
			HandleCommandL(EMobblerCommandVisitWebPage);
			break;
		case EMobblerCommandBiography:
			if (currentTrack)
				{
				delete iArtistBiographyObserver;
				iArtistBiographyObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, ETrue);
				GetBiographyL(ETrue);
				}
			break;
		case EMobblerCommandPlusShareTrack:
		case EMobblerCommandPlusShareArtist:
		case EMobblerCommandPlusShareAlbum:
		case EMobblerCommandPlusPlaylistAddTrack:
			{
			if (CurrentTrack())
				{
				switch (aCommand)
					{
					case EMobblerCommandPlusShareTrack: iWebServicesHelper->TrackShareL(*CurrentTrack()); break;
					case EMobblerCommandPlusShareArtist: iWebServicesHelper->ArtistShareL(*CurrentTrack()); break;
					case EMobblerCommandPlusShareAlbum: iWebServicesHelper->AlbumShareL(*CurrentTrack()); break;
					case EMobblerCommandPlusPlaylistAddTrack: iWebServicesHelper->PlaylistAddL(*CurrentTrack()); break;
					}
				}
			else
				{
				// TODO: display an error
				}
			}
			break;
		case EMobblerCommandPlusSimilarArtists:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarArtists), currentTrack->Artist().String8());
			break;
		case EMobblerCommandPlusSimilarTracks:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarTracks), currentTrack->MbTrackId().String8());
			break;
		case EMobblerCommandPlusEvents:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistEvents), currentTrack->Artist().String8());
			break;
		case EMobblerCommandPlusArtistShoutbox:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistShoutbox), currentTrack->Artist().String8());
			break;
		case EMobblerCommandPlusTopAlbums:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopAlbums), currentTrack->Artist().String8());
			break;
		case EMobblerCommandPlusTopTracks:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTracks), currentTrack->Artist().String8());
			break;
		case EMobblerCommandPlusTopTags:
			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTags), currentTrack->Artist().String8());
			break;
		case EMobblerCommandTrackLyrics:
			{
			if (currentTrack)
				{
				delete iLyricsObserver;
				iLyricsObserver = CMobblerFlatDataObserverHelper::NewL(
											*iLastFmConnection, *this, ETrue);
				iLastFmConnection->FetchLyricsL(currentTrack->Artist().String8(), 
												currentTrack->Title().String8(), 
												*iLyricsObserver);
				}
			}
			break;
			case EMobblerCommandTrackAddTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->AddTagL(*CurrentTrack(), aCommand);
				}
			break;
		case EMobblerCommandTrackRemoveTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->TrackRemoveTagL(*CurrentTrack());
				}
			break;
		case EMobblerCommandAlbumAddTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->AddTagL(*CurrentTrack(), aCommand);
				}
			break;
		case EMobblerCommandAlbumRemoveTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->AlbumRemoveTagL(*CurrentTrack());
				}
			break;
		case EMobblerCommandArtistAddTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->AddTagL(*CurrentTrack(), aCommand);
				}
			break;
		case EMobblerCommandArtistRemoveTag:
			if (CurrentTrack())
				{
				iWebServicesHelper->ArtistRemoveTagL(*CurrentTrack());
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
			{
			CAknQueryDialog* disclaimerDlg(CAknQueryDialog::NewL());
			if(disclaimerDlg->ExecuteLD(R_MOBBLER_OK_CANCEL_QUERY_DIALOG, 
					iResourceReader->ResourceL(R_MOBBLER_ALARM_DISCLAIMER)))
				{
				ActivateLocalViewL(iSettingView->Id(),
									TUid::Uid(CMobblerSettingItemListView::EAlarm),
									KNullDesC8);
				}
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
				_LIT(KLogFile, "c:\\Data\\Mobbler\\.scrobbler.log");
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
			LaunchFileL(KQrCodeFile);
			break;
		default:
			if (aCommand >= EMobblerCommandEqualizerDefault && 
				aCommand <= EMobblerCommandEqualizerMaximum)
				{
				TInt index(aCommand - EMobblerCommandEqualizerDefault - 1);
				RadioPlayer().SetEqualizer(index);
				iSettingView->Settings().SetEqualizerIndex(index);
				return;
				}
			break;
		}
	}

void CMobblerAppUi::RadioStartL(TInt aRadioStation, 
								const CMobblerString* aRadioOption, 
								TBool aSaveStations)
	{
	TRACER_AUTO;
	iPreviousRadioStation = aRadioStation;
	
	// Turn on gesture plug-in
	UpdateAccelerometerGesturesL();
	
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
			case EMobblerCommandRadioGroup:
				delete iPreviousRadioGroup;
				iPreviousRadioGroup = CMobblerString::NewL(aRadioOption->String());
				break;
			case EMobblerCommandRadioCustom:
				delete iPreviousRadioCustom;
				iPreviousRadioCustom = CMobblerString::NewL(aRadioOption->String());
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
		case EMobblerCommandRadioMix:
			station = CMobblerLastFmConnection::EMix;
			break;
		case EMobblerCommandRadioFriends:
			station = CMobblerLastFmConnection::EFriends;
			break;
		case EMobblerCommandRadioRecommendations:
			station = CMobblerLastFmConnection::ERecommendations;
			break;
		case EMobblerCommandRadioPersonal:
			station = CMobblerLastFmConnection::EPersonal;
			break;
		case EMobblerCommandRadioNeighbourhood:
			station = CMobblerLastFmConnection::ENeighbourhood;
			break;
		case EMobblerCommandRadioGroup:
			station = CMobblerLastFmConnection::EGroup;
			break;
		case EMobblerCommandRadioCustom:
			station = CMobblerLastFmConnection::ECustom;
			break;
		default:
			station = CMobblerLastFmConnection::EPersonal;
			break;
		}
	
	iRadioPlayer->StartL(station, aRadioOption);
	}

TBool CMobblerAppUi::RadioStartableL() const
	{
	TRACER_AUTO;
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
	TRACER_AUTO;
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
//	TRACER_AUTO;
	return iLastFmConnection->Mode();
	}

CMobblerLastFmConnection::TState CMobblerAppUi::State() const
	{
//	TRACER_AUTO;
	return iLastFmConnection->State();
	}

TBool CMobblerAppUi::ScrobblingOn() const
	{
//	TRACER_AUTO;
	return iLastFmConnection->ScrobblingOn();
	}

void CMobblerAppUi::HandleStatusPaneSizeChange()
	{
//	TRACER_AUTO;
	}

void CMobblerAppUi::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
	TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if ((aObserver == iAutoCheckForUpdatesObserver) ||
			(aObserver == iManualCheckForUpdatesObserver))
			{
			// we have just sucessfully checked for updates
			// so don't do it again for another week
			TTime now;
			now.UniversalTime();
			now += TTimeIntervalHours(KUpdateIntervalHours);
			iSettingView->Settings().SetNextUpdateCheck(now);
			
			TVersion version;
			TBuf8<KMaxMobblerTextSize> location;
			TInt error(CMobblerParser::ParseUpdateResponseL(aData, version, location));
			
			if (error == KErrNone)
				{
				if ((version.iMajor == KVersion.iMajor && 
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
						if (!iMobblerDownload)
							{
							iMobblerDownload = CMobblerDownload::NewL(*this);
							}
						iMobblerDownload->DownloadL(location, iLastFmConnection->IapId());
						}
					}
				else if (aObserver == iManualCheckForUpdatesObserver)
					{
					// Only show this for manual updates
					CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NO_UPDATE));
					}
				}
			}
		else if (aObserver == iLyricsObserver)
			{
			ShowLyricsL(aData);
			}
		else if (aObserver == iArtistBiographyObserver)
			{
			ShowBiographyL(aData);
			}
#ifdef __SYMBIAN_SIGNED__
		else if (aObserver == iLocalEventsObserver)
			{
			// create a map and open it

			CCoeEnv::Static()->FsSession().MkDirAll(KMapKmlFilename);
			
			_LIT8(KMapKmlStartFormat,		"<kml xmlns=\"http://earth.google.com/kml/2.0\">\r\n"
											"<Document>\r\n");
			
			_LIT8(KMapKmlPlacemarkFormat,	"\t<Placemark>\r\n"
											"\t\t<name>%S</name>\r\n"
											"\t\t<description>\r\n"
											"\t\t\t<![CDATA[\r\n"
											"%S"
											"\t\t\t]]>\r\n"
											"</description>\r\n" 
											"\t\t<Point>\r\n"
											"\t\t\t<coordinates>%S,%S</coordinates>\r\n"
											"\t\t</Point>\r\n"
											"\t</Placemark>\r\n");
			
			_LIT8(KMapKmlEndFormat,			"</Document>\r\n"
											"</kml>\r\n");
			
			_LIT8(KGeoNamespaceUri, 		"http://www.w3.org/2003/01/geo/wgs84_pos#");
			
//			_LIT8(KVenue, 				"venue");
			_LIT8(KLat, 				"lat");
			_LIT8(KLong, 				"long");
			_LIT8(KPoint,				"point");
//			_LIT8(KTitle,				"title");
			_LIT8(KLocation,			"location"); // move to literals

			RFileWriteStream file;
			CleanupClosePushL(file);
			file.Replace(CCoeEnv::Static()->FsSession(), KMapKmlFilename, EFileWrite);
			
			file.WriteL(KMapKmlStartFormat);
			
			// Parse the XML
			CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
			CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
			
			RPointerArray<CSenElement>& events(domFragment->AsElement().Element(KEvents)->ElementsL());
			
			const TInt KEventCount(events.Count());
			for (TInt i(0); i < KEventCount; ++i)
				{
				HBufC8* title(SenXmlUtils::DecodeHttpCharactersLC(events[i]->Element(KTitle)->Content()));
				HBufC8* description(SenXmlUtils::DecodeHttpCharactersLC(events[i]->Element(KDescription)->Content()));
				
				// Get the location of the event
				CSenElement* geoPoint(events[i]->Element(KVenue)->Element(KLocation)->Element(KGeoNamespaceUri, KPoint));
				
				TPtrC8 latitude(KNullDesC8);
				TPtrC8 longitude(KNullDesC8);
				
				if (geoPoint)
					{
					latitude.Set(geoPoint->Element(KGeoNamespaceUri, KLat)->Content());
					longitude.Set(geoPoint->Element(KGeoNamespaceUri, KLong)->Content());
					}
				
				// Add this event to the KML file
				HBufC8* placemark(HBufC8::NewLC(KMapKmlPlacemarkFormat().Length() + title->Length() + description->Length() + longitude.Length() + latitude.Length()));
				TPtrC8 titlePtr(title->Des());
				TPtrC8 descriptionPtr(description->Des());
				placemark->Des().Format(KMapKmlPlacemarkFormat, &titlePtr, &descriptionPtr, &longitude, &latitude);
				file.WriteL(*placemark);
				CleanupStack::PopAndDestroy(placemark);
				CleanupStack::PopAndDestroy(2, title);
				}
			
			CleanupStack::PopAndDestroy(2);	
			
			file.WriteL(KMapKmlEndFormat);
			CleanupStack::PopAndDestroy(&file);
			
			LaunchFileL(KMapKmlFilename);
			}
#endif // __SYMBIAN_SIGNED__
		else if (aObserver == iTwitterAuthObserver)
			{
			DUMPDATA(aData, _L("twitterauth.xml"));
			HBufC8* error(CMobblerParser::ParseTwitterAuthL(aData));
			
			if (!error)
				{
				CAknQueryDialog* dlg(CAknQueryDialog::NewL());
				TBool followMobbler(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_TWITTER_FOLLOW)));
				
				if (followMobbler)
					{
					delete iTwitterFollowObserver;
					iTwitterFollowObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, ETrue);
					iLastFmConnection->QueryTwitterL(CMobblerLastFmConnection::EFollowMobbler, *iTwitterFollowObserver);
					}
				else
					{
					CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_DONE));
					}
				}
			else
				{
				CleanupStack::PushL(error);
				CMobblerString* message(CMobblerString::NewL(*error));
				CleanupStack::PopAndDestroy(error);
				CleanupStack::PushL(message);
				// Tell the user that there was an error connecting
				CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
				note->ExecuteLD(message->String());
				CleanupStack::PopAndDestroy(message);
				}
			}
		} // 	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
	}

void CMobblerAppUi::HandleConnectCompleteL(TInt aError)
	{
	TRACER_AUTO;
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
		if (iSettingView->Settings().CheckForUpdates())
			{
			TTime now;
			now.UniversalTime();
			
			if (now > iSettingView->Settings().NextUpdateCheck())
				{
				// do an update check
				delete iAutoCheckForUpdatesObserver;
				iAutoCheckForUpdatesObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, EFalse);
				iLastFmConnection->CheckForUpdateL(*iAutoCheckForUpdatesObserver);
				}
			}
		
		// See if there's better album art online
		if (CurrentTrack())
			{
			CurrentTrack()->FindBetterImageL();
			}
		}
	}

void CMobblerAppUi::HandleLastFmErrorL(CMobblerLastFmError& aError)
	{
	TRACER_AUTO;
	// iStatusView->DrawDeferred();
	
	CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
	note->ExecuteLD(aError.Text());
	}

void CMobblerAppUi::HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus)
	{
	TRACER_AUTO;
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
	TRACER_AUTO;
	return iTracksSubmitted;
	}

TInt CMobblerAppUi::Queued() const
	{
	TRACER_AUTO;
	return iTracksQueued;
	}

void CMobblerAppUi::HandleTrackNowPlayingL(const CMobblerTrackBase& /*aTrack*/)
	{
	TRACER_AUTO;
	// Tell the status view that the track has changed
//	iStatusView->DrawDeferred();

#ifdef __SYMBIAN_SIGNED__
	iWallpaperSet = EFalse;
	SetAlbumArtAsWallpaper(ETrue);
#endif
	}

void CMobblerAppUi::HandleTrackSubmitted(const CMobblerTrackBase& /*aTrack*/)
	{
	TRACER_AUTO;
	iStatusView->DrawDeferred();
	++iTracksSubmitted;
	--iTracksQueued;
	}

void CMobblerAppUi::HandleTrackQueuedL(const CMobblerTrackBase& /*aTrack*/)
	{
	TRACER_AUTO;
/*	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}*/
	// update the track queued count and change the status bar text
	++iTracksQueued;
	}

void CMobblerAppUi::HandleTrackDequeued(const CMobblerTrackBase& /*aTrack*/)
	{
	TRACER_AUTO;
//	iStatusView->DrawDeferred();
	--iTracksQueued;
	}

TBool CMobblerAppUi::GoOnlineL()
	{
	TRACER_AUTO;
	// Ask if they would like to go online
	CAknQueryDialog* dlg(CAknQueryDialog::NewL());
	TBool goOnline(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_ASK_GO_ONLINE)));
	
	if (goOnline)
		{
		iSettingView->Settings().SetMode(CMobblerLastFmConnection::EOnline);
		}
	
	return goOnline;
	}

void CMobblerAppUi::StatusDrawDeferred()
	{
//	TRACER_AUTO;
	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}
	}

void CMobblerAppUi::StatusDrawNow()
	{
//	TRACER_AUTO;
	if (iStatusView)
		{
		iStatusView->DrawNow();
		}
	}

void CMobblerAppUi::HandleForegroundEventL(TBool aForeground)
	{
//	TRACER_AUTO;
	CAknAppUi::HandleForegroundEventL(aForeground);
	iForeground = aForeground;
	}

TBool CMobblerAppUi::Foreground() const
	{
//	TRACER_AUTO;
	return iForeground;
	}

TBool CMobblerAppUi::Backlight() const
	{
//	TRACER_AUTO;
	return iSettingView->Settings().Backlight();
	}

TInt CMobblerAppUi::ScrobblePercent() const
	{
//	TRACER_AUTO;
	return iSettingView->Settings().ScrobblePercent();
	}

TInt CMobblerAppUi::DownloadAlbumArt() const
	{
//	TRACER_AUTO;
	return iSettingView->Settings().DownloadAlbumArt();
	}

void CMobblerAppUi::TrackStoppedL()
	{
	TRACER_AUTO;
	iSettingView->Settings().SetVolume(RadioPlayer().Volume());
	
	if (iSleepAfterTrackStopped)
		{
		iSleepAfterTrackStopped = EFalse;
		SleepL();
		}
	}

void CMobblerAppUi::LoadRadioStationsL()
	{
	TRACER_AUTO;
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KRadioFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		iPreviousRadioStation = readStream.ReadInt32L();
		if (iPreviousRadioStation <= EMobblerCommandRadioEnumFirst ||
			iPreviousRadioStation >= EMobblerCommandRadioEnumLast ||
			iPreviousRadioStation == EMobblerCommandRadioLoved ||  // discontinued station
			iPreviousRadioStation == EMobblerCommandRadioPlaylist) // discontinued station
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
			// Used to be iPreviousRadioPlaylistId, but playlist radio 
			// has been discontinued by Last.fm so just ignore it.
			}
		loadIt = EFalse;
		TRAP_IGNORE(loadIt = readStream.ReadInt8L());
		if (loadIt)
			{
			readStream >> radio;
			delete iPreviousRadioGroup;
			iPreviousRadioGroup = CMobblerString::NewL(radio);
			}
		loadIt = EFalse;
		TRAP_IGNORE(loadIt = readStream.ReadInt8L());
		if (loadIt)
			{
			readStream >> radio;
			delete iPreviousRadioCustom;
			iPreviousRadioCustom = CMobblerString::NewL(radio);
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
	TRACER_AUTO;
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
		
		// Used to be iPreviousRadioPlaylistId, but playlist radio 
		// has been discontinued by Last.fm so just leave a gap
		// to maintain file format compatibility.
		writeStream.WriteInt8L(EFalse);
		
		if (iPreviousRadioGroup)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioGroup->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioCustom)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioCustom->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerAppUi::LoadSearchTermsL()
	{
	TRACER_AUTO;
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KSearchFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TBuf<KMaxMobblerTextSize> search;
		if (readStream.ReadInt8L())
			{
			readStream >> search;
			delete iPreviousSearchTrack;
			iPreviousSearchTrack = CMobblerString::NewL(search);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> search;
			delete iPreviousSearchAlbum;
			iPreviousSearchAlbum = CMobblerString::NewL(search);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> search;
			delete iPreviousSearchArtist;
			iPreviousSearchArtist = CMobblerString::NewL(search);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> search;
			delete iPreviousSearchTag;
			iPreviousSearchTag = CMobblerString::NewL(search);
			}
		
		CleanupStack::PopAndDestroy(&readStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerAppUi::SaveSearchTermsL()
	{
	TRACER_AUTO;
	CCoeEnv::Static()->FsSession().MkDirAll(KSearchFile);
	
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KSearchFile, EFileWrite));
	
	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		if (iPreviousSearchTrack)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousSearchTrack->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}
		
		if (iPreviousSearchAlbum)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousSearchAlbum->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}
		
		if (iPreviousSearchArtist)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousSearchArtist->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}
		
		if (iPreviousSearchTag)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousSearchTag->String();
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
//	TRACER_AUTO;
	return *iResourceReader;
	}

CMobblerBitmapCollection& CMobblerAppUi::BitmapCollection() const
	{
//	TRACER_AUTO;
	return *iBitmapCollection;
	}

void CMobblerAppUi::SetSleepTimerL(const TInt aMinutes)
	{
	TRACER_AUTO;
	LOG(_L8("CMobblerAppUi::SetSleepTimerL"));
	LOG(aMinutes);
	
	TInt sleepMinutes(aMinutes);
	if (!iSleepTimer)
		{
		iSleepTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
		}
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

	iSettingView->Settings().SetSleepTimerMinutes(sleepMinutes);
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
	TRACER_AUTO;
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
	
	iSettingView->Settings().SetAlarmTime(alarmTime);
	if (!iAlarmTimer)
		{
		iAlarmTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
		}
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
	TRACER_AUTO;
#ifdef _DEBUG
	CEikonEnv::Static()->InfoMsg(_L("Timer expired!"));
	LOG(_L8("CMobblerAppUi::TimerExpiredL"));
	LOG(aError);
#endif
	if (aTimer == iSleepTimer && aError == KErrNone)
		{
		LOG(iSettingView->Settings().SleepTimerImmediacy());
		
		if (CurrentTrack() && 
			iSettingView->Settings().SleepTimerImmediacy() == CMobblerSettingItemListSettings::EEndOfTrack)
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
		iSettingView->Settings().SetAlarmOn(EFalse);
		User::ResetInactivityTime();

		if (iLastFmConnection->IapId() != (TUint)iSettingView->Settings().AlarmIapId())
			{
			HandleCommandL(EMobblerCommandOffline);
			SetIapIDL(iSettingView->Settings().AlarmIapId());
			}
		
		HandleCommandL(EMobblerCommandOnline);
		
		TApaTask task(iEikonEnv->WsSession());
		task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
		task.BringToForeground();
		
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_ALARM_EXPIRED));
		
		iRadioPlayer->SetVolume(iSettingView->Settings().AlarmVolume());

		TInt station(iSettingView->Settings().AlarmStation() + EMobblerCommandRadioArtist);

		switch (station)
			{
			case EMobblerCommandRadioArtist:			// intentional fall-through
			case EMobblerCommandRadioTag:				// intentional fall-through
			case EMobblerCommandRadioUser:				// intentional fall-through
			case EMobblerCommandRadioGroup:				// intentional fall-through
			case EMobblerCommandRadioCustom:			// intentional fall-through
				{
				CMobblerString* option(CMobblerString::NewLC(iSettingView->Settings().AlarmOption()));
				RadioStartL(station, option);
				CleanupStack::PopAndDestroy(option);
				}
				break;
			case EMobblerCommandRadioRecommendations:	// intentional fall-through
			case EMobblerCommandRadioPersonal:			// intentional fall-through
			case EMobblerCommandRadioMix:				// intentional fall-through
			case EMobblerCommandRadioFriends:			// intentional fall-through
			case EMobblerCommandRadioNeighbourhood:		// intentional fall-through
			default:
				RadioStartL(station, NULL);
				break;
			}
		}
	else if (aTimer == iAlarmTimer && aError == KErrAbort)
		{
		iAlarmTimer->At(iSettingView->Settings().AlarmTime());
		}
	else if (aTimer == iAlarmTimer && aError == KErrUnderflow)
		{
		iSettingView->Settings().SetAlarmOn(EFalse);
		}
	}

void CMobblerAppUi::SleepL()
	{
	TRACER_AUTO;
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
	
	switch (iSettingView->Settings().SleepTimerAction())
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
	TRACER_AUTO;
	if (iSleepTimer && iSleepTimer->IsActive())
		{
		iSleepTimer->Cancel();
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_REMOVED));
		}
	}

void CMobblerAppUi::RemoveAlarmL()
	{
	TRACER_AUTO;
	if (iAlarmTimer && iAlarmTimer->IsActive())
		{
		iAlarmTimer->Cancel();
		iSettingView->Settings().SetAlarmOn(EFalse);
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_ALARM_REMOVED));
		}
	}

// Gesture plug-in functions
void CMobblerAppUi::LoadGesturesPluginL()
	{
	TRACER_AUTO;
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

	TRAPD(error, mobblerGestures = static_cast<CMobblerGesturesInterface*>(REComSession::CreateImplementationL(KMobblerGesturePlugin5xUid, dtorIdKey)));
	
	if (error == KErrNone)
		{
		iGesturePlugin = mobblerGestures;
		iGesturePluginDtorUid = dtorIdKey;
		}
	else
		{
		REComSession::DestroyedImplementation(dtorIdKey);
		
		// We didn't load the preferred plug-in, try all other plug-ins

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
	TRACER_AUTO;
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

TInt CMobblerAppUi::LaunchFileL(const TDesC& aFilename)
	{
	TRACER_AUTO;
	if (!iDocHandler)
		{
		iDocHandler = CDocumentHandler::NewL(CEikonEnv::Static()->Process());
		}
	
	TDataType emptyDataType = TDataType();
	TInt error(iDocHandler->OpenFileEmbeddedL(aFilename, emptyDataType));
	return error;
	}

void CMobblerAppUi::GoToLastFmL(TInt aCommand, const TDesC8& aEventId)
	{
	TRACER_AUTO;
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
	TRACER_AUTO;
	CCoeEnv::Static()->FsSession().MkDirAll(KMapKmlFilename);
	
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
	
	TInt error(LaunchFileL(KMapKmlFilename));
	
	if (error != KErrNone)
		{
		_LIT(KMapURLFormat, "http://maptwits.com/displaymap.php?lat=%S&long=%S");
		
		CMobblerString* longitude(CMobblerString::NewLC(aLongitude));
		CMobblerString* latitude(CMobblerString::NewLC(aLatitude));
		//CMobblerString* name(CMobblerString::NewLC(aName)); // TODO: should name be used?
		
		HBufC* url(HBufC::NewLC(KMapURLFormat().Length() + longitude->String().Length() + latitude->String().Length()));
		
		url->Des().Format(KMapURLFormat, &latitude->String(), &longitude->String());
		
		OpenWebBrowserL(*url);
		
		CleanupStack::PopAndDestroy(4);
		}
	}

#ifdef __SYMBIAN_SIGNED__
void CMobblerAppUi::HandleLocationCompleteL(const TDesC8& /*aAccuracy*/, const TDesC8& aLatitude, const TDesC8& aLongitude, const TDesC8& /*aName*/)
	{
	TRACER_AUTO;
	delete iLocalEventsObserver;
	iLocalEventsObserver = CMobblerFlatDataObserverHelper::NewL(*iLastFmConnection, *this, ETrue);
	iLastFmConnection->GeoGetEventsL(aLatitude, aLongitude, *iLocalEventsObserver);
	}
#endif

void CMobblerAppUi::OpenWebBrowserL(const TDesC& aUrl)
	{
	TRACER_AUTO;
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
	if (!iBrowserLauncher)
		{
		iBrowserLauncher = CBrowserLauncher::NewL();
		}
	iBrowserLauncher->LaunchBrowserEmbeddedL(url);
#endif
#endif // __SYMBIAN_SIGNED__
	
	CleanupStack::PopAndDestroy(encode);
	}

void CMobblerAppUi::HandleSystemEventL(const TWsEvent& aEvent)
	{
	TRACER_AUTO;
	switch (*(TApaSystemEvent*)(aEvent.EventData()))
		{
		case EApaSystemEventShutdown:
			{
			if (!iSystemCloseGlobalQuery)
				{
				iSystemCloseGlobalQuery = CMobblerGlobalQuery::NewL(R_MOBBLER_CLOSE_QUERY);
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
	TRACER_AUTO;
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
	TRACER_AUTO;
	TInt error(KErrUnknown);
	_LIT(KWallpaperFile, "C:\\System\\Data\\Mobbler\\wallpaperimage.mbm");
	
	if (!aAutomatically || iSettingView->Settings().AutomaticWallpaper())
		{
		LOG(_L8("Set as wallpaper"));
		if (!iWallpaperSet &&
			CurrentTrack() && 
			CurrentTrack()->Image() && 
			CurrentTrack()->Image()->Bitmap())
			{
			// The current track has album art and it has finished loading
			CCoeEnv::Static()->FsSession().MkDirAll(KWallpaperFile);
			error = CurrentTrack()->Image()->Bitmap(ETrue)->Save(KWallpaperFile);
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
	TRACER_AUTO;
	_LIT(KPassword, "password");
	if ((iSettingView->Settings().Username().Compare(iResourceReader->ResourceL(R_MOBBLER_USERNAME)) == 0) &&
		(iSettingView->Settings().Password().Compare(KPassword) == 0))
		{
		return ETrue;
		}
	return EFalse;
	}

void CMobblerAppUi::ShowLyricsL(const TDesC8& aData)
	{
	TRACER_AUTO;
	DUMPDATA(aData, _L("lyrics.xml"));
	_LIT8(KSg, "sg"); // song
	_LIT8(KAr, "ar"); // artist name
	_LIT8(KTt, "tt"); // title of the song
	_LIT8(KTx, "tx"); // lyrics text
	_LIT8(K200, "200");
	_LIT8(K300, "300");
	
	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
	
	// Get the status error code
	TPtrC8 statusPtrC(domFragment->AsElement().Element(KStatus)->Content());
	
	if ((statusPtrC.CompareF(K200) == 0) || 
		(statusPtrC.CompareF(K300) == 0))
		{
		TPtrC8 artistPtrC(domFragment->AsElement().Element(KSg)->Element(KAr)->Content());
		TPtrC8  titlePtrC(domFragment->AsElement().Element(KSg)->Element(KTt)->Content());
		TPtrC8 lyricsPtrC(domFragment->AsElement().Element(KSg)->Element(KTx)->Content());
		
		HBufC8* artistBuf(HBufC8::NewLC(artistPtrC.Length()));
		HBufC8*  titleBuf(HBufC8::NewLC( titlePtrC.Length()));
		HBufC8* lyricsBuf(HBufC8::NewLC(lyricsPtrC.Length()));
		SenXmlUtils::DecodeHttpCharactersL(artistPtrC, artistBuf);
		SenXmlUtils::DecodeHttpCharactersL( titlePtrC,  titleBuf);
		SenXmlUtils::DecodeHttpCharactersL(lyricsPtrC, lyricsBuf);
		
		TPtr8 lyricsPtr(lyricsBuf->Des());
		MobblerUtility::FixLyricsLineBreaks(lyricsPtr);
		
/*#ifdef PERMANENT_LYRICSFLY_ID_KEY
		// Only link back to corrections with the permanent ID key.
		// Temporary keys don't return correct checksums to prevent abuse.
		_LIT8(KCs, "cs"); // checksum (for link back)
		_LIT8(KId, "id"); // song ID (for link back)
		_LIT8(KLinkBackFormat, "<a href=\"http://lyricsfly.com/search/correction.php?%S&id=%S\">Make corrections</a>\r\n");
		
		TPtrC8 checkSumPtrC(domFragment->AsElement().Element(KSg)->Element(KCs)->Content());
		TPtrC8 idPtrC(domFragment->AsElement().Element(KSg)->Element(KId)->Content());
		
		HBufC8* linkBackBuf(HBufC8::NewLC(KLinkBackFormat().Length() + 
										  checkSumPtrC.Length() + 
										  idPtrC.Length()));
		
		linkBackBuf->Des().Format(KLinkBackFormat, &checkSumPtrC, &idPtrC);
		LOG(*linkBackBuf);
		
		file.WriteL(*linkBackBuf);
#endif*/

		// Show lyrics in the browser view thingy
		HBufC8* lyricsHtml(HBufC8::NewLC(
				KHtmlHeaderTemplate().Length() +
				KLyricsHtmlTemplate().Length() +
				artistBuf->Length() + 
				titleBuf->Length() + 
				lyricsBuf->Length()
/*#ifdef PERMANENT_LYRICSFLY_ID_KEY
				+ KLinkBackFormat().Length()
				+ linkBackBuf->Length()
#endif*/
				));

		TPtr8 lyricsHtmlPtr(lyricsHtml->Des());
		lyricsHtmlPtr.Append(KHtmlHeaderTemplate);
		lyricsHtmlPtr.AppendFormat(
				KLyricsHtmlTemplate,
				artistBuf, 
				titleBuf,
				lyricsBuf);

/*#ifdef PERMANENT_LYRICSFLY_ID_KEY
		lyricsHtmlPtr.Append(*linkBackBuf);
#endif*/

		DUMPDATA(lyricsHtmlPtr, _L("lyrics.txt"));

		ActivateLocalViewL(iBrowserView->Id(), TUid::Uid(EMobblerCommandTrackLyrics), lyricsHtmlPtr);

		CleanupStack::PopAndDestroy(lyricsHtml);
/*#ifdef PERMANENT_LYRICSFLY_ID_KEY
		CleanupStack::PopAndDestroy(linkBackBuf);
#endif*/
		CleanupStack::PopAndDestroy(3); // lyricsBuf, titleBuf, artistBuf
		}
	else
		{
		CAknResourceNoteDialog *note(new (ELeave) CAknInformationNote(EFalse));
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_LYRICS_NOT_FOUND));
		
//		TPtrC8 songPtrC(domFragment->AsElement().Element(KSg)->Content());
//		file.WriteL(songPtrC);
		}
	
#ifdef _DEBUG
	_LIT8(K204, "204");
	_LIT8(K400, "400");
	_LIT8(K401, "401");
	_LIT8(K402, "402");
	_LIT8(K406, "406");
	if (statusPtrC.CompareF(K200) == 0)
		{
		LOG(_L8("200 - ok"));
		//LOG(_L8("    Results are returned. All parameters checked ok."));
		}
	else if (statusPtrC.CompareF(K204) == 0)
		{
		LOG(_L8("204 - NO CONTENT"));
		//LOG(_L8("   Parameter query returned no results. All parameters checked ok."));
		}
	else if (statusPtrC.CompareF(K300) == 0)
		{
		LOG(_L8("300 - TESTING LIMITED"));
		//LOG(_L8("    Temporary access. Limited content. All parameters checked ok."));
		}
	else if (statusPtrC.CompareF(K400) == 0)
		{
		LOG(_L8("400 - MISSING KEY"));
		//LOG(_L8("   Parameter “i” missing. Authorization failed."));
		}
	else if (statusPtrC.CompareF(K401) == 0)
		{
		LOG(_L8("401 – UNAUTHORIZED"));
		//LOG(_L8("   Parameter “i” invalid. Authorization failed."));
		}
	else if (statusPtrC.CompareF(K402) == 0)
		{
		LOG(_L8("402 - LIMITED TIME"));
		//LOG(_L8("    This response is returned only if you query too soon. Limit query requests. Time of delay is shown in <delay> tag in milliseconds."));
		}
	else if (statusPtrC.CompareF(K406) == 0)
		{
		LOG(_L8("406 - QUERY TOO SHORT"));
		//LOG(_L8("    Query request string is too short. All other parameters checked ok."));
		}
#endif // _DEBUG

	CleanupStack::PopAndDestroy(2); // xmlReader & domFragment
	}

void CMobblerAppUi::ShowBiographyL(const TDesC8& aData)
    {
	TRACER_AUTO;
	DUMPDATA(aData, _L("bio.xml"));
	HBufC8* artist(NULL);
	HBufC8* artistInfo(NULL);
	HBufC8* imageUrl(NULL);
	HBufC8* tagsText(NULL);
	HBufC8* similarArtistsText(NULL);
	CMobblerParser::ParseArtistInfoL(aData, artist, artistInfo, imageUrl, tagsText, similarArtistsText);
	if (iGettingLocalisedBiography && artistInfo->Length() == 0)
	    {
	    // No artist biography in the phone's current language so make the request again
	    // in non-localised form
	    delete artist;
	    delete artistInfo;
	    delete imageUrl;
	    delete tagsText;
	    delete similarArtistsText;
	    GetBiographyL(EFalse);
	    return;
	    }
	
	CleanupStack::PushL(artist);
	CleanupStack::PushL(artistInfo);
	CleanupStack::PushL(imageUrl);
	CleanupStack::PushL(tagsText);
	CleanupStack::PushL(similarArtistsText);

	// Decide how big the artist picture should be taking into account the width
	// of the application
	TInt artistImageWidth((TInt)((TReal)ApplicationRect().Width() * 0.40));

	HBufC8* artistInfoHtml(HBufC8::NewL(
			KHtmlHeaderTemplate().Length() +
			KBiographyHtmlTemplate().Length() +
			artist->Length() +
			tagsText->Length() +
			similarArtistsText->Length() +
			imageUrl->Length() +
			3 +
			artistInfo->Length()));

	TPtr8 artistHtmlPtr(artistInfoHtml->Des());
	artistHtmlPtr.Append(KHtmlHeaderTemplate);
	artistHtmlPtr.AppendFormat(
			KBiographyHtmlTemplate,
			artist,
			imageUrl,
			artistImageWidth,
			tagsText,
			similarArtistsText,
			artistInfo);
	DUMPDATA(artistHtmlPtr, _L("artistbio.txt"));

	CleanupStack::PopAndDestroy(5, artist);
	CleanupStack::PushL(artistInfoHtml);
	ActivateLocalViewL(iBrowserView->Id(), TUid::Uid(EMobblerCommandBiography), artistHtmlPtr);
	CleanupStack::PopAndDestroy(artistInfoHtml);
	}

void CMobblerAppUi::WarnOldScrobblesL()
	{
	TRACER_AUTO;
	if (!iOldScrobbleGlobalQuery)
		{
		iOldScrobbleGlobalQuery = CMobblerGlobalQuery::NewL(R_MOBBLER_OLD_SCROBBLES_WARNING);
		CActiveScheduler::Start();
		
		delete iOldScrobbleGlobalQuery;
		iOldScrobbleGlobalQuery = NULL;
		}
	}

void CMobblerAppUi::GetBiographyL(TBool aLocalised)
    {
    TRACER_AUTO;
    iGettingLocalisedBiography = aLocalised;
    iLastFmConnection->WebServicesCallL(KArtist, KGetInfo, CurrentTrack()->Artist().String8(), 
            *iArtistBiographyObserver, KErrNotFound, KErrNotFound, aLocalised);
    }

// End of file
