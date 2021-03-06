/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010, 2011, 2012  Hugo van Kemenade
Copyright (C) 2010 gw111zz

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

#include <aknnotewrappers.h> 
#include <centralrepository.h>
#include <chttpformencoder.h>
#include <coemain.h>
#include <commdbconnpref.h> 
#include <e32math.h>
#include <hash.h>
#include <httperr.h>
#include <httpstringconstants.h>
#include <imcvcodc.h>  
#include <ProfileEngineSDKCRKeys.h>
#include <s32file.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerdestinationsinterface.h"
#include "mobblerflatdataobserver.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblertransaction.h"
#include "mobblerutility.h"
#include "mobblerwebservicesquery.h"

#ifdef BETA_BUILD
#include "mobblerbeta.h"
#else
_LIT8(KLatesverFileLocation, "http://www.mobbler.co.uk/latestver.xml");
#endif


// The file name to store the queue of listened tracks
_LIT(KTracksFile, "c:track_queue.dat");
_LIT(KCurrentTrackFile, "c:current_track.dat");
_LIT8(KLogFileFieldSeperator, "\t");

_LIT(KIapId, "IAP\\Id");

_LIT8(KEmail, "email");
_LIT8(KHttp, "http");
_LIT8(KHttps, "https");
_LIT8(KLimit, "limit");
_LIT8(KMessage, "message");
_LIT8(KPassword, "password");
_LIT8(KPlaylistUrl, "playlistURL");
_LIT8(KUsername, "username");
_LIT8(KQueryPlaylistFetch, "playlist.fetch");

// Last.fm can accept up to this many track in one submission
const TInt KMaxSubmitTracks(50);

CMobblerLastFmConnection* CMobblerLastFmConnection::NewL(MMobblerLastFmConnectionObserver& aObserver, 
															const TDesC& aRecipient, 
															const TDesC& aPassword, 
															TUint32 aIapId, 
															TInt aBitRate)
	{
	TRACER_AUTO;
	CMobblerLastFmConnection* self(new(ELeave) CMobblerLastFmConnection(aObserver, aIapId, aBitRate));
	CleanupStack::PushL(self);
	self->ConstructL(aRecipient, aPassword);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLastFmConnection::CMobblerLastFmConnection(MMobblerLastFmConnectionObserver& aObserver, TUint32 aIapId, TInt aBitRate)
	:CActive(CActive::EPriorityStandard), iIapId(aIapId), iObserver(aObserver), iBitRate(aBitRate),
#ifdef __WINS__
	iScrobblingOn(EFalse)
#else
	iScrobblingOn(ETrue)
#endif
	{
	TRACER_AUTO;
	CActiveScheduler::Add(this);
	}

CMobblerLastFmConnection::~CMobblerLastFmConnection()
	{
	TRACER_AUTO;
	Cancel();
	
	delete iCurrentTrack;
	delete iUniversalScrobbledTrack;
	
	iTrackQueue.ResetAndDestroy();
	
	delete iMp3Location;
	iRadioAudioTransaction.Close();
	
	iStateChangeObservers.Close();
	
	CloseTransactionsL(ETrue);
	
	delete iUsername;
	delete iPassword;
	
	delete iWebServicesSessionKey;
	
	iHTTPSession.Close();
	iConnection.Close();
	iSocketServ.Close();
	}

void CMobblerLastFmConnection::ConstructL(const TDesC& aRecipient, const TDesC& aPassword)
	{
	TRACER_AUTO;
	SetDetailsL(aRecipient, aPassword);
	LoadTrackQueueL();
	
	User::LeaveIfError(iSocketServ.Connect());
	}

void CMobblerLastFmConnection::DoSetModeL(TMode aMode)
	{
	TRACER_AUTO;
	iMode = aMode;
	
	// notify the state change observers when we change mode too
	const TInt KObserverCount(iStateChangeObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iStateChangeObservers[i]->HandleConnectionStateChangedL();
		}
	}

void CMobblerLastFmConnection::SetIapIdL(TUint32 aIapId)
	{
	TRACER_AUTO;
	if (aIapId != iIapId)
		{
		iIapId = aIapId;
		
		if (((iMode == EOnline || iState == EHandshaking) && iCurrentIapId != iIapId)
				|| iState == EConnecting)
			{
			// We are either online/handshaking and the new IAP is different to the old one
			// or we are trying to connect to a new IAP
			// so we should start connecting again
			ConnectL();
			}
		}
	}

void CMobblerLastFmConnection::SetBitRate(TInt aBitRate)
	{
//	TRACER_AUTO;
	iBitRate = aBitRate;
	}

TUint32 CMobblerLastFmConnection::IapId() const
	{
//	TRACER_AUTO;
	return iIapId;
	}

void CMobblerLastFmConnection::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	TRACER_AUTO;
	if (!iUsername
			|| iUsername && iUsername->String().CompareF(aUsername) != 0
			|| !iPassword
			|| iPassword && iPassword->String().Compare(aPassword) != 0)
		{
		// There is either no username or password set
		// or there is a new user or password
		
		HBufC* usernameLower(HBufC::NewLC(aUsername.Length()));
		usernameLower->Des().Copy(aUsername);
		usernameLower->Des().LowerCase();
		CMobblerString* tempUsername(CMobblerString::NewLC(*usernameLower));
		CMobblerString* tempPassword(CMobblerString::NewL(aPassword));
		CleanupStack::Pop(tempUsername);
		delete iUsername;
		delete iPassword;
		iUsername = tempUsername;
		iPassword = tempPassword;
		
		CleanupStack::PopAndDestroy(usernameLower);
		
		if (iMode == EOnline)
			{
			if (Connected())
				{
				// We are in online mode and connected so we should
				// handshake with Last.fm again
				AuthenticateL();
				}
			else
				{
				// We are online, but not connected
				// so redo the whole connection procedure
				if (iState != EConnecting && iState != EHandshaking)
					{
					// we are not already connecting
					ConnectL();
					}
				}
			}
		}
	}

void CMobblerLastFmConnection::SetModeL(TMode aMode)
	{
	TRACER_AUTO;
	if (aMode == EOnline)
		{
		// We are being asked to switch to online mode
		
		if (iMode != EOnline &&
				iState != EConnecting && iState != EHandshaking)
			{
			// We are not already in online mode and we are neither
			// connecting nor handshaking, so start the connecting process
			
			ConnectL();
			}
		}
	else if (aMode == EOffline)
		{
		Disconnect();
		}
	
	DoSetModeL(aMode);
	}

CMobblerLastFmConnection::TMode CMobblerLastFmConnection::Mode() const
	{
//	TRACER_AUTO;
	return iMode;
	}

CMobblerLastFmConnection::TState CMobblerLastFmConnection::State() const
	{
//	TRACER_AUTO;
	return iState;
	}

void CMobblerLastFmConnection::ChangeStateL(TState aState)
	{
	TRACER_AUTO;
	if (iState != aState)
		{
		iState = aState;
		static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
		
		// Notify the observers
		for (TInt i(0); i < iStateChangeObservers.Count(); ++i)
			{
			iStateChangeObservers[i]->HandleConnectionStateChangedL();
			}
		}
	}

void CMobblerLastFmConnection::AddStateChangeObserverL(MMobblerConnectionStateObserver* aObserver)
	{
	TRACER_AUTO;
	iStateChangeObservers.InsertInAddressOrderL(aObserver);
	}

void CMobblerLastFmConnection::RemoveStateChangeObserver(MMobblerConnectionStateObserver* aObserver)
	{
	TRACER_AUTO;
	TInt pos(iStateChangeObservers.FindInAddressOrder(aObserver));
	if (pos != KErrNotFound)
		{
		iStateChangeObservers.Remove(pos);
		}
	}

void CMobblerLastFmConnection::PreferredCarrierAvailable()
	{
	TRACER_AUTO;
	CloseTransactionsL(EFalse);
	iHTTPSession.Close();
	}

void CMobblerLastFmConnection::NewCarrierActive()
	{
	TRACER_AUTO;
	User::LeaveIfError(iConnection.GetIntSetting(KIapId, iCurrentIapId));
	
	iHTTPSession.OpenL();
	
	RStringPool strP(iHTTPSession.StringPool());
	RHTTPConnectionInfo connInfo(iHTTPSession.ConnectionInfo());
	connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), THTTPHdrVal(iSocketServ.Handle()));
	TInt connPtr(REINTERPRET_CAST(TInt, &iConnection));
	connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), THTTPHdrVal(connPtr));
	
	
	// Submit any request that do not require authentication
	TInt KTransactionCount(iTransactions.Count());
	for (TInt i(0); i < KTransactionCount; ++i)
		{
		if (!iTransactions[i]->RequiresAuthentication())
			{
			iTransactions[i]->SubmitL();
			}
		}
	
	if (iMp3Location)
		{
		// An mp3 had been requested after we had lost connection
		// so ask for it again now
		RequestMp3L(*iTrackDownloadObserver, *iMp3Location);
		}
	}

void CMobblerLastFmConnection::RunL()
	{
	TRACER_AUTO;
	if (iStatus.Int() == KErrNone)
		{
		CMobblerDestinationsInterface* destinations(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->Destinations());
		
		if (destinations)
			{
			// Register for mobility: this should connect us
			// to better networks when they become avaliable 
			
			// TODO: leave this out for now as it is causing problems
			//destinations->RegisterMobilityL(iConnection, this);
			}
		
		User::LeaveIfError(iConnection.GetIntSetting(KIapId, iCurrentIapId));
		
		iHTTPSession.Close();
		iHTTPSession.OpenL();
		
		RStringPool strP(iHTTPSession.StringPool());
		RHTTPConnectionInfo connInfo(iHTTPSession.ConnectionInfo());
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), THTTPHdrVal(iSocketServ.Handle()));
		TInt connPtr(REINTERPRET_CAST(TInt, &iConnection));
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), THTTPHdrVal(connPtr));
		
		// Submit any request that does not require authentication
		TInt KTransactionCount(iTransactions.Count());
		for (TInt i(0); i < KTransactionCount; ++i)
			{
			if (!iTransactions[i]->RequiresAuthentication())
				{
				iTransactions[i]->SubmitL();
				}
			}
		
		if (iMp3Location)
			{
			// An mp3 had been requested after we had lost connection
			// so ask for it again now
			RequestMp3L(*iTrackDownloadObserver, *iMp3Location);
			}
		
		// Authenticate now so that API calls that require it will be submitted.
		// This also means that any tracks in the queue will be submitted.
		AuthenticateL();
		}
	else
		{
		ChangeStateL(ENone);
		
		iObserver.HandleConnectCompleteL(iStatus.Int());
		
		CloseTransactionsL(ETrue);
		}
	
	delete iMp3Location;
	iMp3Location = NULL;
	}

CMobblerLastFmConnection::TLastFmMemberType CMobblerLastFmConnection::MemberType() const
	{
//	TRACER_AUTO;
	return iMemberType;
	}

void CMobblerLastFmConnection::DoCancel()
	{
	TRACER_AUTO;
	iConnection.Close();
	}

TBool CMobblerLastFmConnection::Connected()
	{
//	TRACER_AUTO;
	TBool connected(EFalse);
	
	if (iConnection.SubSessionHandle() != 0)
		{
		TNifProgress nifProgress;
		iConnection.Progress(nifProgress);
		connected = (nifProgress.iStage == KLinkLayerOpen);
		}
	
	return connected;
	}

void CMobblerLastFmConnection::ConnectL()
	{
	TRACER_AUTO;
	Cancel();
	Disconnect();
	ChangeStateL(EConnecting);
	
	User::LeaveIfError(iConnection.Open(iSocketServ));
	
	TConnPref* prefs;
	TCommDbConnPref dbConnPrefs;
	TConnSnapPref snapPrefs;
	
	if (iIapId == 0)
		{
		prefs = &dbConnPrefs;
		
		// This means the users has selected to always be asked
		// which access point they want to use
		dbConnPrefs.SetDialogPreference(ECommDbDialogPrefPrompt);
		
		// Filter out operator APs when the phone profile is offline
		TInt activeProfileId;
		CRepository* repository(CRepository::NewL(KCRUidProfileEngine));
		repository->Get(KProEngActiveProfile, activeProfileId);
		delete repository;
		
		const TInt KOfflineProfileId(5);
		if (activeProfileId == KOfflineProfileId)
			{
			dbConnPrefs.SetBearerSet(ECommDbBearerWLAN);
			}
		}
	else
		{
		if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->Destinations())
			{
			// We are using destinations so use the TCommSnapPref
			prefs = &snapPrefs;
			snapPrefs.SetSnap(iIapId);
			}
		else
			{
			prefs = &dbConnPrefs;
			dbConnPrefs.SetIapId(iIapId);
			dbConnPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
			}
		}
	
	iConnection.Start(*prefs, iStatus);
	
	SetActive();
	}

void CMobblerLastFmConnection::AuthenticateL()
	{
	TRACER_AUTO;
	iAuthenticated = EFalse;
	
	// Handshake with Last.fm
	ChangeStateL(EHandshaking);
	WebServicesHandshakeL();
#ifdef FULL_BETA_BUILD
	BetaHandshakeL();
#endif
	}

void CMobblerLastFmConnection::WebServicesHandshakeL()
	{
	TRACER_AUTO;
	// start the web services authentications
	iMemberType = EMemberTypeUnknown;
	
	delete iWebServicesSessionKey;
	iWebServicesSessionKey = NULL;
	
	_LIT8(KQueryAuthGetMobileSession, "auth.getMobileSession");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryAuthGetMobileSession));
	query->AddFieldL(KUsername, iUsername->String8());
	query->AddFieldL(KPassword, iPassword->String8());
	HBufC8* queryText(query->GetQueryAuthLC());
	
	CUri8* uri(SetUpWebServicesUriLC(ETrue));
	uri->SetComponentL(*queryText, EUriQuery);
	
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = CMobblerTransaction::NewL(*this, ETrue, uri, query);
	CleanupStack::Pop(uri);
	iWebServicesHandshakeTransaction->SubmitL();
	CleanupStack::PopAndDestroy(queryText);
	}

#ifdef FULL_BETA_BUILD
void CMobblerLastFmConnection::BetaHandshakeL()
	{
	TRACER_AUTO;
	// Start the web services authentications
	TUriParser8 uriParser;
	uriParser.Parse(KBetaTestersFileLocation);
	CUri8* uri(CUri8::NewLC(uriParser));
	
	delete iBetaTestersTransaction;
	iBetaTestersTransaction = CMobblerTransaction::NewL(*this, uri);
	CleanupStack::Pop(uri);
	iBetaTestersTransaction->SubmitL();
	}
#endif

void CMobblerLastFmConnection::CheckForUpdateL(MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	TUriParser8 uriParser;
	uriParser.Parse(KLatesverFileLocation);
	CUri8* uri(CUri8::NewLC(uriParser));
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	CleanupStack::Pop(uri);
	transaction->SetFlatDataObserver(&aObserver);
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::PlaylistCreateL(const TDesC& aTitle, const TDesC& aDescription, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryPlaylistCreate, "playlist.create");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryPlaylistCreate));
	
	CMobblerString* title(CMobblerString::NewLC(aTitle));
	query->AddFieldL(KTitle, title->String8());
	CleanupStack::PopAndDestroy(title);
	
	CMobblerString* description(CMobblerString::NewLC(aDescription));
	query->AddFieldL(KDescription, description->String8());
	CleanupStack::PopAndDestroy(description);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::PlaylistFetchUserL(const TDesC8& aPlaylistId, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	_LIT8(KUserPlaylistFormat, "lastfm://playlist/%S");
	
	CUri8* uri(SetUpWebServicesUriLC());
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryPlaylistFetch));
	
	HBufC8* playlistUrl(HBufC8::NewLC(KUserPlaylistFormat().Length() + aPlaylistId.Length()));
	playlistUrl->Des().Format(KUserPlaylistFormat, &aPlaylistId);
	query->AddFieldL(KPlaylistUrl, *playlistUrl);
	CleanupStack::PopAndDestroy(playlistUrl);
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::PlaylistFetchAlbumL(const TDesC8& aAlbumId, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	_LIT8(KUserPlaylistFormat, "lastfm://playlist/album/%S");
	
	CUri8* uri(SetUpWebServicesUriLC());
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryPlaylistFetch));
	
	HBufC8* playlistUrl(HBufC8::NewLC(KUserPlaylistFormat().Length() + aAlbumId.Length()));
	playlistUrl->Des().Format(KUserPlaylistFormat, &aAlbumId);
	query->AddFieldL(KPlaylistUrl, *playlistUrl);
	CleanupStack::PopAndDestroy(playlistUrl);
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::PlaylistAddTrackL(const TDesC8& aPlaylistId, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryPlaylistAddTrack, "playlist.addtrack");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryPlaylistAddTrack));
	_LIT8(KPlaylistId, "playlistID");
	query->AddFieldL(KPlaylistId, aPlaylistId);
	query->AddFieldL(KTrack, aTrack);
	query->AddFieldL(KArtist, aArtist);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::ShoutL(const TDesC8& aClass, const TDesC8& aArgument, const TDesC8& aMessage)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	TBuf8<KMaxMobblerTextSize> signature;
	_LIT8(KShoutFormat, "%S.shout");
	signature.Format(KShoutFormat, &aClass);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(signature));
	query->AddFieldL(aClass, aArgument);
	query->AddFieldL(KMessage, aMessage);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::RecommendedEventsL(MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryUserGetRecommendedEvents, "user.getrecommendedevents");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryUserGetRecommendedEvents));
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::TrackBanL(const TDesC8& aArtist, const TDesC8& aTrack)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryTrackBan, "track.ban");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryTrackBan));
	query->AddFieldL(KTrack, aTrack);
	query->AddFieldL(KArtist, aArtist);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::SimilarL(const TInt aCommand, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	CMobblerWebServicesQuery* query(NULL);
	switch (aCommand)
		{
		case EMobblerCommandSimilarArtists:
			_LIT8(KQueryArtistGetSimilar, "artist.getsimilar");
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistGetSimilar);
			break;
		case EMobblerCommandSimilarTracks:
			_LIT8(KQueryTrackGetSimilar, "track.getsimilar");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackGetSimilar);
			query->AddFieldL(KTrack, *MobblerUtility::URLEncodeLC(aTrack));
			CleanupStack::PopAndDestroy(); // URLEncodeLC()
			break;
		default:
			// TODO panic
			break;
		}

	query->AddFieldL(KArtist, *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // URLEncodeLC()
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::RecentTracksL(const TDesC8& aUser, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryUserGetRecentTracks, "user.getrecenttracks");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryUserGetRecentTracks));
	query->AddFieldL(KUser, *MobblerUtility::URLEncodeLC(aUser));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::ArtistGetImageL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryArtistGetImages, "artist.getimages");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryArtistGetImages));
	query->AddFieldL(KArtist, *MobblerUtility::URLEncodeLC(aArtist));
	query->AddFieldL(KLimit, K1);
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

/*void CMobblerLastFmConnection::ArtistGetTopTagsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryArtistGetTopTags, "artist.gettoptags");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryArtistGetTopTags));
	query->AddFieldL(KArtist, *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}*/

/*void CMobblerLastFmConnection::TrackGetTopTagsL(const TDesC8& aTrack, const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryTrackGetTopTags, "track.gettoptags");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryTrackGetTopTags));
	query->AddFieldL(KArtist, *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	query->AddFieldL(KTrack, *MobblerUtility::URLEncodeLC(aTrack));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}*/

void CMobblerLastFmConnection::QueryLastFmL(const TInt aCommand, 
											const TDesC8& aArtist, 
											const TDesC8& aAlbum, 
											const TDesC8& aTrack, 
											const TDesC8& aTag, 
											MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryArtistGetEvents, "artist.getEvents");
	CMobblerWebServicesQuery* query(NULL);
	switch (aCommand)
		{
		case EMobblerCommandTrackLove:
			_LIT8(KQueryTrackLove, "track.love");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackLove);
			query->AddFieldL(KTrack, aTrack);
			query->AddFieldL(KArtist, aArtist);
			break;
		case EMobblerCommandRecommendedArtists:
			_LIT8(KQueryUserGetRecommendedArtists, "user.getrecommendedartists");
			query = CMobblerWebServicesQuery::NewLC(KQueryUserGetRecommendedArtists);
			break;
		case EMobblerCommandArtistEvents:
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistGetEvents);
			query->AddFieldL(KArtist, aArtist);
			break;
		case EMobblerCommandArtistEventSingle:
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistGetEvents);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KLimit, K1);
			break;
		case EMobblerCommandArtistAddTag:
			_LIT8(KQueryArtistAddTags, "artist.addtags");
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistAddTags);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTags, aTag);
			break;
		case EMobblerCommandAlbumAddTag:
			_LIT8(KQueryAlbumAddTags, "album.addtags");
			query = CMobblerWebServicesQuery::NewLC(KQueryAlbumAddTags);
			query->AddFieldL(KAlbum, aAlbum);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTags, aTag);
			break;
		case EMobblerCommandTrackAddTag:
			_LIT8(KQueryTrackAddTags, "track.addtags");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackAddTags);
			query->AddFieldL(KTrack, aTrack);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTags, aTag);
			break;

		case EMobblerCommandArtistGetTags:
			_LIT8(KQueryArtistGetTags, "artist.gettags");
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistGetTags);
			query->AddFieldL(KArtist, aArtist);
			break;
		case EMobblerCommandAlbumGetTags:
			_LIT8(KQueryAlbumGetTags, "album.gettags");
			query = CMobblerWebServicesQuery::NewLC(KQueryAlbumGetTags);
			query->AddFieldL(KAlbum, aAlbum);
			query->AddFieldL(KArtist, aArtist);
			break;
		case EMobblerCommandTrackGetTags:
			_LIT8(KQueryTrackGetTags, "track.gettags");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackGetTags);
			query->AddFieldL(KTrack, aTrack);
			query->AddFieldL(KArtist, aArtist);
			break;

		case EMobblerCommandArtistRemoveTag:
			_LIT8(KQueryArtistRemoveTag, "artist.removetag");
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistRemoveTag);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTag, aTag);
			break;
		case EMobblerCommandAlbumRemoveTag:
			_LIT8(KQueryAlbumRemoveTag, "album.removetag");
			query = CMobblerWebServicesQuery::NewLC(KQueryAlbumRemoveTag);
			query->AddFieldL(KAlbum, aAlbum);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTag, aTag);
			break;
		case EMobblerCommandTrackRemoveTag:
			_LIT8(KQueryTrackRemoveTag, "track.removetag");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackRemoveTag);
			query->AddFieldL(KTrack, aTrack);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTag, aTag);
			break;
		default:
			// TODO panic
			break;
		}
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::GetInfoL(const TInt aCommand, const TDesC8& aArtist, const TDesC8& aAlbum, const TDesC8& aTrack, const TDesC8& aMbId, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	CMobblerWebServicesQuery* query(NULL);
	switch (aCommand)
		{
		case EMobblerCommandTrackGetInfo:
			_LIT8(KQueryTrackGetInfo, "track.getinfo");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackGetInfo);

			query->AddFieldL(KTrack, *MobblerUtility::URLEncodeLC(aTrack));
			CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack)
			
			query->AddFieldL(KUsername, iUsername->String8());
			break;
		case EMobblerCommandAlbumGetInfo:
			_LIT8(KQueryAlbumGetInfo, "album.getinfo");
			query = CMobblerWebServicesQuery::NewLC(KQueryAlbumGetInfo);
			
			if (aAlbum.Length() > 0)
				{
				query->AddFieldL(KAlbum, *MobblerUtility::URLEncodeLC(aAlbum));
				CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aAlbum)
				}
			break;
		default:
			// TODO panic
			break;
		}
	
	if (aMbId.Length() > 0)
		{
		query->AddFieldL(KMbid, aMbId);
		}

	if (aArtist.Length() > 0)
		{
		query->AddFieldL(KArtist, *MobblerUtility::URLEncodeLC(aArtist));
		CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aArtist)
		}
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::WebServicesCallL(const TDesC8& aClass, const TDesC8& aMethod, const TDesC8& aText, MMobblerFlatDataObserver& aObserver, TInt aPage, TInt aPerPage, TBool aLang)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KApiSignature, "%S.%S");
	TBuf8<KMaxMobblerTextSize> apiSignature;
	apiSignature.Format(KApiSignature, &aClass, &aMethod);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(apiSignature));
	
	if (aClass.Compare(KUser) == 0)
		{
		if (aText.Length() == 0)
			{
			query->AddFieldL(KUser, iUsername->String8());
			}
		else
			{
			query->AddFieldL(KUser, aText);
			}
		
		if (aMethod.Compare(KGetFriends) == 0)
			{
			// We are getting a user's friends so also ask for the most recent track
			query->AddFieldL(KRecentTracks, K1);
			}
		}
	else
		{
		query->AddFieldL(aClass, *MobblerUtility::URLEncodeLC(aText));
		CleanupStack::PopAndDestroy(); // URLEncodeLC
		}
	
	if (aPage != KErrNotFound)
		{
		TBuf8<3> page;
		page.AppendNum(aPage);
		_LIT8(KPage, "page");
		query->AddFieldL(KPage, page);
		}
	
	if (aPerPage != KErrNotFound)
		{
		TBuf8<3> perPage;
		perPage.AppendNum(aPerPage);
		_LIT8(KLimit, "limit");
		query->AddFieldL(KLimit, perPage);
		}
	
	if (aLang)
	    {
	    query->AddFieldL(KLang, MobblerUtility::LanguageL());
	    }
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // GetQueryLC
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::ShareL(const TInt aCommand, const TDesC8& aRecipient, const TDesC8& aArtist, const TDesC8& aAlbum, const TDesC8& aTrack, const TDesC8& aEventId, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	CMobblerWebServicesQuery* query(NULL);
	switch (aCommand)
		{
		case EMobblerCommandTrackShare:
			_LIT8(KQueryTrackShare, "track.share");
			query = CMobblerWebServicesQuery::NewLC(KQueryTrackShare);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KTrack, aTrack);
			break;
		case EMobblerCommandAlbumShare:
			_LIT8(KQueryAlbumShare, "album.share");
			query = CMobblerWebServicesQuery::NewLC(KQueryAlbumShare);
			query->AddFieldL(KArtist, aArtist);
			query->AddFieldL(KAlbum, aAlbum);
			break;
		case EMobblerCommandArtistShare:
			_LIT8(KQueryArtistShare, "artist.share");
			query = CMobblerWebServicesQuery::NewLC(KQueryArtistShare);
			query->AddFieldL(KArtist, aArtist);
			break;
		case EMobblerCommandEventShare:
			_LIT8(KQueryEventShare, "event.share");
			query = CMobblerWebServicesQuery::NewLC(KQueryEventShare);
			query->AddFieldL(KEvent, aEventId);
			break;
		default:
			// TODO panic
			break;
		}
	
	_LIT8(KRecipient, "recipient");
	query->AddFieldL(KRecipient, aRecipient);
	
	const TDesC& tagline(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_SHARE_TAGLINE));
	HBufC8* message(HBufC8::NewLC(aMessage.Length() + tagline.Length()));
	message->Des().Append(aMessage);
	message->Des().Append(tagline);
	query->AddFieldL(KMessage, *message);
	CleanupStack::PopAndDestroy(message);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::EventAttendL(const TDesC8& aEventId, TEventStatus aEventStatus, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryEventAttend, "event.attend");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryEventAttend));
	query->AddFieldL(KEvent, aEventId);
	
	switch (aEventStatus)
		{
		case EAttending:
			query->AddFieldL(KStatus, K0);
			break;
		case EMaybe:
			query->AddFieldL(KStatus, K1);
			break;
		case ENotAttending:
			_LIT8(K2, "2");
			query->AddFieldL(KStatus, K2);
			break;
		}
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::RadioStop()
	{
	TRACER_AUTO;
	if (iTrackDownloadObserver)
		{
		TRAP_IGNORE(iTrackDownloadObserver->DataCompleteL(CMobblerLastFmConnection::ETransactionErrorCancel, KErrNone, KNullDesC8));
		iTrackDownloadObserver = NULL;
		}
	
	iRadioAudioTransaction.Close();
	}

void CMobblerLastFmConnection::SelectStationL(MMobblerFlatDataObserver* aObserver, TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	TRACER_AUTO;
	// Set up the Last.fm formatted station URI
	HBufC8* radioUrl(HBufC8::NewLC(KMaxMobblerTextSize));
	HBufC8* text(NULL);
	
	if (aRadioText.Length() == 0)
		{
		// no text supplied so use the username
		text = iUsername->String8().AllocLC();
		}
	else
		{
		// text supplied so use it
		text = HBufC8::NewLC(aRadioText.Length());
		text->Des().Copy(aRadioText);
		}
	
	TPtr8 textPtr(text->Des());
	
	_LIT8(KRadioStationPersonal, "lastfm://user/%S/library");
	_LIT8(KRadioStationMix, "lastfm://user/%S/mix");
	_LIT8(KRadioStationArtist, "lastfm://artist/%S/similarartists");
	_LIT8(KRadioStationTag, "lastfm://globaltags/%S");
	_LIT8(KRadioStationFriends, "lastfm://user/%S/friends");
	_LIT8(KRadioStationNeighbours, "lastfm://user/%S/neighbours");
	_LIT8(KRadioStationRecommended, "lastfm://user/%S/recommended");
	_LIT8(KRadioStationGroup, "lastfm://group/%S");
	switch (aRadioStation)
		{
		case EPersonal: radioUrl->Des().AppendFormat(KRadioStationPersonal, &textPtr); break;
		case EMix: radioUrl->Des().AppendFormat(KRadioStationMix, &textPtr); break;
		case EFriends: radioUrl->Des().AppendFormat(KRadioStationFriends, &textPtr); break;
		case ERecommendations: radioUrl->Des().AppendFormat(KRadioStationRecommended, &textPtr); break;
		case ENeighbourhood: radioUrl->Des().AppendFormat(KRadioStationNeighbours, &textPtr); break;
		case EArtist: radioUrl->Des().AppendFormat(KRadioStationArtist, &textPtr); break;
		case ETag: radioUrl->Des().AppendFormat(KRadioStationTag, &textPtr); break;
		case EGroup: radioUrl->Des().AppendFormat(KRadioStationGroup, &textPtr); break;
		case ECustom: radioUrl->Des().Copy(*text); break;
		default: break;
		}
	
	LOG2(_L8("radioUrl"), *radioUrl);
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryRadioTune, "radio.tune");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryRadioTune));
	query->AddFieldL(KLang, MobblerUtility::LanguageL());
	query->AddFieldL(KStation, *radioUrl);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	CleanupStack::PopAndDestroy(text);
	CleanupStack::PopAndDestroy(radioUrl);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::RequestPlaylistL(MMobblerFlatDataObserver* aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryRadioGetPlaylist, "radio.getPlaylist");
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryRadioGetPlaylist));
	
	//query->AddFieldL(_L8("rtp"), _L8("?"));
	
	// Always ask for the mp3 to be downloaded at twice the speed that it plays at.
	// Should improve battery life by downloading for less time.
	_LIT8(KSpeedMultiplier, "speed_multiplier");
	_LIT8(KTwoDotZero, "2.0");
	query->AddFieldL(KSpeedMultiplier, KTwoDotZero);
	
	_LIT8(KBitRate, "bitrate");
	switch (iBitRate)
		{
		case 0:
			_LIT8(K64, "64"); 
			query->AddFieldL(KBitRate, K64);
			break;
		case 1:
			_LIT8(K128, "128");
			query->AddFieldL(KBitRate, K128);
			break;
		default:
			// TODO: should panic
			break;
		}
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFmConnection::RequestMp3L(MMobblerSegDataObserver& aObserver, const TDesC8& aMp3Location)
	{
	TRACER_AUTO;
	if (iMode == EOnline)
		{
		iTrackDownloadObserver = &aObserver;
		
		if (Connected())
			{
			// Request the mp3 data
			TUriParser8 urimp3Parser;
			urimp3Parser.Parse(aMp3Location);
			
			iRadioAudioTransaction.Close();
			iRadioAudioTransaction = iHTTPSession.OpenTransactionL(urimp3Parser, *this);
			
			_LIT8(KMobbler, "mobbler");
			RStringF mobbler(iHTTPSession.StringPool().OpenFStringL(KMobbler));
			iRadioAudioTransaction.Request().GetHeaderCollection().SetFieldL(iHTTPSession.StringPool().StringF(HTTP::EConnection, RHTTPSession::GetTable()), mobbler);
			mobbler.Close();
			
			iRadioAudioTransaction.SubmitL();
			}
		else if (iState != EConnecting && iState != EHandshaking)
			{
			iMp3Location = aMp3Location.AllocL();
			ConnectL();
			}
		}
	}

void CMobblerLastFmConnection::RequestImageL(MMobblerFlatDataObserver* aObserver, const TDesC8& aImageLocation)
	{
	TRACER_AUTO;
	if (aImageLocation.Compare(KNullDesC8) != 0 && Connected())
		{
		// Request the album art data
		TUriParser8 imageLocationParser;
		imageLocationParser.Parse(aImageLocation);
		CUri8* uri(CUri8::NewLC(imageLocationParser));
		CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
		CleanupStack::Pop(uri);
		transaction->SetFlatDataObserver(aObserver);
		AppendAndSubmitTransactionL(transaction);
		}
	}

#ifdef __SYMBIAN_SIGNED__
void CMobblerLastFmConnection::GetLocationL(const CTelephony::TNetworkInfoV1& aNetworkInfo, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	LOG2(_L8("Cell ID"), aNetworkInfo.iCellId);
	LOG2(_L8("LAC"), aNetworkInfo.iLocationAreaCode);
	
	_LIT(KApiKey, "5fqrIG5mGL0R4ll2KdoNJP1VmzcDf8ul2WFrREbv");
	_LIT(KLocationUrlFormat, "http://cellid.labs.ericsson.net/xml/lookup?cellid=%08x&mnc=%S&mcc=%S&lac=%04x&key=%S");
	
	HBufC* url(HBufC::NewLC(1024));
	url->Des().Format(KLocationUrlFormat, aNetworkInfo.iCellId, &aNetworkInfo.iNetworkId, &aNetworkInfo.iCountryCode, aNetworkInfo.iLocationAreaCode, &KApiKey());
	CMobblerString* urlString(CMobblerString::NewLC(*url));
	
	TUriParser8 locationUrl;
	locationUrl.Parse(urlString->String8());
	CUri8* uri(CUri8::NewLC(locationUrl));
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	
	CleanupStack::Pop(uri);
	CleanupStack::PopAndDestroy(2, url);
	
	transaction->SetFlatDataObserver(&aObserver);
	AppendAndSubmitTransactionL(transaction);
	}
#endif // __SYMBIAN_SIGNED__

void CMobblerLastFmConnection::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
	TRACER_AUTO;
	}

#ifdef __SYMBIAN_SIGNED__
void CMobblerLastFmConnection::GeoGetEventsL(const TDesC8& aLatitude, const TDesC8& aLongitude, MMobblerFlatDataObserver& aObserver)
	{
	TRACER_AUTO;
	CUri8* uri(SetUpWebServicesUriLC());
	
	_LIT8(KQueryGeoGetEvents, "geo.getevents");
	_LIT8(KLong, "long");
	_LIT8(KLat, "lat");
	_LIT8(KDistance, "distance");
	_LIT8(K10, "10");
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(KQueryGeoGetEvents));
	query->AddFieldL(KLong, aLongitude);
	query->AddFieldL(KLat, aLatitude);
	query->AddFieldL(KDistance, K10);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, EFalse, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	transaction->SetFlatDataObserver(&aObserver);
	AppendAndSubmitTransactionL(transaction);
	}
#endif // __SYMBIAN_SIGNED__

void CMobblerLastFmConnection::CancelTransaction(MMobblerFlatDataObserver* aObserver)
	{
	TRACER_AUTO;
	// cancel all image downloads associated the observer
	for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
		{
		if (iTransactions[i]->FlatDataObserver() == aObserver)
			{
			delete iTransactions[i];
			iTransactions.Remove(i);
			}
		}
	}

void CMobblerLastFmConnection::Disconnect()
	{
	TRACER_AUTO;
	Cancel();
	
	ChangeStateL(ENone);
	
	CloseTransactionsL(EFalse);
	
	iAuthenticated = EFalse;
	
	iHTTPSession.Close();
	iConnection.Close();
	}

void CMobblerLastFmConnection::DoNowPlayingL()
	{
	TRACER_AUTO;
	if (iCurrentTrack)
		{
		iObserver.HandleTrackNowPlayingL(*iCurrentTrack);
		
		if (iMode == EOnline && iAuthenticated)
			{
			// We must be in online mode and have recieved the now playing URL and session ID from Last.fm
			// before we try to submit and tracks
			_LIT8(KTrackUpdateNowPlaying, "track.updateNowPlaying");
			CMobblerWebServicesQuery* nowPlayingQuery(CMobblerWebServicesQuery::NewLC(KTrackUpdateNowPlaying));
			nowPlayingQuery->AddFieldL(KArtist, iCurrentTrack->Artist().String8());
			nowPlayingQuery->AddFieldL(KTrack, iCurrentTrack->Title().String8());
			
			if (iCurrentTrack->Album().String8().Length() != 0)
				{
				nowPlayingQuery->AddFieldL(KAlbum, iCurrentTrack->Album().String8());
				}
			
			TBuf8<10> trackLength;
			trackLength.AppendNum(iCurrentTrack->TrackLength().Int());
			_LIT8(KDuration, "duration");
			nowPlayingQuery->AddFieldL(KDuration, trackLength);
			
			CUri8* uri(SetUpWebServicesUriLC());
			
			delete iNowPlayingTransaction;
			iNowPlayingTransaction = CMobblerTransaction::NewL(*this, ETrue, uri, nowPlayingQuery);
			iNowPlayingTransaction->SubmitL();
			CleanupStack::Pop(2, nowPlayingQuery);
			}
		}
	}

void CMobblerLastFmConnection::TrackStartedL(CMobblerTrack* aTrack)
	{
	TRACER_AUTO;
	delete iCurrentTrack;
	iCurrentTrack = CMobblerTrackBase::NewL(*aTrack);
	
	DoNowPlayingL();
	}
	
void CMobblerLastFmConnection::TrackStoppedL(const CMobblerTrackBase* aTrack)
	{
	TRACER_AUTO;
	// Make sure that we haven't already tried to scrobble this track
	if (iCurrentTrack && !iCurrentTrack->Scrobbled())
		{
		iCurrentTrack->SetScrobbled();
		
		TTimeIntervalSeconds listenedFor(0);
		
		if (!iCurrentTrack->IsMusicPlayerTrack())
			{
			// It's a radio track so test the amount of continuous playback
			listenedFor = aTrack->PlaybackPosition();
			}
		else
			{
			// The current track is a music player app so test the amount of continuous playback
			listenedFor = iCurrentTrack->TotalPlayed().Int();
			
			if (iCurrentTrack->TrackPlaying())
				{
				TTimeIntervalSeconds lastPlayedSection(0);
				
				TTime now;
				now.UniversalTime();
				User::LeaveIfError(now.SecondsFrom(iCurrentTrack->StartTimeUTC(), lastPlayedSection));
				
				listenedFor = listenedFor.Int() + lastPlayedSection.Int();
				}
			}
		
		// Test if the track passes Last.fm's scrobble rules
		if (iScrobblingOn 
			&& listenedFor.Int() >= iCurrentTrack->ScrobbleDuration().Int()
			&& iCurrentTrack->TrackLength().Int() >= 30				// Track length is over 30 seconds
			&& iCurrentTrack->Artist().String().Length() > 0)		// Must have an artist name
			{
			// It passed, so notify and append it to the list
			iObserver.HandleTrackQueuedL(*iCurrentTrack);
			iTrackQueue.AppendL(iCurrentTrack);
			iCurrentTrack = NULL;
			CheckQueueAgeL();
			}
		}
	
	// There is no longer a current track
	delete iCurrentTrack;
	iCurrentTrack = NULL;
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	
	// Save the track queue and try to do a submission
	DeleteCurrentTrackFile();
	SaveTrackQueueL();
	DoSubmitL();
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->TrackStoppedL();
	}

TBool CMobblerLastFmConnection::DoSubmitL()
	{
	TRACER_AUTO;
	TBool submitting(EFalse);
	
	if (iMode == EOnline && iAuthenticated && !iSubmitTransaction)
		{
		// We are connected and not already submitting tracks 
		// so try to submit the tracks in the queue
		
		const TInt KSubmitTracksCount(iTrackQueue.Count());
		
		if (KSubmitTracksCount > 0)
			{
			_LIT8(KTrackScrobble, "track.scrobble");
			CMobblerWebServicesQuery* submitQuery(CMobblerWebServicesQuery::NewLC(KTrackScrobble));
			
			for (TInt ii(0); ii < KSubmitTracksCount && ii < KMaxSubmitTracks; ++ii)
				{
				_LIT8(KArtistFormat, "artist[%d]");
				_LIT8(KAlbumFormat, "album[%d]");
				_LIT8(KTrackFormat, "track[%d]");
				_LIT8(KTimestampFormat, "timestamp[%d]");
				_LIT8(KDurationFormat, "duration[%d]");
				_LIT8(KTrackNumberFormat, "trackNumber[%d]");
				_LIT8(KStreamIdFormat, "streamId[%d]");
			
				TBuf8<16> artist;
				TBuf8<16> album;
				TBuf8<16> track;
				TBuf8<16> timestamp;
				TBuf8<16> duration;
				TBuf8<16> trackNumber;
				TBuf8<16> streamId;
				artist.AppendFormat(KArtistFormat, ii);
				album.AppendFormat(KAlbumFormat, ii);
				track.AppendFormat(KTrackFormat, ii);
				timestamp.AppendFormat(KTimestampFormat, ii);
				duration.AppendFormat(KDurationFormat, ii);
				trackNumber.AppendFormat(KTrackNumberFormat, ii);
				streamId.AppendFormat(KStreamIdFormat, ii);
				
				submitQuery->AddFieldL(artist, iTrackQueue[ii]->Artist().String8());
				submitQuery->AddFieldL(track, iTrackQueue[ii]->Title().String8());
				
				if (iTrackQueue[ii]->Album().String8().Length() != 0)
					{
					submitQuery->AddFieldL(album, iTrackQueue[ii]->Album().String8());
					}
				
				TTimeIntervalSeconds unixTimeStamp;
				TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
				User::LeaveIfError(iTrackQueue[ii]->StartTimeUTC().SecondsFrom(epoch, unixTimeStamp));
				TBuf8<20> startTimeBuf;
				startTimeBuf.AppendNum(unixTimeStamp.Int());
				submitQuery->AddFieldL(timestamp, startTimeBuf);
				
				if (iTrackQueue[ii]->StreamId().Length() != 0)
					{
					// This is a radio track so submit the stream ID
					submitQuery->AddFieldL(streamId, iTrackQueue[ii]->StreamId());
					}
				
				if (iTrackQueue[ii]->Love() != CMobblerTrack::ENoLove)
					{
					// Make sure we also tell Last.fm in a web service call
					iTrackQueue[ii]->LoveTrackL();
					}
				
				TBuf8<10> trackLength;
				trackLength.AppendNum(iTrackQueue[ii]->TrackLength().Int());
				submitQuery->AddFieldL(duration, trackLength);
				
				if (iTrackQueue[ii]->TrackNumber() != KErrUnknown)
					{
					TBuf8<10> number;
					number.AppendNum(iTrackQueue[ii]->TrackNumber());
					submitQuery->AddFieldL(trackNumber, number);
					}
				}
			
			CUri8* uri(SetUpWebServicesUriLC());
			
			delete iSubmitTransaction;
			iSubmitTransaction = CMobblerTransaction::NewL(*this, ETrue, uri, submitQuery);
			iSubmitTransaction->SubmitL();
			CleanupStack::Pop(2, submitQuery);
			
			submitting = ETrue;
			}
		}
	
	return submitting;
	}

void CMobblerLastFmConnection::HandleHandshakeErrorL(CMobblerLastFmError* aError)
	{
	TRACER_AUTO;
	if (!aError)
		{
		// The handshake was ok
		
		if (iWebServicesSessionKey
#ifdef FULL_BETA_BUILD
				&& iIsBetaTester
#endif
				)
			{
			iAuthenticated = ETrue;
			
			// only notify the UI when we are fully connected
			iObserver.HandleConnectCompleteL(KErrNone);
			ChangeStateL(ENone);
			
			DoSubmitL();
			
			const TInt KTransactionCount(iTransactions.Count());
			for (TInt i(0); i < KTransactionCount; ++i)
				{
				iTransactions[i]->SubmitL();
				}
			
			if (iTrackDownloadObserver)
				{
				// There is a track observer so this must be because
				// we failed to start downloading a track, but have now reconnected
				iTrackDownloadObserver->DataCompleteL(ETransactionErrorHandshake, KErrNone, KNullDesC8);
				iTrackDownloadObserver = NULL;
				}
			}
		}
	else
		{
		// There was an error with one of the handshakes
		iObserver.HandleLastFmErrorL(*aError);
		ChangeStateL(ENone);
		iWebServicesHandshakeTransaction->Cancel();
		
		// close all transactions that require authentication
		// but let other ones carry on
		for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
			{
			if (iTransactions[i]->RequiresAuthentication())
				{
				delete iTransactions[i];
				iTransactions.Remove(i);
				}
			}
		}
	}

void CMobblerLastFmConnection::AppendAndSubmitTransactionL(CMobblerTransaction* aTransaction)
	{
	TRACER_AUTO;
	if (aTransaction)
		{
		iTransactions.AppendL(aTransaction);
		}
	
	if (iMode != EOnline)
		{
		if (iState != EConnecting && iState != EHandshaking)
			{
			if (iObserver.GoOnlineL())
				{
				SetModeL(EOnline);
				}
			else
				{
				CloseTransactionsL(ETrue);
				}
			}
		}
	else if (aTransaction && iAuthenticated && Connected())
		{
		// we are online and we have authenticated
		aTransaction->SubmitL();
		}
	else if (Connected())
		{
		// we are connected, but not authenticated
		AuthenticateL();
		}
	else
		{
		// we are online, but authentication was unsucsessful
		// so retry the whole connection procedure
		if (iState != EConnecting && iState != EHandshaking)
			{
			// we are not already connecting
			ConnectL();
			}
		}
	}

void CMobblerLastFmConnection::CloseTransactionsL(TBool aCloseTransactionArray)
	{
	TRACER_AUTO;
	// close any ongoing transactions
	delete iNowPlayingTransaction;
	iNowPlayingTransaction = NULL;
	delete iSubmitTransaction;
	iSubmitTransaction = NULL;
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = NULL;
#ifdef FULL_BETA_BUILD
	delete iBetaTestersTransaction;
	iBetaTestersTransaction = NULL;
#endif
	
	iRadioAudioTransaction.Close();
	
	if (iTrackDownloadObserver)
		{
		iTrackDownloadObserver->DataCompleteL(ETransactionErrorCancel, KErrNone, KNullDesC8);
		iTrackDownloadObserver = NULL;
		}
	
	if (aCloseTransactionArray)
		{
		// close all the transactions and callback the observers
		for (TInt i(iTransactions.Count() - 1); i >= 0; --i)
			{
			if (iTransactions[i]->FlatDataObserver())
				{
				iTransactions[i]->FlatDataObserver()->DataL(KNullDesC8, ETransactionErrorCancel);
				}
			
			delete iTransactions[i];
			iTransactions.Remove(i);
			}
		
		iTransactions.Close();
		}
	}

void CMobblerLastFmConnection::TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse)
	{
	TRACER_AUTO;
	if (aTransaction == iWebServicesHandshakeTransaction)
		{
		CMobblerLastFmError* error(CMobblerParser::ParseWebServicesHandshakeL(aResponse, iWebServicesSessionKey, iMemberType));
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
#ifdef FULL_BETA_BUILD
	else if (aTransaction == iBetaTestersTransaction)
		{
		CMobblerLastFmError* error(CMobblerParser::ParseBetaTestersHandshakeL(aResponse, iUsername->String8(), iIsBetaTester));
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
#endif
	else if (aTransaction == iSubmitTransaction)
		{
		CMobblerLastFmError* error(CMobblerParser::ParseScrobbleResponseL(aResponse));
		
		// We have done a submission so remove up to the
		// first KMaxSubmitTracks from the queued tracks array
		const TInt KCount(iTrackQueue.Count());
		for (TInt i(Min(KCount - 1, KMaxSubmitTracks - 1)) ; i >= 0 ; --i)
			{
			iObserver.HandleTrackSubmitted(*iTrackQueue[i]);
			delete iTrackQueue[i];
			iTrackQueue.Remove(i);
			}
			
		SaveTrackQueueL();
			
		if (error)
			{
			CleanupStack::PushL(error);
			iObserver.HandleLastFmErrorL(*error);
			CleanupStack::PopAndDestroy(error);
			}
		}
	else if (aTransaction == iNowPlayingTransaction)
		{
		CMobblerLastFmError* error(CMobblerParser::ParseScrobbleResponseL(aResponse));
		
		if (error)
			{
			CleanupStack::PushL(error);
			iObserver.HandleLastFmErrorL(*error);
			CleanupStack::PopAndDestroy(error);
			}
		else
			{
			// there was no error so try to submit any tracks in the queue
			DoSubmitL();
			}
		}
	else
		{
		const TInt KTransactionCount(iTransactions.Count());
		for (TInt i(0); i < KTransactionCount; ++i)
			{
			if (aTransaction == iTransactions[i])
				{
				if (iTransactions[i]->FlatDataObserver())
					{
					iTransactions[i]->FlatDataObserver()->DataL(aResponse, ETransactionErrorNone);
					}
				break;
				}
			}
		}
	}

void CMobblerLastFmConnection::TransactionCompleteL(CMobblerTransaction* aTransaction)
	{
	TRACER_AUTO;
	if (aTransaction == iSubmitTransaction)
		{
		// This pointer is used to tell if we are already submitting some tracks
		// so it is important to delete and set to NULL when finished
		delete iSubmitTransaction;
		iSubmitTransaction = NULL;
		
		if (!DoSubmitL())
			{
			// There are no more tracks to submit
			// so do a now playing
			DoNowPlayingL();
			}
		}
	else
		{
		for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
			{
			if (aTransaction == iTransactions[i])
				{
				delete aTransaction;
				iTransactions.Remove(i);
				break;
				}
			}
		}
	}

void CMobblerLastFmConnection::TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aResponse, const TDesC8& aStatus, TInt aStatusCode)
	{
	TRACER_AUTO;
	DUMPDATA(aResponse, _L("transactionfailed.xml"));
#ifdef _DEBUG
	// Transaction log file 
	_LIT(KTransactionLogFile, "C:\\Data\\Mobbler\\transaction.log");
	
	RFile file;
	CleanupClosePushL(file);
	
	CCoeEnv::Static()->FsSession().MkDirAll(KTransactionLogFile);
	
	TInt error(file.Open(CCoeEnv::Static()->FsSession(), KTransactionLogFile, EFileWrite));
	if (error != KErrNone)
		{
		error = file.Create(CCoeEnv::Static()->FsSession(), KTransactionLogFile, EFileWrite);
		}
	
	if (error == KErrNone)
		{
		// Move the seek head to the end of the file
		TInt fileSize;
		file.Size(fileSize);
		file.Seek(ESeekEnd,fileSize);
		
		// Write a line in the log file
		TBuf<KMaxMobblerTextSize> logMessage;
		
		TTime now;
		now.UniversalTime();
		
		now.FormatL(logMessage, _L("%F%D/%M/%Y %H:%T:%S\t"));
		
		if (aTransaction == iWebServicesHandshakeTransaction) logMessage.Append(_L("WebServicesHandsake\t"));
		else if (aTransaction == iNowPlayingTransaction) logMessage.Append(_L("NowPlaying\t"));
		else if (aTransaction == iSubmitTransaction) logMessage.Append(_L("Submit\t"));
		
		HBufC* status(HBufC::NewLC(aStatus.Length()));
		status->Des().Copy(aStatus);
		logMessage.Append(*status);
		CleanupStack::PopAndDestroy(status);
		logMessage.Append(_L("\t"));
		logMessage.AppendNum(aStatusCode);
		logMessage.Append(_L("\r\n"));
		
		HBufC8* logMessage8(HBufC8::NewLC(logMessage.Length()));
		logMessage8->Des().Copy(logMessage);
		file.Write(*logMessage8);
		
		CleanupStack::PopAndDestroy(logMessage8);
		CleanupStack::PopAndDestroy(&file);
		}
#endif
	
	if (aTransaction == iSubmitTransaction)
		{
		// This pointer is used to tell if we are already submitting some tracks
		// so it is important to delete and set to NULL when finished
		delete iSubmitTransaction;
		iSubmitTransaction = NULL;
		}
	
	if (!Connected() &&
			iState != EConnecting && iState != EHandshaking)
		{
		// The connection is not open so we should try to connect again
		ConnectL();
		}
	else
		{
		// If it was one of the handshake transactions then
		// complain to the app UI so an error is displayed
		if (aTransaction == iWebServicesHandshakeTransaction
#ifdef FULL_BETA_BUILD
				|| aTransaction == iBetaTestersTransaction
#endif
				)
			{
			iObserver.HandleCommsErrorL(aStatusCode, aStatus);
			ChangeStateL(ENone);
			
			// Because the handshaking failed we should fail and close all transactions 
			CloseTransactionsL(ETrue);
			}
		else
			{
			// find the transaction that failed
			// tell the observer that it failed and then remove the transaction
			for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
				{
				if (aTransaction == iTransactions[i])
					{
					if (aTransaction->FlatDataObserver())
						{
						aTransaction->FlatDataObserver()->DataL(aResponse, ETransactionErrorFailed);
						}
					
					delete aTransaction;
					iTransactions.Remove(i);
					break;
					}
				}
			}
		}
	}

void CMobblerLastFmConnection::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{
	TRACER_AUTO;
	// it must be a transaction event
	TPtrC8 nextDataPartPtr;
	
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseBodyData:
			{
			aTransaction.Response().Body()->GetNextDataPart(nextDataPartPtr);
			TInt dataSize(aTransaction.Response().Body()->OverallDataSize());
			if (iTrackDownloadObserver)
				{
				iTrackDownloadObserver->DataPart(nextDataPartPtr, dataSize);
				}
			aTransaction.Response().Body()->ReleaseData();
			}
			break;
		case THTTPEvent::EFailed:
			if (iTrackDownloadObserver)
				{
				iTrackDownloadObserver->DataCompleteL(ETransactionErrorFailed, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			iTrackDownloadObserver = NULL;
			break;
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			// tell the radio player that the track has finished downloading
			if (iTrackDownloadObserver)
				{
				iTrackDownloadObserver->DataCompleteL(ETransactionErrorCancel, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			break;
		case THTTPEvent::ESucceeded:
			// tell the radio player that the track has finished downloading
			if (iTrackDownloadObserver)
				{
				iTrackDownloadObserver->DataCompleteL(ETransactionErrorNone, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			iTrackDownloadObserver = NULL;
			break;
		default:
			break;
		}
	}

TInt CMobblerLastFmConnection::MHFRunError(TInt /*aError*/, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
	TRACER_AUTO;
	// send KErrNone back so that it doesn't panic
	return KErrNone;
	}

TInt CMobblerLastFmConnection::ScrobbleLogCount() const
	{
//	TRACER_AUTO;
	return iTrackQueue.Count();
	}

const CMobblerTrackBase& CMobblerLastFmConnection::ScrobbleLogItem(TInt aIndex) const
	{
//	TRACER_AUTO;
	return *iTrackQueue[aIndex];
	}

void CMobblerLastFmConnection::RemoveScrobbleLogItemL(TInt aIndex)
	{
	TRACER_AUTO;
	if (iTrackQueue.Count() > aIndex)
		{
		iObserver.HandleTrackDequeued(*iTrackQueue[aIndex]);
		
		delete iTrackQueue[aIndex];
		iTrackQueue.Remove(aIndex);
		
		// make sure this track is removed from the file
		SaveTrackQueueL();
		}
	}

void CMobblerLastFmConnection::LoadTrackQueueL()
	{
	TRACER_AUTO;
	iTrackQueue.ResetAndDestroy();
	
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KTracksFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TInt trackCount(0);
		TRAP_IGNORE(trackCount = readStream.ReadInt32L());
		
		for (TInt i(0); i < trackCount; ++i)
			{
			CMobblerTrackBase* track(CMobblerTrackBase::NewL(readStream));
			CleanupStack::PushL(track);
			iTrackQueue.AppendL(track);
			CleanupStack::Pop(track);
			}
		
		CleanupStack::PopAndDestroy(&readStream);
		
		const TInt KTrackQueueCount(iTrackQueue.Count());
		for (TInt i(KTrackQueueCount - 1); i >= 0 ; --i)
			{
			iObserver.HandleTrackQueuedL(*iTrackQueue[i]);
			}
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFmConnection::SaveTrackQueueL()
	{
	TRACER_AUTO;
	CCoeEnv::Static()->FsSession().MkDirAll(KTracksFile);
	
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KTracksFile, EFileWrite));
	
	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		const TInt KTracksCount(iTrackQueue.Count());
		
		writeStream.WriteInt32L(KTracksCount);
		
		for (TInt i(0); i < KTracksCount; ++i)
			{
			writeStream << *iTrackQueue[i];
			}
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFmConnection::CheckQueueAgeL()
	{
	TRACER_AUTO;
	TTime now;
	now.UniversalTime();
	TInt dayNoInYear(now.DayNoInYear());
	
	if (iDayNoInYearOfLastAgeCheck != dayNoInYear)
		{
		iDayNoInYearOfLastAgeCheck = dayNoInYear;
		
		const TInt KTrackQueueCount(iTrackQueue.Count());
		for (TInt i(0); i < KTrackQueueCount; ++i)
			{
/*			// For testing with 1 minute:
			TTimeIntervalMinutes minutesOld;
			now.MinutesFrom(iTrackQueue[i]->StartTimeUTC(), minutesOld);
			if (minutesOld.Int() > 1)*/
			if (now.DaysFrom(iTrackQueue[i]->StartTimeUTC()).Int() > 12)
				{
				// Warn the user to scrobble soon
				static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->WarnOldScrobblesL();
				break;
				}
			}
		}
	}

TBool CMobblerLastFmConnection::ExportQueueToLogFileL()
	{
	TRACER_AUTO;
	// Format described here: http://www.audioscrobbler.net/wiki/Portable_Player_Logging
	// Uploaders can be found here: http://www.rockbox.org/twiki/bin/view/Main/LastFmLog
	
	const TInt KTracksCount(iTrackQueue.Count());
	if (KTracksCount == 0)
		{
		return EFalse;
		}
	
	_LIT(KLogFile, "c:\\Data\\Mobbler\\.scrobbler.log");
	CCoeEnv::Static()->FsSession().MkDirAll(KLogFile);
	
	TInt errors(KErrNone);
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KLogFile, EFileWrite));
	_LIT8(KLogFileEndOfLine, "\n");
	if (replaceError != KErrNone)
		{
		return EFalse;
		}
	else
		{
		_LIT8(KLogFileHeader, "#AUDIOSCROBBLER/1.1\n#TZ/UTC\n#CLIENT/Mobbler ");
		errors = file.Write(KLogFileHeader);
		TBuf8<10> version;
		version.Copy(KVersion.Name());
		errors += file.Write(version);
		errors += file.Write(KLogFileEndOfLine);
		}
	
	if (errors != KErrNone)
		{
		return EFalse;
		}
	
	errors = KErrNone;
	for (TInt i(0); i < KTracksCount && errors == KErrNone; ++i)
		{
		// artist name
		TBuf8<KMaxMobblerTextSize> buf(iTrackQueue[i]->Artist().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// album name (optional)
		buf.Zero();
		buf.Append(iTrackQueue[i]->Album().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// track name
		buf.Zero();
		buf.Append(iTrackQueue[i]->Title().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// track position on album (optional)
		if (iTrackQueue[i]->TrackNumber() != KErrUnknown)
			{
			TBuf8<10> trackNumber;
			trackNumber.AppendNum(iTrackQueue[i]->TrackNumber());
			errors += file.Write(trackNumber);
			}
		file.Write(KLogFileFieldSeperator);
		
		// song duration in seconds
		TBuf8<10> trackLength;
		trackLength.AppendNum(iTrackQueue[i]->TrackLength().Int());
		errors += file.Write(trackLength);
		errors += file.Write(KLogFileFieldSeperator);
		
		// rating (L if listened at least 50% or S if skipped)
		_LIT8(KLogFileListenedRating, "L");
		errors += file.Write(KLogFileListenedRating);
		errors += file.Write(KLogFileFieldSeperator);
		
		// UNIX timestamp when song started playing
		TTimeIntervalSeconds unixTimeStamp;
		TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
		iTrackQueue[i]->StartTimeUTC().SecondsFrom(epoch, unixTimeStamp);
		TBuf8<20> startTimeBuf;
		startTimeBuf.AppendNum(unixTimeStamp.Int());
		errors += file.Write(startTimeBuf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// MusicBrainz Track ID (optional)
		errors += file.Write(KLogFileEndOfLine);
		}
	
	if (errors != KErrNone)
		{
		return EFalse;
		}
		
	// No errors, ok to empty the queue
	for (TInt i(KTracksCount - 1); i >= 0; --i)
		{
		// If the track was loved, tough, that can't be submitted via log file
		iObserver.HandleTrackDequeued(*iTrackQueue[i]);
		delete iTrackQueue[i];
		iTrackQueue.Remove(i);
		}
	SaveTrackQueueL();
	
	CleanupStack::PopAndDestroy(&file);
	
	return ETrue;
	}

void CMobblerLastFmConnection::StripOutTabs(TDes8& aString)
	{
	TRACER_AUTO;
	TInt position(aString.Find(KLogFileFieldSeperator));
	 while (position != KErrNotFound)
		{
		aString.Delete(position, 1);
		position = aString.Find(KLogFileFieldSeperator);
		}
	}

void CMobblerLastFmConnection::ToggleScrobblingL()
	{
	TRACER_AUTO;
	iScrobblingOn = !iScrobblingOn;
	
	iScrobblingOn ?
		SaveCurrentTrackL() :
		DeleteCurrentTrackFile();
	}

void CMobblerLastFmConnection::LoadCurrentTrackL()
	{
	TRACER_AUTO;
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(),
							 KCurrentTrackFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);

		//  There is already a current track so get rid of it
		delete iCurrentTrack;
		
		iCurrentTrack  = CMobblerTrackBase::NewL(readStream);
		iCurrentTrack->SetPlaybackPosition(readStream.ReadInt32L());
		iCurrentTrack->SetTotalPlayed(readStream.ReadInt32L());
		CleanupStack::PopAndDestroy(&readStream);
		TrackStoppedL(iCurrentTrack);
		DeleteCurrentTrackFile();
		}
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFmConnection::SaveCurrentTrackL()
	{
	TRACER_AUTO;
	if (!iCurrentTrackSaved &&
		iScrobblingOn && 
		iCurrentTrack)
		{
		TInt playbackPosition(Min(iCurrentTrack->PlaybackPosition().Int(), 
								  iCurrentTrack->TrackLength().Int()));
		TInt scrobbleTime(iCurrentTrack->InitialPlaybackPosition().Int() + 
						  iCurrentTrack->ScrobbleDuration().Int());

		if (playbackPosition >= scrobbleTime)
			{
			// Save the track to file
			CCoeEnv::Static()->FsSession().MkDirAll(KCurrentTrackFile);
			
			RFile file;
			CleanupClosePushL(file);
			TInt createError(file.Create(CCoeEnv::Static()->FsSession(), 
										 KCurrentTrackFile, EFileWrite));
			
			if (createError == KErrNone)
				{
				RFileWriteStream writeStream(file);
				CleanupClosePushL(writeStream);
				writeStream << *iCurrentTrack;
				writeStream.WriteInt32L(iCurrentTrack->PlaybackPosition().Int());
				writeStream.WriteInt32L(iCurrentTrack->TotalPlayed().Int());
				
				CleanupStack::PopAndDestroy(&writeStream);
				iCurrentTrackSaved = ETrue;
				}
			
			CleanupStack::PopAndDestroy(&file);
			}
		}
	}

void CMobblerLastFmConnection::DeleteCurrentTrackFile()
	{
	TRACER_AUTO;
	CCoeEnv::Static()->FsSession().Delete(KCurrentTrackFile);
	iCurrentTrackSaved = EFalse;
	}

CUri8* CMobblerLastFmConnection::SetUpWebServicesUriLC(
    const TBool aHttps)
	{
	TRACER_AUTO;
	CUri8* uri(CUri8::NewLC());
	
	_LIT8(KComponentTwoDotZero, "/2.0/");
    if (aHttps)
        {
        uri->SetComponentL(KHttps, EUriScheme);
        }
    else
        {
        uri->SetComponentL(KHttp, EUriScheme);
        }
	// The Last.fm URL to send the handshake to:
	_LIT8(KWebServicesHost, "ws.audioscrobbler.com");
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(KComponentTwoDotZero, EUriPath);
	
	return uri;
	}

void CMobblerLastFmConnection::ScrobbleTrackL(const CMobblerTrackBase* aTrack, const TBool aSubmit)
	{
	TRACER_AUTO;
	delete iUniversalScrobbledTrack;
	iUniversalScrobbledTrack = CMobblerTrackBase::NewL(*aTrack);
	iObserver.HandleTrackQueuedL(*iUniversalScrobbledTrack);
	iTrackQueue.AppendL(iUniversalScrobbledTrack);
	
	// Save the track queue and try a submission
	if (aSubmit)
		{
		SaveTrackQueueL();
		DoSubmitL();
		}
	
	iUniversalScrobbledTrack = NULL;
	}

// End of file
