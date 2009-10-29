/*
musicappobservermpx.h

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#ifndef __MUSICAPPOBSERVERMPX_H__
#define __MUSICAPPOBSERVERMPX_H__

#include <e32base.h>
#include <mobbler\mobblermusicapp.h>
#include <mpxplaybackutility.h>
#include <mpxplaybackobserver.h>

class CMobblerMusicAppObserverMPX : public CMobblerMusicApp, public MMPXPlaybackObserver, public MMPXPlaybackCallback
	{
public:
	static CMobblerMusicAppObserverMPX* NewL(TAny* aObserver);
	~CMobblerMusicAppObserverMPX();
	
private:
	CMobblerMusicAppObserverMPX(TAny* aObserver);
	void ConstructL();
	
	void RequestMediaL();
	TMPlayerRemoteControlState MPlayerState(TMPXPlaybackState aState);
	
private: // from MMobblerMusicApp
	HBufC* NameL();
	TMPlayerRemoteControlState PlayerState();
	const TDesC& Title();
	const TDesC& Artist();
	const TDesC& Album();
	TTimeIntervalSeconds Duration();
	
private:
	void HandlePlaybackMessage(CMPXMessage* aMessage, TInt aError);
	
private:
	void HandlePropertyL(TMPXPlaybackProperty aProperty, TInt aValue, TInt aError);
	void HandleSubPlayerNamesL(TUid aPlayer, const MDesCArray* aSubPlayers, TBool aComplete, TInt aError);
	void HandleMediaL(const CMPXMedia& aMedia, TInt aError);
	void HandlePlaybackCommandComplete(CMPXCommand* aCommandResult, TInt aError);
	
private: // from MMPlayerCommandObserver
	void CommandReceived(TMPlayerRemoteControlCommands aCmd);
	
private: // from MMPlayerPlaybackObserver
	void PlayerStateChanged(TMPlayerRemoteControlState aState);
	void TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist);
	void PlaylistChanged();
	void PlaybackPositionChanged(TInt aPosition);
	void EqualizerPresetChanged(TInt aPresetNameKey); 
	void PlaybackModeChanged(TBool aRandom, TMPlayerRepeatMode aRepeat); 
	void PlayerUidChanged(TInt aPlayerUid);
	void VolumeChanged(TInt aVolume); 
	
private:
	MMobblerMusicAppObserver* iObserver;
	TApaAppCaption iName;
	
	TInt iPosition;
	TInt iDuration;
	
	HBufC* iArtist;
	HBufC* iTitle;
	
	MMPXPlaybackUtility* iPlaybackUtility;
	};

#endif // __MUSICAPPOBSERVER_H__

// End of file
