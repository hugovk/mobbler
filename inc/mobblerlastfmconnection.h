/*
mobblerlastfmconnection.h

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

#ifndef __MOBBLERLASTFMCONNECTION_H__
#define __MOBBLERLASTFMCONNECTION_H__

#include <e32base.h>
#include <es_sock.h>
#include <http/mhttptransactioncallback.h> 
#include <http/rhttpsession.h>
#include <mobbler/mobblerdestinationsinterface.h>

#include "mobblerlastfmerror.h"

_LIT(KLogFile, "c:\\Data\\Mobbler\\.scrobbler.log");

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

class CMobblerLastFmConnection : public CActive, public MHTTPTransactionCallback, public MMobblerDestinationsInterfaceObserver
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
		ELovedTracks,
		EPlaylist,
		EArtist,
		ETag
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
	
	TLastFmMemberType MemberType() const;
	
	// Scrobbler log access
	TInt ScrobbleLogCount() const;
	const CMobblerTrackBase& ScrobbleLogItem(TInt aIndex) const;
	void RemoveScrobbleLogItemL(TInt aIndex);
	
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
	void WebServicesCallL(const TDesC8& aClass, const TDesC8& aMethod, const TDesC8& aText, MMobblerFlatDataObserver& aObserver);
	
	void ShoutL(const TDesC8& aClass, const TDesC8& aArgument, const TDesC8& aMessage);
	
	void TrackLoveL(const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	void TrackBanL(const TDesC8& aArtist, const TDesC8& aTrack);
	
	void TrackShareL(const TDesC8& aRecipient, const TDesC8& aArtist, const TDesC8& aTrack, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	void ArtistShareL(const TDesC8& aRecipient, const TDesC8& aArtist, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	void EventShareL(const TDesC8& aRecipient, const TDesC8& aEventId, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	
	void EventAttendL(const TDesC8& aEventId, TEventStatus aEventStatus, MMobblerFlatDataObserver& aObserver);
	
	void RecommendedArtistsL(MMobblerFlatDataObserver& aObserver);
	void RecommendedEventsL(MMobblerFlatDataObserver& aObserver);
	
	void RecentTracksL(const TDesC8& aUser, MMobblerFlatDataObserver& aObserver);
	void SimilarTracksL(const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	
	void SimilarArtistsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void ArtistGetImageL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void TrackGetTagsL(const TDesC8& aTrack, const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void TrackGetTopTagsL(const TDesC8& aTrack, const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void TrackAddTagL(const TDesC8& aTrack, const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	void TrackRemoveTagL(const TDesC8& aTrack, const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	
	void ArtistGetTagsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void ArtistGetTopTagsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void ArtistAddTagL(const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	void ArtistRemoveTagL(const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	void ArtistGetEventsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void AlbumGetTagsL(const TDesC8& aAlbum, const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void AlbumAddTagL(const TDesC8& aAlbum, const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	void AlbumRemoveTagL(const TDesC8& aAlbum, const TDesC8& aArtist, const TDesC8& aTag, MMobblerFlatDataObserver& aObserver);
	
	void TrackGetInfoL(const TDesC8& aTrack, const TDesC8& aArtist, const TDesC8& aMbId, MMobblerFlatDataObserver& aObserver);
	void AlbumGetInfoL(const TDesC8& aMbId, MMobblerFlatDataObserver& aObserver);
	void AlbumGetInfoL(const TDesC8& aAlbum, const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void PlaylistCreateL(const TDesC& aTitle, const TDesC& aDescription, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchUserL(const TDesC8& aPlaylistId, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchAlbumL(const TDesC8& aAlbumId, MMobblerFlatDataObserver& aObserver);
	void PlaylistAddTrackL(const TDesC8& aPlaylistId, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	
	void FoursquareL(const TDesC8& aLongitude, const TDesC8& aLatitude, MMobblerFlatDataObserver& aObserver);
	void FetchLyricsL(const TDesC8& aArtist, const TDesC8& aTitle, MMobblerFlatDataObserver& aObserver);
	
	TBool ExportQueueToLogFileL();
	
	TBool ScrobblingOn() { return iScrobblingOn; }
	void ToggleScrobblingL();
	
	void LoadCurrentTrackL();
	void SaveCurrentTrackL();
	
private:
	void RunL();
	void DoCancel();
	
private: // fomr MMobblerDestinationsInterfaceObserver
	void PreferredCarrierAvailable();
	void NewCarrierActive();
	
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
	
	void ChangeStateL(TState aState);
	void DoSetModeL(TMode aMode);
	
	// handshaking
	void AuthenticateL();
	
	void ScrobbleHandshakeL();
	void WebServicesHandshakeL();
	void OldRadioHandshakeL();
#ifdef BETA_BUILD
	void BetaHandshakeL();
#endif
	
	void HandleHandshakeErrorL(CMobblerLastFmError* aError);
	
private:
	void ConstructL(const TDesC& aUsername, const TDesC& aPassword);
	CMobblerLastFmConnection(MMobblerLastFmConnectionObserver& aObserver, TUint32 aIapId, TInt aBitRate);
	
private:  // utilities
	void CreateAuthTokenL(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp);
	void StripOutTabs(TDes8& aString);
	
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

private:
	RHTTPSession iHTTPSession;
	RSocketServ iSocketServ;
	RConnection iConnection;
	
	TUint32 iIapId;
	TUint32 iCurrentIapId;
	
	// authentication transactions
	CMobblerTransaction* iHandshakeTransaction;
	CMobblerTransaction* iWebServicesHandshakeTransaction;
	CMobblerTransaction* iOldRadioHandshakeTransaction;
#ifdef BETA_BUILD
	CMobblerTransaction* iBetaTestersTransaction;
	TBool iIsBetaTester;
#endif
	
	// Old radio things
	HBufC8* iOldRadioSessionId;
	HBufC8* iOldRadioBaseUrl;
	HBufC8* iOldRadioBasePath;
	
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
	
	HBufC8* iScrobbleSessionId;
	
	HBufC8* iWebServicesSessionKey;
	
	HBufC8* iNowPlayingUrl;
	HBufC8* iSubmitUrl;
	
	MMobblerLastFmConnectionObserver& iObserver;
	
	CMobblerTrackBase* iCurrentTrack;
	
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
	};

#endif // __MOBBLERLASTFMCONNECTION_H__

// End of file
