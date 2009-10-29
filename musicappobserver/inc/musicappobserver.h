/*
musicappobserver.h

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

#ifndef __MUSICAPPOBSERVER_H__
#define __MUSICAPPOBSERVER_H__

#include <e32base.h>
#include <mobbler\mobblermusicapp.h>
#include <mplayerremotecontrol.h>

class CMobblerMusicAppObserver : public CMobblerMusicApp, public MMPlayerPlaybackObserver, public MMPlayerCommandObserver
	{
public:
	static CMobblerMusicAppObserver* NewL(TAny* aObserver);
	~CMobblerMusicAppObserver();
	
private:
	CMobblerMusicAppObserver(TAny* aObserver);
	void ConstructL();
	
private: // from MMobblerMusicApp
	HBufC* NameL();
	TMPlayerRemoteControlState PlayerState();
	const TDesC& Title();
	const TDesC& Artist();
	const TDesC& Album();
	TTimeIntervalSeconds Duration();
	
private: // from MMPlayerCommandObserver
	void CommandReceived(TMPlayerRemoteControlCommands aCmd);
		
private: // from MMPlayerPlaybackObserver
	void PlayerStateChanged(TMPlayerRemoteControlState aState);
	void TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist);
	void PlaylistChanged();
	void PlaybackPositionChanged(TInt aPosition);
	void EqualizerPresetChanged(TInt aPresetNameKey); 
	void PlaybackModeChanged(TBool aRandom, TMPlayerRepeatMode aRepeat); 
	void PlayerUidChanged(TInt aPlayerUid );   
    void VolumeChanged(TInt aVolume); 
	
private:
	MMPlayerRemoteControl* iEngine;
	MMobblerMusicAppObserver* iObserver;
	TApaAppCaption iName;
	};

#endif // __MUSICAPPOBSERVER_H__
