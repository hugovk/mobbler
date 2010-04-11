/*
mobblermusiclistener.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade

http://code.google.com/p/mobbler

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <bautils.h> 
#include <utf.h>

#include "mobblerappui.h"
#include "mobblermusiclistener.h"
#include "mobblernowplayingcallback.h"
#include "mobblerradioplayer.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"

#ifdef __SYMBIAN_SIGNED__
const TUid KMobblerMusicAppInterfaceUid = {0x20027117};
#else
const TUid KMobblerMusicAppInterfaceUid = {0xA000D9F6};
#endif

CMobblerMusicAppListener* CMobblerMusicAppListener::NewL(CMobblerLastFmConnection& aSubmitter)
	{
    TRACER_AUTO;
	CMobblerMusicAppListener* self(new(ELeave) CMobblerMusicAppListener(aSubmitter));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppListener::CMobblerMusicAppListener(CMobblerLastFmConnection& aSubmitter)
	:iLastFmConnection(aSubmitter)
	{
    TRACER_AUTO;
	}

void CMobblerMusicAppListener::ConstructL()
	{
    TRACER_AUTO;
	RImplInfoPtrArray implInfoPtrArray;
	CleanupClosePushL(implInfoPtrArray);
	
	REComSession::ListImplementationsL(KMobblerMusicAppInterfaceUid, implInfoPtrArray);
	
	const TInt KImplCount(implInfoPtrArray.Count());
	for (TInt i(0); i < KImplCount ; ++i)
		{
		TUid dtorIdKey;
		CMobblerMusicApp* mobblerMusicApp(NULL);
		TRAPD(error, mobblerMusicApp = static_cast<CMobblerMusicApp*>(REComSession::CreateImplementationL(implInfoPtrArray[i]->ImplementationUid(), dtorIdKey, dynamic_cast<MMobblerMusicAppObserver*>(this))));
		
		if (error == KErrNone)
			{
			iMobblerMusicApps.AppendL(mobblerMusicApp);
			iDtorIdKeys.AppendL(dtorIdKey);
			}
		else
			{
			REComSession::DestroyedImplementation(dtorIdKey);
			}
		
		delete implInfoPtrArray[i];
		}
	
	CleanupStack::PopAndDestroy(&implInfoPtrArray);
	
	iMusicPlayerState = EPlayerNotRunning;
	
	// check if there is a song playing when Mobbler is started
	ScheduleNowPlayingL();
	}

CMobblerMusicAppListener::~CMobblerMusicAppListener()
	{
    TRACER_AUTO;
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		}
	
	iMobblerMusicApps.ResetAndDestroy();
	
	const TInt KDtorIdKeysCount(iDtorIdKeys.Count());
	for (TInt i(0); i < KDtorIdKeysCount; ++i)
		{
		REComSession::DestroyedImplementation(iDtorIdKeys[i]);
		}
	
	iDtorIdKeys.Close();
	
	// not supposed to call this in app code, but there seems to be problems reinstalling without it
	REComSession::FinalClose();
	
	delete iNowPlayingCallback;
	
	iObservers.Close();
	}

void CMobblerMusicAppListener::AddObserverL(MMobblerMusicAppListenerObserver* aObserver)
	{
    TRACER_AUTO;
	iObservers.InsertInAddressOrderL(aObserver);
	}

void CMobblerMusicAppListener::RemoveObserver(MMobblerMusicAppListenerObserver* aObserver)
	{
    TRACER_AUTO;
	TInt position(iObservers.FindInAddressOrder(aObserver));
	
	if (position != KErrNotFound)
		{
		iObservers.Remove(position);
		}
	}

void CMobblerMusicAppListener::NotifyChangeL()
	{
    TRACER_AUTO;
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleMusicAppChangeL();
		}
	}

CMobblerTrack* CMobblerMusicAppListener::CurrentTrack()
	{
//	TRACER_AUTO;
	return iCurrentTrack;
	}

void CMobblerMusicAppListener::HandleTrackChangeL(const TDesC& /*aTrack*/)
	{
    TRACER_AUTO;
	iLastFmConnection.TrackStoppedL(iCurrentTrack);
	
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		iCurrentTrack = NULL;
		}
	
	ScheduleNowPlayingL();
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::HandleMusicStateChangeL(TInt aState)
	{
    TRACER_AUTO;
	if (aState == EPlayerPlaying || aState == EPlayerPaused)
		{
		ScheduleNowPlayingL();
		}
	else
		{
		iLastFmConnection.TrackStoppedL(iCurrentTrack);
		
		if (iCurrentTrack)
			{
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		}
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::ScheduleNowPlayingL()
	{
    TRACER_AUTO;
	if (iNowPlayingCallback && iNowPlayingCallback->IsActive())
		{
		// We are already waiting for a callback so let that happen
		}
	else
		{
		delete iNowPlayingCallback;
		iNowPlayingCallback = CMobblerNowPlayingCallback::NewL(*this);
		}
	}

HBufC* CMobblerMusicAppListener::MusicAppNameL() const
	{
    TRACER_AUTO;
	HBufC* musicAppName(NULL);
	
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0); i < KMusicAppCount; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
			{
			musicAppName = iMobblerMusicApps[i]->NameL();
			break;
			}
		else if (iMobblerMusicApps[i]->PlayerState() == EPlayerPaused)
			{
			musicAppName = iMobblerMusicApps[i]->NameL();
			// Intentionally don't break if found a paused app,
			// so a playing app takes precendence.
			}
		}
	
	return musicAppName;
	}

void CMobblerMusicAppListener::NowPlayingL()
	{
    TRACER_AUTO;
	if (iCurrentTrack)
		{
		// We are currently listening to a track so just send the old one again
		iLastFmConnection.TrackStartedL(iCurrentTrack);
		}
	else
		{
		// We are not listening to a track so fetch the details
		// and try to submit it as 'now playing'
		TBuf<KMaxMobblerTextSize> trackArtist;
		TBuf<KMaxMobblerTextSize> trackTitle;
		TBuf<KMaxMobblerTextSize> trackAlbum;
		
		TTimeIntervalSeconds trackLength(0);
		
		TTime startTimeUTC;
		startTimeUTC.UniversalTime();
		startTimeUTC -= KNowPlayingCallbackDelay;
		
		// find the first music app observer that is playing a track
		TInt musicAppIndex(KErrNotFound);
		const TInt KMusicAppCount(iMobblerMusicApps.Count());
		for (TInt i(0); i < KMusicAppCount; ++i)
			{
			if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
				{
				musicAppIndex = i;
				iMusicPlayerState = EPlayerPlaying;
				break;
				}
			}
		
		if (musicAppIndex != KErrNotFound)
			{
			trackArtist.Copy(iMobblerMusicApps[musicAppIndex]->Artist());
			trackTitle.Copy(iMobblerMusicApps[musicAppIndex]->Title());
			trackAlbum.Copy(iMobblerMusicApps[musicAppIndex]->Album());
			trackLength = iMobblerMusicApps[musicAppIndex]->Duration();
			
			if (trackLength.Int() >= 30 && trackArtist.Length() > 0)
				{
				HBufC8* artist(CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackArtist));
				CleanupStack::PushL(artist);
				HBufC8* title(CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackTitle));
				CleanupStack::PushL(title);
				
				if (iCurrentTrack)
					{
					iCurrentTrack->Release();
					}
				
				if (trackAlbum.Length() == 0)
					{
					iCurrentTrack = CMobblerTrack::NewL(*artist, *title, KNullDesC8, /*KNullDesC8,*/ KNullDesC8, KNullDesC8, KNullDesC8, trackLength, KNullDesC8, EFalse);
					iCurrentTrack->FindLocalTrackL();
					}
				else
					{
					HBufC8* album(CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackAlbum));
					CleanupStack::PushL(album);
					iCurrentTrack = CMobblerTrack::NewL(*artist, *title, *album, /*KNullDesC8,*/ KNullDesC8, KNullDesC8, KNullDesC8, trackLength, KNullDesC8, EFalse);
					iCurrentTrack->FindLocalTrackL();
					CleanupStack::PopAndDestroy(album);
					}
				CleanupStack::PopAndDestroy(2, artist); // artist, title
				iCurrentTrack->SetStartTimeUTC(startTimeUTC);
				iLastFmConnection.TrackStartedL(iCurrentTrack);
				}
			}
		}
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::PlayerStateChangedL(TMobblerMusicAppObserverState aState)
	{
    TRACER_AUTO;
	TMobblerMusicAppObserverState oldState(iMusicPlayerState);
	TMobblerMusicAppObserverState newState(aState);
	iMusicPlayerState = newState;
	
	 // Not playing -> playing
	if ((oldState != EPlayerPlaying) && 
		(newState == EPlayerPlaying))
		{
		if (static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->
												RadioPlayer().CurrentTrack())
			{
			static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->
												RadioPlayer().StopL();
			}


		// Set start time = now
		if (iCurrentTrack)
			{
			TTime startTimeUTC;
			startTimeUTC.UniversalTime();
			iCurrentTrack->SetStartTimeUTC(startTimeUTC);
			}
		ScheduleNowPlayingL();
		}
	 // Playing -> not playing
	else if ((oldState == EPlayerPlaying) && 
			 (newState != EPlayerPlaying))
		{
		// Update total played
		if (iCurrentTrack)
			{
			TTimeIntervalSeconds totalPlayed(iCurrentTrack->TotalPlayed());
			
			TTime now;
			now.UniversalTime();
			TTimeIntervalSeconds newPeriod;
			User::LeaveIfError(now.SecondsFrom(iCurrentTrack->StartTimeUTC(), 
											   newPeriod));
			
			TInt64 newTotal(totalPlayed.Int() + newPeriod.Int());
			iCurrentTrack->SetTotalPlayed(newTotal);
			}
		ScheduleNowPlayingL();
		}
	
	 //  -> non-playing/paused state
	if ((newState != EPlayerPlaying) && 
		(newState != EPlayerPaused))
		{
		if (iCurrentTrack)
			{
			iCurrentTrack->SetTrackPlaying(EFalse);
			iLastFmConnection.TrackStoppedL(iCurrentTrack);
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		}
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::TrackInfoChangedL(const TDesC& /*aTitle*/, const TDesC& /*aArtist*/)
	{
    TRACER_AUTO;
	if (iCurrentTrack)
		{
		iLastFmConnection.TrackStoppedL(iCurrentTrack);
		
		iCurrentTrack->Release();
		iCurrentTrack = NULL;
		}
	
	ScheduleNowPlayingL();
	}

void CMobblerMusicAppListener::CommandReceivedL(TMobblerMusicAppObserverCommand aCommand)
	{
    TRACER_AUTO;
	if (aCommand == EPlayerCmdPlay)
		{
		ScheduleNowPlayingL();
		}
	else if (aCommand != EPlayerCmdBack 
				&& aCommand != EPlayerCmdNoCommand)
		{
		iLastFmConnection.TrackStoppedL(iCurrentTrack);
		
		if (iCurrentTrack)
			{
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		}
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::PlayerPositionL(TTimeIntervalSeconds aPlayerPosition)
	{
    TRACER_AUTO;
	if (iCurrentTrack)
		{
		if (iCurrentTrack->PlaybackPosition() != aPlayerPosition)
			{
			iCurrentTrack->SetPlaybackPosition(aPlayerPosition);
			static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
			iLastFmConnection.SaveCurrentTrackL();
			}
		}
	}

TBool CMobblerMusicAppListener::IsPlaying() const
	{
    TRACER_AUTO;
	return (iCurrentTrack && iMusicPlayerState == EPlayerPlaying);
	}

TBool CMobblerMusicAppListener::ControlsSupported()
	{
    TRACER_AUTO;
	// find the first music app observer that is playing a track
	TInt musicAppIndex(KErrNotFound);
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0) ; i < KMusicAppCount ; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
			{
			musicAppIndex = i;
			break;
			}
		}
	
	return musicAppIndex == KErrNotFound ? EFalse : iMobblerMusicApps[musicAppIndex]->ControlsSupported();
	}

void CMobblerMusicAppListener::PlayL()
	{
    TRACER_AUTO;
	// find the first music app observer that is playing a track
	TInt musicAppIndex(KErrNotFound);
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0) ; i < KMusicAppCount ; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
			{
			musicAppIndex = i;
			break;
			}
		}
	
	if (musicAppIndex != KErrNotFound)
		{
		iMobblerMusicApps[musicAppIndex]->PlayL();
		}
	}

void CMobblerMusicAppListener::StopL()
	{
    TRACER_AUTO;
	// find the first music app observer that is playing a track
	TInt musicAppIndex(KErrNotFound);
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0) ; i < KMusicAppCount ; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
			{
			musicAppIndex = i;
			break;
			}
		}
	
	if (musicAppIndex != KErrNotFound)
		{
		iMobblerMusicApps[musicAppIndex]->StopL();
		}
	}

void CMobblerMusicAppListener::SkipL()
	{
    TRACER_AUTO;
	// find the first music app observer that is playing a track
	TInt musicAppIndex(KErrNotFound);
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0) ; i < KMusicAppCount ; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EPlayerPlaying)
			{
			musicAppIndex = i;
			break;
			}
		}
	
	if (musicAppIndex != KErrNotFound)
		{
		iMobblerMusicApps[musicAppIndex]->SkipL();
		}
	}

// End of file
