/*
mobblerappui.h

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

#ifndef __MOBBLERAPPUI_H__
#define __MOBBLERAPPUI_H__

#include <aknserverapp.h>	// MAknServerAppExitObserver
#include <aknviewappui.h>
#include <remconcoreapitargetobserver.h>    // link against RemConCoreApi.lib
#include <remconcoreapitarget.h>            // and
#include <remconinterfaceselector.h>        // RemConInterfaceBase.lib

#include "mobblerdataobserver.h"
#include "mobblerdownload.h"
#include "mobblergesturesinterface.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerlocationobserver.h"
#include "mobblersleeptimer.h"

#ifdef BETA_BUILD
const TInt KUpdateIntervalHours(1);
#else
const TInt KUpdateIntervalHours(24);
#endif
const TInt KMaxMobblerTextSize(255);

const TInt KMobblerMinorVersion(9);
const TInt KMobblerBuildNumber(265);

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
class CMobblerLocation;
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

class CMobblerSystemCloseGlobalQuery : public CActive
	{
public:
	static CMobblerSystemCloseGlobalQuery* NewL();
	~CMobblerSystemCloseGlobalQuery();

private:
	CMobblerSystemCloseGlobalQuery();
	void ConstructL();

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
						public MMobblerGestures,
						public MMobblerLocationObserver
	{
public:
	enum TDownloadAlbumArt
		{
		ENever,
		EOnlyRadio,
		EAlwaysWhenOnline
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

	TBool SleepTimerActive() const { return iSleepTimer->IsActive(); }
	TBool AlarmActive() const { return iAlarmTimer->IsActive(); }
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
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);

private: // from MAknWsEventObserver
	void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);
	void HandleSystemEventL(const TWsEvent& aEvent);

private: // from MMobllerLocationObserver
	void HandleLocationCompleteL(const TDesC8& aAccuracy, const TDesC8& aLatitude, const TDesC8& aLongitude, const TDesC8& aName);
	
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
	
	// Location
	CMobblerLocation* iLocation;

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
	CMobblerString* iPreviousRadioPlaylistId;
	CMobblerString* iPreviousSearchTrack;
	CMobblerString* iPreviousSearchAlbum;
	CMobblerString* iPreviousSearchArtist;
	CMobblerString* iPreviousSearchTag;

	CMobblerBitmapCollection* iBitmapCollection;

#if !defined(__SYMBIAN_SIGNED__) && !defined(__WINS__)
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;

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
	CMobblerFlatDataObserverHelper* iLocalEventsObserver;

	CMobblerDestinationsInterface* iDestinations;
	TUid iDestinationsDtorUid;

	CMobblerSystemCloseGlobalQuery* iSystemCloseGlobalQuery;

#ifdef __SYMBIAN_SIGNED__
	TBool iWallpaperSet;
#endif

	CDocumentHandler* iDocHandler;
	};

#endif // __MOBBLERAPPUI_H__

// End of file
