/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey

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

#include <ecom/implementationproxy.h>

#include "mobblermusicapp30.h"

const TUid KMusicAppUid = {0x102072c3};

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x20039AF8}; 
#else
const TInt KImplementationUid = {0xA0007CAF}; 
#endif

const TInt KTrackTitleKey(2);
const TInt KStateKey(3);
const TInt KTrackArtistKey(5);
//const TInt KPlaybackPositionKey(6);
const TInt KTrackLengthKey(7);
//const TInt KPlaylistPositionKey(8);
//const TInt KCommandKey(12);

_LIT(KMusicPlayer, "Music Player");

const TImplementationProxy ImplementationTable[] =
	{
	{{KImplementationUid}, TProxyNewLPtr(CMobblerMusicAppObserver30::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerMusicAppObserver30* CMobblerMusicAppObserver30::NewL(TAny* aObserver)
	{
	CMobblerMusicAppObserver30* self(new(ELeave) CMobblerMusicAppObserver30(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppObserver30::CMobblerMusicAppObserver30(TAny* aObserver)
	:iObserver(static_cast<MMobblerMusicAppObserver*>(aObserver))
	{
	}

void CMobblerMusicAppObserver30::ConstructL()
	{
	iMusicAppStateListener = CMobblerMusicAppStateListener::NewL(*this);
	iPlaybackPositionListener = CMobblerPlaybackPositionListener::NewL(*this);
	iTrackListener = CMobblerTrackListener::NewL(*this);
	}

CMobblerMusicAppObserver30::~CMobblerMusicAppObserver30()
	{
	delete iMusicAppStateListener;
	delete iPlaybackPositionListener;
	delete iTrackListener;
	}

void CMobblerMusicAppObserver30::HandleMusicStateChangeL(TInt aMPlayerState)
	{
	PlayerStateChanged(static_cast<TMPlayerRemoteControlState>(aMPlayerState));
	}

void CMobblerMusicAppObserver30::HandleTrackChangeL(const TDesC& aTrack)
	{
	TrackInfoChanged(aTrack, aTrack);
	}

void CMobblerMusicAppObserver30::HandlePlaybackPositionChangeL(TTimeIntervalSeconds aPlaybackPosition)
	{
	iObserver->PlayerPositionL(aPlaybackPosition);
	}

void CMobblerMusicAppObserver30::CommandReceived(TMPlayerRemoteControlCommands aCmd)
	{
	iObserver->CommandReceivedL(ConvertCommand(aCmd));
	}

void CMobblerMusicAppObserver30::PlayerStateChanged(TMPlayerRemoteControlState aState)
	{
	iObserver->PlayerStateChangedL(ConvertState(aState));
	}

void CMobblerMusicAppObserver30::TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist)
	{
	iObserver->TrackInfoChangedL(aTitle, aArtist);
	}

TMobblerMusicAppObserverState CMobblerMusicAppObserver30::PlayerState()
	{
	TInt playerState(EMPlayerRCtrlNotRunning);
	RProperty::Get(KMusicAppUid, KStateKey, playerState);
	return ConvertState(static_cast<TMPlayerRemoteControlState>(playerState));
	}

HBufC* CMobblerMusicAppObserver30::NameL()
	{
	return KMusicPlayer().AllocL();
	}
	
const TDesC& CMobblerMusicAppObserver30::Title()
	{
	RProperty::Get(KMusicAppUid, KTrackTitleKey, iTitle);
	return iTitle;
	}

const TDesC& CMobblerMusicAppObserver30::Artist()
	{
	RProperty::Get(KMusicAppUid, KTrackArtistKey, iArtist);
	return iArtist;
	}

const TDesC& CMobblerMusicAppObserver30::Album()
	{
	return KNullDesC;
	}

TTimeIntervalSeconds CMobblerMusicAppObserver30::Duration()
	{
	TInt duration;
	RProperty::Get(KMusicAppUid, KTrackLengthKey, duration);
	return duration;
	}

TMobblerMusicAppObserverState CMobblerMusicAppObserver30::ConvertState(TMPlayerRemoteControlState aState)
	{
	TMobblerMusicAppObserverState playerState(EPlayerNotRunning);
	
	switch (aState)
		{
		case EMPlayerRCtrlNotRunning:
			playerState = EPlayerNotRunning;
			break;
		case EMPlayerRCtrlNotInitialised:
			playerState = EPlayerNotInitialised;
			break;
		case EMPlayerRCtrlInitialising:
			playerState = EPlayerInitialising;
			break;
		case EMPlayerRCtrlStopped:
			playerState = EPlayerStopped;
			break;
		case EMPlayerRCtrlPlaying:
			playerState = EPlayerPlaying;
			break;
		case EMPlayerRCtrlPaused:
			playerState = EPlayerPaused;
			break;
		case EMPlayerRCtrlSeekingForward:
			playerState = EPlayerSeekingForward;
			break;
		case EMPlayerRCtrlSeekingBackward:
			playerState = EPlayerSeekingBackward;
			break;
		}
	
	return playerState;
	}

TMobblerMusicAppObserverCommand CMobblerMusicAppObserver30::ConvertCommand(TMPlayerRemoteControlCommands aCommand)
	{
	TMobblerMusicAppObserverCommand command(EPlayerCmdNoCommand);
	
	switch (aCommand)
		{
		case EMPlayerRCtrlCmdNoCommand:
			command = EPlayerCmdNoCommand;
			break;
		case EMPlayerRCtrlCmdPlay:
			command = EPlayerCmdPlay;
			break;
		case EMPlayerRCtrlCmdPause:
			command = EPlayerCmdPause;
			break;
		case EMPlayerRCtrlCmdStop:
			command = EPlayerCmdStop;
			break;
		case EMPlayerRCtrlCmdStartSeekForward:
			command = EPlayerCmdStartSeekForward;
			break;
		case EMPlayerRCtrlCmdStartSeekBackward:
			command = EPlayerCmdStartSeekBackward;
			break;
		case EMPlayerRCtrlCmdStopSeeking:
			command = EPlayerCmdStopSeeking;
			break;
		case EMPlayerRCtrlCmdNextTrack:
			command = EPlayerCmdNextTrack;
			break;
		case EMPlayerRCtrlCmdPreviousTrack:
			command = EPlayerCmdPreviousTrack;
			break;
		case EMPlayerRCtrlCmdStartMusicPlayer:
			command = EPlayerCmdStartMusicPlayer;
			break;
		case EMPlayerRCtrlCmdCloseMusicPlayer:
			command = EPlayerCmdCloseMusicPlayer;
			break;
		case EMPlayerRCtrlCmdBack:
			command = EPlayerCmdBack;
			break;
		case EMPlayerRCtrlCmdPlayPause:
			command = EPlayerCmdPlayPause;
			break;
		}
	
	return command;
	}

TBool CMobblerMusicAppObserver30::ControlsSupported()
	{
	return EFalse;
	}

void CMobblerMusicAppObserver30::PlayL()
	{
	
	}

void CMobblerMusicAppObserver30::StopL()
	{
	
	}

void CMobblerMusicAppObserver30::SkipL()
	{
	
	}

// End of file
