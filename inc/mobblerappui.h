/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade
Copyright (C) 2009  James Aley
Copyright (C) 2009, 2010  gw111zz

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

#ifndef __MOBBLERAPPUI_H__
#define __MOBBLERAPPUI_H__

#include <aknviewappui.h>
#include <remconcoreapitargetobserver.h>    // link against RemConCoreApi.lib
#include <remconcoreapitarget.h>            // and
#include <remconinterfaceselector.h>        // RemConInterfaceBase.lib

#include "mobblerdataobserver.h"
#include "mobblerdownload.h"
#include "mobblergesturesinterface.h"
#include "mobblerlastfmconnectionobserver.h"
#ifdef __SYMBIAN_SIGNED__
#include "mobblerlocationobserver.h"
#endif
#include "mobblersleeptimer.h"

#ifdef BETA_BUILD
const TInt KUpdateIntervalHours(1);
#else
const TInt KUpdateIntervalHours(24);
#endif
const TInt KMaxMobblerTextSize(255);

const TInt KMobblerMinorVersion(10);
const TInt KMobblerBuildNumber(80);

#ifdef __SYMBIAN_SIGNED__
const TVersion KVersion(1, KMobblerMinorVersion, KMobblerBuildNumber);
const TInt KMobblerAppUid = 0x2002655A;
const TInt KMobblerSettingsViewUid = 0x2002655C;
const TInt KMobblerStatusViewUid = 0x2002655B;
const TInt KMobblerWebServicesViewUid = 0x2002656B;
const TInt KMobblerBrowserViewUid = 0x2002656C;
#else
const TVersion KVersion(0, KMobblerMinorVersion, KMobblerBuildNumber);
const TInt KMobblerAppUid = 0xA0007648;
const TInt KMobblerSettingsViewUid = 0xA0007CA9;
const TInt KMobblerStatusViewUid = 0xA0007CA8;
const TInt KMobblerWebServicesViewUid = 0xA000B6C3;
const TInt KMobblerBrowserViewUid = 0xA000B6D4;
#ifndef __WINS__
class CBrowserLauncher;
#endif
#endif

class CAknGlobalConfirmationQuery;
class CDocumentHandler;
class CMobblerBitmapCollection;
class CMobblerBrowserView;
class CMobblerDestinationsInterface;
class CMobblerDownload;
#ifdef __SYMBIAN_SIGNED__
class CMobblerLocation;
#endif
class CMobblerMusicAppListener;
class CMobblerRadioPlayer;
class CMobblerResourceReader;
class CMobblerSettingItemListView;
class CMobblerStatusView;
class CMobblerString;
class CMobblerTrack;
class CMobblerWebServicesView;
class CMobblerWebServicesHelper;
class CMobblerContentListingInterface;

class CMobblerGlobalQuery : public CActive
	{
public:
	static CMobblerGlobalQuery* NewL(TInt aResourceId);
	~CMobblerGlobalQuery();

private:
	CMobblerGlobalQuery();
	void ConstructL(TInt aResourceId);

private: // from CActive
	void RunL();
	void DoCancel();

private:
	CAknGlobalConfirmationQuery* iGlobalConfirmationQuery;
	HBufC* iMessage;
	};

class CMobblerAppUi : public CAknViewAppUi,
						public MMobblerLastFmConnectionObserver,
						public MMobblerDownloadObserver,
						public MMobblerSleepTimerNotify,
						public MRemConCoreApiTargetObserver,
						public MMobblerFlatDataObserverHelper,
						public MMobblerGestures
#ifdef __SYMBIAN_SIGNED__
						, public MMobblerLocationObserver
#endif
	{
public:
	enum TDownloadAlbumArt
		{
		ENever,
		EOnlyRadio,
		EAlwaysAndKeep,
		EAlwaysAndDitch
		};


public:
	void ConstructL();
	CMobblerAppUi();
	~CMobblerAppUi();

	const CMobblerTrack* CurrentTrack() const;
	CMobblerTrack* CurrentTrack();

	CMobblerRadioPlayer& RadioPlayer() const;
	CMobblerLastFmConnection& LastFmConnection() const;
	CMobblerMusicAppListener& MusicListener() const;
	CMobblerBitmapCollection& BitmapCollection() const;
	CMobblerDestinationsInterface* Destinations() const;
	CMobblerContentListingInterface* ContentListing() const;

	CMobblerSettingItemListView& SettingView() const;
	CMobblerBrowserView& BrowserView() const;
	HBufC* MusicAppNameL() const;

	void RadioStartL(TInt aRadioStation,
					 const CMobblerString* aRadioOption,
					 TBool aSaveStations = ETrue);

	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword, 
					 TBool aAndSaveToSettings = EFalse);
	void SetIapIDL(TUint32 aIapID);
	void SetBufferSize(TTimeIntervalSeconds aBufferSize);
	void UpdateAccelerometerGesturesL();
	TBool AccelerometerGesturesAvailable() const;
	void SetSleepTimerL(const TInt aMinutes);
	void SetAlarmTimerL(const TTime aTime);
	void SetBitRateL(TInt aBitRate);

	TInt Scrobbled() const;
	TInt Queued() const;

	void StatusDrawDeferred();
	void StatusDrawNow();

	CMobblerLastFmConnection::TMode Mode() const;
	CMobblerLastFmConnection::TState State() const;
	TBool ScrobblingOn() const;

	TBool RadioResumable() const;
	TBool Foreground() const;
	TBool Backlight() const;
	TInt ScrobblePercent() const;
	TInt DownloadAlbumArt() const;
	void TrackStoppedL();

	CMobblerResourceReader& CMobblerAppUi::ResourceReader() const;

	TBool SleepTimerActive() const { return iSleepTimer ? iSleepTimer->IsActive() : EFalse; }
	TBool AlarmActive()      const { return iAlarmTimer ? iAlarmTimer->IsActive() : EFalse; }
	void RemoveSleepTimerL();
	void RemoveAlarmL();
	TBool SleepAfterTrackStopped() { return iSleepAfterTrackStopped; }

#ifdef __SYMBIAN_SIGNED__
	TInt SetAlbumArtAsWallpaper(TBool aAutomatically = EFalse);
#endif
	void GoToLastFmL(TInt aCommand, const TDesC8& aEventId = KNullDesC8);
	void OpenWebBrowserL(const TDesC& aUrl);
	void GoToMapL(const TDesC8& aName, const TDesC8& aLatitude, const TDesC8& aLongitude);
	TInt LaunchFileL(const TDesC& aFilename);
	TBool DetailsNeeded();
	void ShowLyricsL(const TDesC8& aData);
	void ShowBiographyL(const TDesC8& aData);
	void WarnOldScrobblesL();

public: // CEikAppUi
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);

private:
	void HandleInstallStartedL();

private:
	void HandleStatusPaneSizeChange();

	void HandleConnectCompleteL(TInt aError);
	void HandleLastFmErrorL(CMobblerLastFmError& aError);
	void HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus);
	void HandleTrackSubmitted(const CMobblerTrackBase& aTrack);
	void HandleTrackQueuedL(const CMobblerTrackBase& aTrack);
	void HandleTrackDequeued(const CMobblerTrackBase& aTrack);
	void HandleTrackNowPlayingL(const CMobblerTrackBase& aTrack);

	TBool GoOnlineL();

	void LoadRadioStationsL();
	void SaveRadioStationsL();
	void LoadSearchTermsL();
	void SaveSearchTermsL();
	void SleepL();
	TBool RadioStartableL() const;
	void GetBiographyL(TBool aLocalised);

private: // from MMobblerSleepTimerNotify
	void TimerExpiredL(TAny* aTimer, TInt aError);

private: // auto-repeat audio button callbacks
	static TInt VolumeUpCallBack(TAny *self);
	static TInt VolumeDownCallBack(TAny *self);

private:
	void LoadGesturesPluginL();

	// Gestures, from MMobblerGestures
	void HandleSingleShakeL(TMobblerShakeGestureDirection aDirection);

	// Observer of media button clicks
	void MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct);

private:
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError);

private: // from MAknWsEventObserver
	void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);
	void HandleSystemEventL(const TWsEvent& aEvent);

#ifdef __SYMBIAN_SIGNED__
private: // from MMobblerLocationObserver
	void HandleLocationCompleteL(const TDesC8& aAccuracy, const TDesC8& aLatitude, const TDesC8& aLongitude, const TDesC8& aName);
#endif
	
private:
	// the view classes
	CMobblerSettingItemListView* iSettingView;
	CMobblerStatusView* iStatusView;
	CMobblerWebServicesView* iWebServicesView;
	CMobblerBrowserView* iBrowserView;


	// The application engine classes
	CMobblerLastFmConnection* iLastFmConnection;
	CMobblerRadioPlayer* iRadioPlayer;
	CMobblerMusicAppListener* iMusicListener;

	// Gesture observer plugin
	TUid iGesturePluginDtorUid;
	CMobblerGesturesInterface* iGesturePlugin;
	
#ifdef __SYMBIAN_SIGNED__
	// Location
	CMobblerLocation* iLocation;
#endif

	// The current track submit and queue count
	TInt iTracksSubmitted;
	TInt iTracksQueued;

	// media buttons
	CRemConInterfaceSelector* iInterfaceSelector;
	CRemConCoreApiTarget*     iCoreTarget;

	// timers and callbacks for media buttons autorepeat
	CPeriodic* iVolumeUpTimer;
	CPeriodic* iVolumeDownTimer;
	TCallBack iVolumeUpCallBack;
	TCallBack iVolumeDownCallBack;
	
	// content listing framework
	CMobblerContentListingInterface* iContentListing;
	TUid iContentListingDtorUid;

	TInt iPreviousRadioStation;

	CMobblerString* iPreviousRadioArtist;
	CMobblerString* iPreviousRadioTag;
	CMobblerString* iPreviousRadioUser;
	CMobblerString* iPreviousRadioGroup;
	CMobblerString* iPreviousRadioCustom;
	CMobblerString* iPreviousSearchTrack;
	CMobblerString* iPreviousSearchAlbum;
	CMobblerString* iPreviousSearchArtist;
	CMobblerString* iPreviousSearchTag;

	CMobblerBitmapCollection* iBitmapCollection;

#if !defined(__SYMBIAN_SIGNED__) && !defined(__WINS__)
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;
	TBool iGettingLocalisedBiography;

	CMobblerDownload* iMobblerDownload;

	CMobblerResourceReader* iResourceReader;

	CMobblerSleepTimer* iSleepTimer;
	TTime iTimeToSleep;
	TBool iSleepAfterTrackStopped;
	CMobblerSleepTimer* iAlarmTimer;

	CMobblerWebServicesHelper* iWebServicesHelper;
	CMobblerFlatDataObserverHelper* iArtistBiographyObserver;
	CMobblerFlatDataObserverHelper* iAutoCheckForUpdatesObserver;
	CMobblerFlatDataObserverHelper* iManualCheckForUpdatesObserver;
	CMobblerFlatDataObserverHelper* iLyricsObserver;
#ifdef __SYMBIAN_SIGNED__
	CMobblerFlatDataObserverHelper* iLocalEventsObserver;
#endif
	CMobblerFlatDataObserverHelper* iTwitterAuthObserver;
	CMobblerFlatDataObserverHelper* iTwitterFollowObserver;

	CMobblerDestinationsInterface* iDestinations;
	TUid iDestinationsDtorUid;

	CMobblerGlobalQuery* iSystemCloseGlobalQuery;
	CMobblerGlobalQuery* iOldScrobbleGlobalQuery;

#ifdef __SYMBIAN_SIGNED__
	TBool iWallpaperSet;
#endif

	CDocumentHandler* iDocHandler;
	};

#endif // __MOBBLERAPPUI_H__

// End of file
