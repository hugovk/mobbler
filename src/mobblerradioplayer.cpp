/*
mobblerradioplayer.cpp

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

#include <aknnotewrappers.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobbleraudiocontrol.h"
#include "mobbleraudiothread.h"
#include "mobblerincomingcallmonitor.h"
#include "mobblerlastfmconnection.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"

const TInt KDefaultMaxVolume(10);

// The radio should timeout and delete its playlists after 5 minutes
// so that we do not get tracks that can't be downloaded when restarting
const TTimeIntervalMicroSeconds32 KRadioTimeout(5 * 60 * 1000000);

CMobblerRadioPlayer* CMobblerRadioPlayer::NewL(CMobblerLastFmConnection& aSubmitter, 
												TTimeIntervalSeconds aPreBufferSize,
												TInt aEqualizerIndex, 
												TInt aVolume,
												TInt aBitRate)
	{
	CMobblerRadioPlayer* self(new(ELeave) CMobblerRadioPlayer(aSubmitter, aPreBufferSize, aEqualizerIndex, aVolume, aBitRate));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerRadioPlayer::CMobblerRadioPlayer(CMobblerLastFmConnection& aLastFmConnection,
											TTimeIntervalSeconds aPreBufferSize,
											TInt aEqualizerIndex,
											TInt aVolume,
											TInt aBitRate)
	:CActive(CActive::EPriorityStandard), 
	iLastFmConnection(aLastFmConnection), 
	iPreBufferSize(aPreBufferSize), 
	iVolume(aVolume), 
	iMaxVolume(KDefaultMaxVolume), 
	iEqualizerIndex(aEqualizerIndex),
	iBitRate(aBitRate)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerRadioPlayer::ConstructL()
	{
	iIncomingCallMonitor = CMobblerIncomingCallMonitor::NewL(*this);
	iStation = CMobblerString::NewL(KNullDesC);
	User::LeaveIfError(iTimer.CreateLocal());
	iPlaylist = CMobblerRadioPlaylist::NewL();
	iLastFmConnection.AddStateChangeObserverL(this);
	}

CMobblerRadioPlayer::~CMobblerRadioPlayer()
	{
	Cancel();
	iTimer.Close();
	
	delete iPlaylist;
	delete iCurrentAudioControl;
	delete iNextAudioControl;
	delete iStation;
	
	delete iIncomingCallMonitor;
	
	iObservers.Close();
	}

void CMobblerRadioPlayer::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		if (!CurrentTrack())
			{
			// The radio has not been playing for 5 minutes
			iPlaylist->Reset();
			}
		}
	}

void CMobblerRadioPlayer::DoCancel()
	{
	iTimer.Cancel();
	}

void CMobblerRadioPlayer::AddObserverL(MMobblerRadioStateChangeObserver* aObserver)
	{
	iObservers.InsertInAddressOrder(aObserver);
	}

void CMobblerRadioPlayer::RemoveObserver(MMobblerRadioStateChangeObserver* aObserver)
	{
	TInt position(iObservers.FindInAddressOrder(aObserver));
	
	if (position != KErrNotFound)
		{
		iObservers.Remove(position);
		}
	}

void CMobblerRadioPlayer::DoChangeStateL(TState aState)
	{
	iState = aState;
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleRadioStateChangedL();
		}
	
	if (iState == EIdle)
		{
		// make sure the timeout timer has started  
		if (!IsActive())
			{
			iTimer.After(iStatus, KRadioTimeout);
			SetActive();
			}
		}
	else
		{
		// cancel the timeout timer
		Cancel();
		}
	
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::DoChangeTransactionStateL(TTransactionState aTransactionState)
	{
	iTransactionState = aTransactionState;
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleRadioStateChangedL();
		}
	}

void CMobblerRadioPlayer::HandleConnectionStateChangedL()
	{
	switch (iLastFmConnection.State())
		{
		case ENone:
			{
			if (iRestart)
				{
				iRestart = EFalse;
				RequestPlaylistL(ETrue);
				DoChangeStateL(EStarting);
				}
			}
			break;
		case CMobblerLastFmConnection::EConnecting:
		case CMobblerLastFmConnection::EHandshaking:
			{
			if (iState == EPlaying)
				{
				// if we are in the playing state and we start
				// connecting then it means we have lost connection
				// we should stop the radio and wait to be told
				// that the connection is back, this will come through
				// the try again callback from the auido control
				DoStop(ETrue);
				iRestart = ETrue;
				}
			}
			break;
		}
	}

void CMobblerRadioPlayer::HandleAudioPositionChangeL()
	{
	// if we are finished downloading
	// and only have the prebuffer amount of time left of the track
	// then start downloading the next track
	
	if (!iNextAudioControl &&
			iCurrentAudioControl &&
			iCurrentAudioControl->DownloadComplete() &&
			(( (*iPlaylist)[0]->TrackLength().Int() - (*iPlaylist)[0]->PlaybackPosition().Int() ) <= iCurrentAudioControl->PreBufferSize().Int() ))
		{
		// There is another track in the playlist.
		// We have not created the next track yet.
		// The download on the current track is complete
		// and there is only the length of the pre-buffer to go on the current track 
		// so start downloading the next track now.
		
		if (iPlaylist->Count() > 1)
			{
			// There is more in the playlist so start fetching the next track
			iNextAudioControl = CMobblerAudioControl::NewL(*this,  *(*iPlaylist)[1], iPreBufferSize, iVolume, iEqualizerIndex, iBitRate);
			iLastFmConnection.RequestMp3L(*iNextAudioControl, (*iPlaylist)[0 + 1]->Mp3Location());
			DoChangeStateL(EPlaying);
			}
		}
	
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawDeferred();
	iLastFmConnection.SaveCurrentTrackL();
	}

void CMobblerRadioPlayer::HandleAudioFinishedL(CMobblerAudioControl* aAudioControl)
	{
	if (iState == EPlaying)
		{
		// We are playing and the track has finished for some reason so try to go to the next one
		
		if (aAudioControl == iCurrentAudioControl)
			{
			// The current track has finished so try to start the new one 
			SkipTrackL();
			}
		else if (aAudioControl == iNextAudioControl)
			{
			// The next audio track failed before this one so
			// delete it and remove it from the playlist
			delete iNextAudioControl;
			iNextAudioControl = NULL;
			
			if (iPlaylist->Count() > 1)
				{
				iPlaylist->RemoveAndReleaseTrack(1);
				}
			}
		}
	}

CMobblerRadioPlayer::TState CMobblerRadioPlayer::State() const
	{
	return iState;
	}

CMobblerRadioPlayer::TTransactionState CMobblerRadioPlayer::TransactionState() const
	{
	return iTransactionState;
	}

void CMobblerRadioPlayer::StartL(CMobblerLastFmConnection::TRadioStation aRadioStation, const CMobblerString* aRadioText)
	{
	delete iStation;
	iStation = NULL;
	
	TBuf<255> station;
	TPtrC text(KNullDesC);
	
	if (!aRadioText)
		{
		// use the user's username
		text.Set(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->SettingView().Username());
		}
	else
		{
		text.Set(aRadioText->String());
		}

	switch (aRadioStation)
		{
		case CMobblerLastFmConnection::EPersonal:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_PERSONAL_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::ERecommendations:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_RECOMMENDATIONS_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::ENeighbourhood:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NEIGHBOURHOOD_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::ELovedTracks:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_LOVED_TRACKS_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::EPlaylist:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::EArtist:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ARTIST_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::ETag:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_TAG_FORMAT), &text);
			break;
		case CMobblerLastFmConnection::EUnknown:
		default:
			break;
		}

	iStation = CMobblerString::NewL(station);
	
	// Stop the radio and also make sure that
	// we get rid of any playlists hanging around
	DoStop(ETrue);
	
	iPlaylist->Reset();
	
	// now ask for the radio to start again
	if (aRadioText)
		{
		HBufC8* urlEncoded(MobblerUtility::URLEncodeLC(aRadioText->String8()));
		iLastFmConnection.SelectStationL(this, aRadioStation, *urlEncoded);
		CleanupStack::PopAndDestroy(urlEncoded);
		}
	else
		{
		iLastFmConnection.SelectStationL(this, aRadioStation, KNullDesC8);
		}
	
	DoChangeStateL(EStarting);
	DoChangeTransactionStateL(ESelectingStation);
	}

void CMobblerRadioPlayer::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	switch (iTransactionState)
		{
		case ESelectingStation:
			{
			if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				CMobblerLastFmError* radioError(NULL);
				
				if (iLastFmConnection.MemberType() == CMobblerLastFmConnection::EMember)
					{
					radioError = CMobblerParser::ParseOldRadioTuneL(aData);
					}
				else
					{
					delete iStation;
					radioError = CMobblerParser::ParseRadioTuneL(aData, iStation);
					}
				
				if (!radioError)
					{
					// There were no errors selecting the station
					RequestPlaylistL(EFalse);
					}
				else
					{
					DoChangeTransactionStateL(ENone);
					DoChangeStateL(EIdle);
					
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(radioError->Text());
					}
				}
			else
				{
				// There was an error with the radio tune transaction
				DoChangeTransactionStateL(ENone);
				DoChangeStateL(EIdle);
				
				CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
				note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION));
				}
			}
			break;
		case EFetchingPlaylist:
			{
			if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				DoChangeTransactionStateL(ENone);
		
				CMobblerLastFmError* error(NULL);
				
				if (iLastFmConnection.MemberType() == CMobblerLastFmConnection::ESubscriber)
					{
					error = CMobblerParser::ParseRadioPlaylistL(aData, *iPlaylist);
					}
				else
					{
					error = CMobblerParser::ParseOldRadioPlaylistL(aData, *iPlaylist);
					}
				
				if (!error)
					{
					if (iState == EStarting)
						{
						// This will play the first song in the playlist.
						// If we are not starting then the playlist was
						// being fetched in the background so we do nothing
						SkipTrackL();
						}
					}
				else
					{
					// There was an error so display the error text
					
					CleanupStack::PushL(error);
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(error->Text());
					CleanupStack::PopAndDestroy(error);
					
					DoChangeTransactionStateL(ENone);
					DoChangeStateL(EIdle);
					}
				}
			else
				{
				// There was a transaction error when fetching

				if (iState == EStarting)
					{
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION));
					
					// Go back to the idle state
					DoChangeStateL(EIdle);
					}
				
				DoChangeTransactionStateL(ENone);
				}
			}
			break;
		default:
			DoChangeTransactionStateL(ENone);
			break;
		}
	}

void CMobblerRadioPlayer::SkipTrackL()
	{
	if 	(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->SleepAfterTrackStopped())
		{
		DoStop(ETrue);
		}
	else
		{
		DoStop(EFalse);
		
		if (iPlaylist->Count() < 5)
			{
			// This is the last track in the playlist so
			// fetch the next playlist
			
			RequestPlaylistL(EFalse);
			}
		
		if (iPlaylist->Count() > 0)
			{
			// We are indexing a track in this playlist so start it
			
			delete iCurrentAudioControl;
			iCurrentAudioControl = NULL;
			
			if (iNextAudioControl)
				{
				// We have already created the next audio control so use that
				iCurrentAudioControl = iNextAudioControl;
				iNextAudioControl = NULL;
				}
			else
				{
				// Create the next audio control and request the mp3
				iCurrentAudioControl = CMobblerAudioControl::NewL(*this, *(*iPlaylist)[0], iPreBufferSize, iVolume, iEqualizerIndex, iBitRate);
				iLastFmConnection.RequestMp3L(*iCurrentAudioControl, (*iPlaylist)[0]->Mp3Location());
				DoChangeStateL(EPlaying);
				}
			
			// Tell the audio thread that is should start writing data to the output stream
			iCurrentAudioControl->SetCurrent();
			
			// We have started playing the track so tell Last.fm
			CMobblerTrack* track((*iPlaylist)[0]);
			
			if (track->StartTimeUTC() == Time::NullTTime())
				{
				// we haven't set the start time for this track yet
				// so this must be the first time we are writing data
				// to the output stream.  Set the start time now.
				TTime now;
				now.UniversalTime();
				track->SetStartTimeUTC(now);
				}
			
			iLastFmConnection.TrackStartedL(track);
			}
		else
			{
			// There are no tracks in the playlist
			
			if (iState == EPlaying)
				{
				// We have skipped of the end of the fetched playlist
				// We are playing so we must already be fetching a playlist
				// so go back to the starting state and wait for the
				// playlist to downloads
				
				DoChangeStateL(EStarting);
				}
			}
		}
	}

void CMobblerRadioPlayer::UpdateVolume()
	{
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetVolume(iVolume);
		}
	if (iNextAudioControl)
		{
		iNextAudioControl->SetVolume(iVolume);
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::VolumeUp()
	{
	TInt volume(Volume());
	TInt maxVolume(MaxVolume());
	iVolume = Min(volume + (maxVolume / 10), maxVolume);
	
	UpdateVolume();
	}

void CMobblerRadioPlayer::VolumeDown()
	{
	TInt volume(Volume());
	TInt maxVolume(MaxVolume());
	iVolume = Max(volume - (maxVolume / 10), 0);
	
	UpdateVolume();
	}

void CMobblerRadioPlayer::SetVolume(TInt aVolume)
	{
	iVolume = Min(aVolume, MaxVolume());
	iVolume = Max(iVolume, 0);
	
	UpdateVolume();
	}

TInt CMobblerRadioPlayer::Volume() const
	{
	TInt volume(iVolume);
	
	if (iCurrentAudioControl)
		{
		volume = iCurrentAudioControl->Volume();
		}
	
	return volume;
	}

TInt CMobblerRadioPlayer::MaxVolume() const
	{
	TInt maxVolume(iMaxVolume);
	
	if (iCurrentAudioControl)
		{
		if (iMaxVolume != iCurrentAudioControl->MaxVolume())
			{
			// The max audio volume has changed
			// so correct the volume for this
			iCurrentAudioControl->SetVolume((iCurrentAudioControl->Volume() * iCurrentAudioControl->MaxVolume()) / iMaxVolume);
			}
		
		maxVolume = iCurrentAudioControl->MaxVolume();
		}
	
	return maxVolume;
	}

TInt CMobblerRadioPlayer::EqualizerIndex() const
	{
	return iEqualizerIndex;
	}

const CMobblerString& CMobblerRadioPlayer::Station() const
	{
	return *iStation;
	}

void CMobblerRadioPlayer::SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize)
	{
	iPreBufferSize = aPreBufferSize;
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetPreBufferSize(aPreBufferSize);
		}
	}

void CMobblerRadioPlayer::SetBitRateL(TInt aBitRate)
	{
	if (aBitRate != iBitRate)
		{
		// The sample rate has changed so we need to
		// fetch a new playlist for the next song.
		
		iBitRate = aBitRate;
		
		// Remove all future tracks, but not this one
		for (TInt i(iPlaylist->Count() - 1) ; i >= 1 ; --i)
			{
			iPlaylist->RemoveAndReleaseTrack(i);
			}
		
		if (iState == EPlaying)
			{
			// The next track will have the correct new bit rate.
			RequestPlaylistL(ETrue);
			}
		}
	}

void CMobblerRadioPlayer::RequestPlaylistL(TBool aCancelPrevious)
	{
	if (aCancelPrevious && iTransactionState == EFetchingPlaylist)
		{
		// Cancel the previous fetch playlist transaction
		iLastFmConnection.CancelTransaction(this);
		}
	
	if (aCancelPrevious || iTransactionState != EFetchingPlaylist)
		{
		DoChangeTransactionStateL(EFetchingPlaylist);
		iLastFmConnection.RequestPlaylistL(this);
		}
	}

void CMobblerRadioPlayer::Stop()
	{
	DoStop(ETrue);
	}

void CMobblerRadioPlayer::DoStop(TBool aFullStop)
	{
	// Try to submit the last played track
	SubmitCurrentTrackL();

	if (aFullStop)
		{
		// We are stopping the current track because we
		// are stopping the radio playing completely
		
		if (iCurrentAudioControl)
			{
			iPlaylist->RemoveAndReleaseTrack(0);
			
			delete iCurrentAudioControl;
			iCurrentAudioControl = NULL;
			}
		
		if (iNextAudioControl)
			{
			// if there was a next track then increment the current track
			// because we can't download the same track twice
			
			iPlaylist->RemoveAndReleaseTrack(0);
			
			delete iNextAudioControl;
			iNextAudioControl = NULL;
			}
		
		DoChangeStateL(EIdle);
		
		// Make sure any select station or fetching
		// playlist transactions are cancelled
		iLastFmConnection.CancelTransaction(this);
		DoChangeTransactionStateL(ENone);
		
		// stop all radio downloads
		iLastFmConnection.RadioStop();
		}
	else
		{
		//  We are stopping because we want to skip to the next track
		
		if (!iNextAudioControl)
			{
			// We don't want to delete the next track, but
			// it hasn't started downloading yet so stop
			// the current one
			iLastFmConnection.RadioStop();
			}
	
		if (iCurrentAudioControl)
			{
			iPlaylist->RemoveAndReleaseTrack(0);
			
			delete iCurrentAudioControl;
			iCurrentAudioControl = NULL;
			}	
		}
	
//	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

CMobblerTrack* CMobblerRadioPlayer::CurrentTrack()
	{
	if (iCurrentAudioControl && 
		(!iCurrentAudioControl->DownloadComplete() || iCurrentAudioControl->Playing()) && 
		iPlaylist->Count() > 0)
		{
		return (*iPlaylist)[0];
		}
	
	return NULL;
	}

CMobblerTrack* CMobblerRadioPlayer::NextTrack()
	{
	if (iPlaylist->Count() > 1)
		{
		return (*iPlaylist)[1];
		}
	
	return NULL;
	}

void CMobblerRadioPlayer::SubmitCurrentTrackL()
	{
	if (iCurrentAudioControl && (!iCurrentAudioControl->DownloadComplete() || iCurrentAudioControl->Playing()))
		{
		iLastFmConnection.TrackStoppedL(CurrentTrack());
		}
	}

void CMobblerRadioPlayer::HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState)
	{
	switch (aPSTelephonyCallState)
		{
		case EPSTelephonyCallStateAlerting:
		case EPSTelephonyCallStateRinging:
		case EPSTelephonyCallStateDialling:
		case EPSTelephonyCallStateAnswering:
		case EPSTelephonyCallStateConnected:
			// There was an incoming call so stop playing the radio
			if (iState == EPlaying)
				{
				DoStop(ETrue);
				iRestartRadioOnCallDisconnect = ETrue;
				}
			break;
		case EPSTelephonyCallStateDisconnecting:
			if (iRestartRadioOnCallDisconnect)
				{
				iRestartRadioOnCallDisconnect = EFalse;
				SkipTrackL();
				}
			break;
		case EPSTelephonyCallStateUninitialized:
		case EPSTelephonyCallStateNone:
		case EPSTelephonyCallStateHold:
		default:
			// do nothing
			break;
		}
	}

void CMobblerRadioPlayer::SetEqualizer(TInt aIndex)
	{
	iEqualizerIndex = aIndex;
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetEqualizerIndex(aIndex);
		}
	if (iNextAudioControl)
		{
		iNextAudioControl->SetEqualizerIndex(aIndex);
		}
	}

TBool CMobblerRadioPlayer::HasPlaylist() const
	{
	return (iPlaylist->Count() > 0);
	}

// End of file
