/*
mobblerlastfmconnection.h

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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

#ifndef __MOBBLERLASTFMCONNECTION_H__
#define __MOBBLERLASTFMCONNECTION_H__

#include <e32base.h>
#include <badesca.h>
#include <es_sock.h>
#include <http/rhttpsession.h>
#include <http/mhttptransactioncallback.h> 
#include <http/mhttpdatasupplier.h>
#include <cmmanager.h>

#include "mobblertransactionobserver.h"
#include "mobblerlastfmerror.h"

class CHTTPFormEncoder;
class CMobblerTrack;
class MMobblerRadioPlayer;
class MWebServicesObserver;
class CMobblerParser;
class MMobblerLastFMConnectionObserver;
class CMobblerTrack;


class MMobblerDownloadObserver
	{
public:
	virtual void WriteMp3DataL(const TDesC8& aData, TInt aTotalSize) = 0;
	virtual void SetAbumArtL(const TDesC8& aAlbumArt) = 0;
	virtual void TrackDownloadCompleteL() = 0;
	};

class CMobblerLastFMConnection : public CActive, public MHTTPTransactionCallback, public MMobblerTransactionObserver
	{
public:
	enum TRadioStation
		{
		EUnknown,
		EPersonal,
		ERecommendations,
		ENeighbourhood,
		ELovedTracks,
		EMyPlaylist,
		EArtist,
		ETag,
		EUser
		};
	
	enum TMode
		{
		EOffline,
		EOnline
		};
	
	enum TState
		{
		ENone,
		EConnecting,
		EHandshaking,
		ERadioSelect,
		ERadioPlaylist,
		EUpdates
		};
	
public:
	static CMobblerLastFMConnection* NewL(MMobblerLastFMConnectionObserver& aObserver, const TDesC& aUsername, const TDesC& aPassword, TUint32 aIapID, TBool aCheckUpdates);
	~CMobblerLastFMConnection();
	
	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetModeL(TMode aMode);
	TMode Mode() const;
	TState State() const;
	
	void SetIapIDL(TUint32 aIadID);
	TUint32 IapID() const;
	
	// Updates
	TInt CheckForUpdateL();
	void SetCheckForUpdatesL(TBool aCheckUpdates);
	
	// Scrobbling methods
	void TrackStartedL(CMobblerTrack* aCurrentTrack);
	void TrackStoppedL();
	
	// Radio APIs
	void SetRadioPlayer(MMobblerRadioPlayer& aRadioPlayer);
	TInt RadioStartL(TRadioStation aRadioStation, const TDesC8& aRadioText);
	void RequestMp3L(MMobblerDownloadObserver& aDownloadObserver, CMobblerTrack* aTrack);
	void RequestPlaylistL();
	void RadioStop();
	
	// Web services APIs
	TInt UserGetFriendsL(const TDesC8& aUser, MWebServicesObserver& aObserver);
	TInt TrackBanL(const CMobblerTrack& aTrack);
	TInt ArtistGetInfoL(const CMobblerTrack& aTrack, MWebServicesObserver& aObserver);
	
private:
	void RunL();
	void DoCancel();
	
private: // from MHTTPTransactionCallback
	void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	
private: // from MMobblerTransactionObserver
	void TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse);
	void TransactionCompleteL(CMobblerTransaction* aTransaction);
	void TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aStatus, TInt aStatusCode);
	
private:
	void DoNowPlayingL();
	TBool DoSubmitL();
	void DoTrackLoveL();
	
	void ChangeState(TState aState);
	
	// Settings
	void LoadSettingsL();
	void SaveSettingsL();
	
	void DoSetModeL(TMode aMode);
	
	// handshaking
	void AuthenticateL();
	
	void HandshakeL();
	void WSHandshakeL();
	void RadioHandshakeL();
	
	void HandleHandshakeErrorL(CMobblerLastFMError* aError);
	
private:
	void ConstructL(const TDesC& aUsername, const TDesC& aPassword);
	CMobblerLastFMConnection(MMobblerLastFMConnectionObserver& aObserver, TUint32 aIapID, TBool aAutoUpdatesOn);
	
private:  // utilities
	void CreateAuthToken(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp);
	
	// track queue methods
	void LoadTrackQueueL();
	void DoLoadTrackQueueL();
	void SaveTrackQueue();

	void ConnectL();
	void Disconnect();
	
	TBool Connected();
	
private:
	RHTTPSession iHTTPSession;
	RSocketServ iSocketServ;
	RConnection iConnection;
	
	TUint32 iIapID;
	TUint32 iCurrentIapID;
	
	CMobblerTransaction* iHandshakeTransaction;
	CMobblerTransaction* iRadioHandshakeTransaction;
	CMobblerTransaction* iWebServicesHandshakeTransaction;
	CMobblerTransaction* iRadioSelectStationTransaction;
	CMobblerTransaction* iRadioPlaylistTransaction;
	CMobblerTransaction* iRadioAlbumArtTransaction;
	CMobblerTransaction* iNowPlayingTransaction;
	CMobblerTransaction* iSubmitTransaction;
	CMobblerTransaction* iTrackLoveTransaction;
	CMobblerTransaction* iTrackBanTransaction;
	CMobblerTransaction* iUpdateTransaction;
	CMobblerTransaction* iArtistGetInfoTransaction;
	
	RHTTPTransaction iRadioAudioTransaction;
	MMobblerDownloadObserver* iDownloadObserver;
	
	TBool iSubscriber;

	HBufC* iUsername;
	HBufC* iPassword;
	
	HBufC8* iSessionID;
	
	HBufC8* iWebServicesSessionKey;
	
	HBufC8* iRadioSessionID;
	HBufC8* iRadioStreamURL;
	HBufC8* iRadioBaseURL;
	HBufC8* iRadioBasePath;
	
	HBufC8* iNowPlayingURL;
	HBufC8* iSubmitURL;
	
	MMobblerRadioPlayer* iRadioPlayer;
	
	MMobblerLastFMConnectionObserver& iObserver;
	
	CMobblerTrack* iCurrentTrack;
	//CMobblerTrack* iPreviousTrack;
	
	RPointerArray<CMobblerTrack> iTrackQueue;
	RPointerArray<CMobblerTrack> iLovedTrackQueue;
	
	TMode iMode;
	TState iState;
	TBool iAuthenticated;
	
	TTime iNextUpdateCheck;
	TBool iCheckForUpdates;
	};

#endif // __MOBBLERLASTFMCONNECTION_H__
