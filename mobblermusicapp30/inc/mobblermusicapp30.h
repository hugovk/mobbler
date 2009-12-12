/*
mobblermusicapp30.h

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

#ifndef __MOBBLERMUSICAPP30_H__
#define __MOBBLERMUSICAPP30_H__

#include <e32base.h>
#include <mplayerremotecontrol.h>
#include <mobbler\mobblermusicapp.h>

#include "mobblermusicstatelistener.h"
#include "mobblertracklistener.h"
#include "mobblerplaybackpositionlistener.h"

class CMobblerMusicAppObserver30 : public CMobblerMusicApp,
									public CMobblerTrackListener::MMobblerTrackObserver,
									public CMobblerMusicAppStateListener::MMobblerMusicAppStateObserver,
									public CMobblerPlaybackPositionListener::MMobblerPlaybackPositionObserver
	{
public:
	static CMobblerMusicAppObserver30* NewL(TAny* aObserver);
	~CMobblerMusicAppObserver30();
	
private:
	CMobblerMusicAppObserver30(TAny* aObserver);
	void ConstructL();
	
	TMobblerMusicAppObserverState ConvertState(TMPlayerRemoteControlState aState);
	TMobblerMusicAppObserverCommand ConvertCommand(TMPlayerRemoteControlCommands aCommand);
	
private:
	void HandleMusicStateChangeL(TInt aMPlayerState);
	void HandleTrackChangeL(const TDesC& aTrack);
	void HandlePlaybackPositionChangeL(TTimeIntervalSeconds aPlaybackPosition);
	
private: // from MMobblerMusicApp
	HBufC* NameL();
	TMobblerMusicAppObserverState PlayerState();
	const TDesC& Title();
	const TDesC& Artist();
	const TDesC& Album();
	TTimeIntervalSeconds Duration();
	
private:
	void CommandReceived(TMPlayerRemoteControlCommands aCmd);
	void PlayerStateChanged(TMPlayerRemoteControlState aState);
	void TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist);
	
private:
	CMobblerMusicAppStateListener* iMusicAppStateListener;
	CMobblerTrackListener* iTrackListener;
	
	TBuf<255> iTitle;
	TBuf<255> iArtist;
	
	MMobblerMusicAppObserver* iObserver;
	};

#endif // __MOBBLERMUSICAPP30_H__
