/*
mobblermusiclistener.cpp

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

#include <bautils.h> 
#include <mobbler/mobblercontentlistinginterface.h>
#include <utf.h>

#include "mobblerappui.h"
#include "mobblermusiclistener.h"
#include "mobblernowplayingcallback.h"
#include "mobblerradioplayer.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

#ifdef __SYMBIAN_SIGNED__
const TUid KMobblerMusicAppInterfaceUid = {0x2002655D};
const TUid KContentListingImplUid = {0x2002661E};
#else
const TUid KMobblerMusicAppInterfaceUid = {0xA0007CAA};
const TUid KContentListingImplUid = {0xA000BEB3};
#endif

CMobblerMusicAppListener* CMobblerMusicAppListener::NewL(CMobblerLastFmConnection& aSubmitter)
	{
	CMobblerMusicAppListener* self(new(ELeave) CMobblerMusicAppListener(aSubmitter));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppListener::CMobblerMusicAppListener(CMobblerLastFmConnection& aSubmitter)
	:iLastFmConnection(aSubmitter)
	{
	}

void CMobblerMusicAppListener::ConstructL()
	{
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
	
	TRAP_IGNORE(iMobblerContentListing = static_cast<CMobblerContentListingInterface*>(REComSession::CreateImplementationL(KContentListingImplUid, iDtorIdKey)));
	if (iMobblerContentListing)
		{
		iMobblerContentListing->SetObserver(*this);
		}
	
	iMusicPlayerState = EPlayerNotRunning;
	
	// check if there is a song playing when Mobbler is started
	ScheduleNowPlayingL();
	}

CMobblerMusicAppListener::~CMobblerMusicAppListener()
	{
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
	
	if (iMobblerContentListing)
		{
		delete iMobblerContentListing;
		REComSession::DestroyedImplementation(iDtorIdKey);
		}
	
	iObservers.Close();
	}

void CMobblerMusicAppListener::AddObserverL(MMobblerMusicAppListenerObserver* aObserver)
	{
	iObservers.InsertInAddressOrderL(aObserver);
	}

void CMobblerMusicAppListener::RemoveObserver(MMobblerMusicAppListenerObserver* aObserver)
	{
	TInt position(iObservers.FindInAddressOrder(aObserver));
	
	if (position != KErrNotFound)
		{
		iObservers.Remove(position);
		}
	}

void CMobblerMusicAppListener::NotifyChangeL()
	{
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleMusicAppChangeL();
		}
	}

CMobblerTrack* CMobblerMusicAppListener::CurrentTrack()
	{
	return iCurrentTrack;
	}

void CMobblerMusicAppListener::HandleTrackChangeL(const TDesC& /*aTrack*/)
	{
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
	if (static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->
												RadioPlayer().CurrentTrack())
		{
		static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->
												RadioPlayer().StopL();
		}

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
		for (TInt i(0) ; i < KMusicAppCount ; ++i)
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
					iCurrentTrack = CMobblerTrack::NewL(*artist, *title, KNullDesC8, /*KNullDesC8,*/ KNullDesC8, KNullDesC8, KNullDesC8, trackLength, KNullDesC8);
					
					if (trackTitle.Length() != 0 && trackArtist.Length() != 0)
						{
						iMobblerContentListing->FindAndSetAlbumNameL(trackArtist, trackTitle);
						}
					}
				else
					{
					HBufC8* album(CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackAlbum));
					CleanupStack::PushL(album);
					iCurrentTrack = CMobblerTrack::NewL(*artist, *title, *album, /*KNullDesC8,*/ KNullDesC8, KNullDesC8, KNullDesC8, trackLength, KNullDesC8);
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

void CMobblerMusicAppListener::SetAlbumL(const TDesC& aAlbum)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->SetAlbumL(aAlbum);
		}
	}

void CMobblerMusicAppListener::SetPathL(const TDesC& aPath)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->SetPathL(aPath);
		}
	}

void CMobblerMusicAppListener::SetTrackNumber(const TInt aTrackNumber)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->SetTrackNumber(aTrackNumber);
		}
	}

void CMobblerMusicAppListener::PlayerStateChangedL(TMobblerMusicAppObserverState aState)
	{
	TMobblerMusicAppObserverState oldState(iMusicPlayerState);
	TMobblerMusicAppObserverState newState(aState);
	iMusicPlayerState = newState;
	
	if ((oldState != EPlayerPlaying) && 
		(newState == EPlayerPlaying))
		{
		// Set start time = now
		if (iCurrentTrack)
			{
			TTime startTimeUTC;
			startTimeUTC.UniversalTime();
			iCurrentTrack->SetStartTimeUTC(startTimeUTC);
			}
		ScheduleNowPlayingL();
		}
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
		else
			{
			iLastFmConnection.TrackStoppedL(iCurrentTrack);
			}
		}
	
	NotifyChangeL();
	}

void CMobblerMusicAppListener::TrackInfoChangedL(const TDesC& /*aTitle*/, const TDesC& /*aArtist*/)
	{
	iLastFmConnection.TrackStoppedL(iCurrentTrack);
	
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		iCurrentTrack = NULL;
		}
	
	ScheduleNowPlayingL();
	}

void CMobblerMusicAppListener::CommandReceivedL(TMobblerMusicAppObserverCommand aCommand)
	{
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
	return (iCurrentTrack && iMusicPlayerState == EPlayerPlaying);
	}

// End of file
