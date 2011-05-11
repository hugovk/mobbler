/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010, 2011  Hugo van Kemenade
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

#ifndef __MOBBLERLASTFMCONNECTION_H__
#define __MOBBLERLASTFMCONNECTION_H__

#include <e32base.h>
#include <es_sock.h>
#ifdef __SYMBIAN_SIGNED__
#include <Etel3rdParty.h>
#endif
#include <http/mhttptransactioncallback.h>
#include <http/rhttpsession.h>
#include <mobbler/mobblerdestinationsinterface.h>

#include "mobbler.hrh"
#include "mobblerflatdataobserverhelper.h"
#include "mobblerlastfmerror.h"

class CHTTPFormEncoder;
class CMobblerString;
class CMobblerTrack;
class CMobblerTrackBase;
class CMobblerTransaction;
class MMobblerFlatDataObserver;
class MMobblerLastFmConnectionObserver;
class MMobblerSegDataObserver;

class MMobblerConnectionStateObserver
	{
public:
	virtual void HandleConnectionStateChangedL() = 0;
	};

class CMobblerLastFmConnection : public CActive,
									public MHTTPTransactionCallback,
									public MMobblerDestinationsInterfaceObserver,
									public MMobblerFlatDataObserverHelper
	{
public:
	friend class CMobblerTransaction;
	
public:
	enum TLastFmMemberType
		{
		EMemberTypeUnknown,
		EMember,
		ESubscriber
		};

	enum TTransactionError
		{
		ETransactionErrorNone,
		ETransactionErrorHandshake,
		ETransactionErrorFailed,
		ETransactionErrorCancel
		};

	enum TRadioStation
		{
		EUnknown,
		EPersonal,
		ERecommendations,
		ENeighbourhood,
		EArtist,
		ETag,
		EGroup,
		ECustom,
		EMix,
		EFriends
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
		EHandshaking
		};

	enum TEventStatus
		{
		EAttending,
		EMaybe,
		ENotAttending
		};

	enum TTwitterCommand
		{
		ETweet,
		EFollowMobbler,
		EAccessToken
		};

public:
	static CMobblerLastFmConnection* NewL(MMobblerLastFmConnectionObserver& aObserver,
											const TDesC& aUsername,
											const TDesC& aPassword,
											TUint32 aIapId,
											TInt aBitRate);
	~CMobblerLastFmConnection();

	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetModeL(TMode aMode);
	TMode Mode() const;
	TState State() const;

	void SetIapIdL(TUint32 aIadId);
	TUint32 IapId() const;

	void SetBitRate(TInt aBitRate);

	inline RSocketServ& SocketServ() { return iSocketServ; }
	inline RConnection& Connection() { return iConnection; }

	TLastFmMemberType MemberType() const;

	// Scrobbler log access
	TInt ScrobbleLogCount() const;
	const CMobblerTrackBase& ScrobbleLogItem(TInt aIndex) const;
	void RemoveScrobbleLogItemL(TInt aIndex);
	void ScrobbleTrackL(const CMobblerTrackBase* aTrack, const TBool aSubmit = ETrue);
	
	// state observers
	void AddStateChangeObserverL(MMobblerConnectionStateObserver* aObserver);
	void RemoveStateChangeObserver(MMobblerConnectionStateObserver* aObserver);

	// Updates
	void CheckForUpdateL(MMobblerFlatDataObserver& aObserver);

	// Scrobbling methods
	void TrackStartedL(CMobblerTrack* aTrack);
	void TrackStoppedL(const CMobblerTrackBase* aTrack);

	// Radio APIs
	void SelectStationL(MMobblerFlatDataObserver* aObserver, TRadioStation aRadioStation, const TDesC8& aRadioText);
	void RequestPlaylistL(MMobblerFlatDataObserver* aObserver);

	void RequestMp3L(MMobblerSegDataObserver& aObserver, const TDesC8& aMp3Location);
	void RadioStop();

	void RequestImageL(MMobblerFlatDataObserver* aObserver, const TDesC8& aImageLocation);
	void CancelTransaction(MMobblerFlatDataObserver* aObserver);

	// Web services APIs
	void WebServicesCallL(const TDesC8& aClass, const TDesC8& aMethod, const TDesC8& aText, MMobblerFlatDataObserver& aObserver, TInt aPage = KErrNotFound, TInt aPerPage = KErrNotFound, TBool aLang = EFalse);

	void ShoutL(const TDesC8& aClass, const TDesC8& aArgument, const TDesC8& aMessage);

	void TrackBanL(const TDesC8& aArtist, const TDesC8& aTrack);

	void ShareL(const TInt aCommand, 
				const TDesC8& aRecipient, 
				const TDesC8& aArtist, 
				const TDesC8& aAlbum, 
				const TDesC8& aTrack, 
				const TDesC8& aEventId, 
				const TDesC8& aMessage, 
				MMobblerFlatDataObserver& aObserver);

	void EventAttendL(const TDesC8& aEventId, TEventStatus aEventStatus, MMobblerFlatDataObserver& aObserver);

	void RecommendedEventsL(MMobblerFlatDataObserver& aObserver);

	void RecentTracksL(const TDesC8& aUser, MMobblerFlatDataObserver& aObserver);

	void SimilarL(const TInt aCommand, 
				  const TDesC8& aArtist, 
				  const TDesC8& aTrack, 
				  MMobblerFlatDataObserver& aObserver);
	
	void ArtistGetImageL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void QueryLastFmL(const TInt aCommand, 
					  const TDesC8& aArtist, 
					  const TDesC8& aAlbum, 
					  const TDesC8& aTrack, 
					  const TDesC8& aTag, 
					  MMobblerFlatDataObserver& aObserver);

	void GetInfoL(const TInt aCommand, 
				  const TDesC8& aArtist, 
				  const TDesC8& aAlbum, 
				  const TDesC8& aTrack, 
				  const TDesC8& aMbId, 
				  MMobblerFlatDataObserver& aObserver);

	void PlaylistCreateL(const TDesC& aTitle, const TDesC& aDescription, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchUserL(const TDesC8& aPlaylistId, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchAlbumL(const TDesC8& aAlbumId, MMobblerFlatDataObserver& aObserver);
	void PlaylistAddTrackL(const TDesC8& aPlaylistId, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	
	TBool ExportQueueToLogFileL();

	TBool ScrobblingOn() { return iScrobblingOn; }
	void ToggleScrobblingL();

	void LoadCurrentTrackL();
	void SaveCurrentTrackL();
	void CheckQueueAgeL();
	
#ifdef __SYMBIAN_SIGNED__
	// Ericsson API
	void GetLocationL(const CTelephony::TNetworkInfoV1& aNetworkInfo, MMobblerFlatDataObserver& aObserver);
	void GeoGetEventsL(const TDesC8& aLatitude, const TDesC8& aLongitude, MMobblerFlatDataObserver& aObserver);
#endif
	
	void ShortenL(const TDesC8& aUrl, MMobblerFlatDataObserver& aObserver);
	TBool QueryTwitterL(const TInt aCommand, 
						MMobblerFlatDataObserver& aObserver, 
						const TDesC8& aTweet = KNullDesC8);
	
private:
	void RunL();
	void DoCancel();

private: // from MMobblerDestinationsInterfaceObserver
	void PreferredCarrierAvailable();
	void NewCarrierActive();

private: // from MHTTPTransactionCallback
	void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);

private: // from MMobblerTransactionObserver
	void TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse);
	void TransactionCompleteL(CMobblerTransaction* aTransaction);
	void TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aResponse, const TDesC8& aStatus, TInt aStatusCode);

private:
	void DoNowPlayingL();
	TBool DoSubmitL();

	void ChangeStateL(TState aState);
	void DoSetModeL(TMode aMode);

	// handshaking
	void AuthenticateL();
	void WebServicesHandshakeL();
	void OldRadioHandshakeL();
#ifdef FULL_BETA_BUILD
	void BetaHandshakeL();
#endif

	void HandleHandshakeErrorL(CMobblerLastFmError* aError);

private:
	void ConstructL(const TDesC& aUsername, const TDesC& aPassword);
	CMobblerLastFmConnection(MMobblerLastFmConnectionObserver& aObserver, TUint32 aIapId, TInt aBitRate);

private:  // utilities
	void CreateAuthTokenL(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp);
	void StripOutTabs(TDes8& aString);
	CUri8* SetUpWebServicesUriLC();

	// track queue methods
	void LoadTrackQueueL();
	void SaveTrackQueueL();

	void ConnectL();
	void Disconnect();

	TBool Connected();

	CMobblerTransaction* CreateRequestPlaylistTransactionL(MMobblerFlatDataObserver* aObserver);
	void AppendAndSubmitTransactionL(CMobblerTransaction* aTransaction);
	void CloseTransactionsL(TBool aCloseTransactionArray);

	void DeleteCurrentTrackFile();
	
private: // from MMobblerFlatDataObserverHelper
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError);

private:
	RHTTPSession iHTTPSession;
	RSocketServ iSocketServ;
	RConnection iConnection;

	TUint32 iIapId;
	TUint32 iCurrentIapId;

	// authentication transactions
	CMobblerTransaction* iWebServicesHandshakeTransaction;
#ifdef FULL_BETA_BUILD
	CMobblerTransaction* iBetaTestersTransaction;
	TBool iIsBetaTester;
#endif

	// scrobble transactions
	CMobblerTransaction* iNowPlayingTransaction;
	CMobblerTransaction* iSubmitTransaction;

	// audio transaction
	RHTTPTransaction iRadioAudioTransaction;
	MMobblerSegDataObserver* iTrackDownloadObserver;
	HBufC8* iMp3Location;

	RPointerArray<CMobblerTransaction> iTransactions;

	TBool iSubscriber;

	CMobblerString* iUsername;
	CMobblerString* iPassword;

	HBufC8* iWebServicesSessionKey;

	MMobblerLastFmConnectionObserver& iObserver;

	CMobblerTrackBase* iCurrentTrack;
	CMobblerTrackBase* iUniversalScrobbledTrack;

	RPointerArray<CMobblerTrackBase> iTrackQueue;

	TMode iMode;
	TState iState;
	TBool iAuthenticated;

	TInt iBitRate;
	TBool iScrobblingOn;
	TBool iCurrentTrackSaved;

	RPointerArray<MMobblerConnectionStateObserver> iStateChangeObservers;

	TLastFmMemberType iMemberType;

	TBool i64KbpsWarningShown;
	TInt iDayNoInYearOfLastAgeCheck;
	
	CMobblerFlatDataObserverHelper* iTwitterTokenHelper;
	HBufC8* iTweet;
	MMobblerFlatDataObserver* iTweetObserver;
	TInt64 iNonceSeed;
	};

#endif // __MOBBLERLASTFMCONNECTION_H__

// End of file
