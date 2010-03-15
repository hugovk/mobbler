/*
mobblermusiclistener.h

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

#ifndef __MOBBLERMUSICLISTENER_H__
#define __MOBBLERMUSICLISTENER_H__

#include <e32base.h>

#include "mobblerlastfmconnection.h"
#include "mobblermusicapp.h"

class CMobblerNowPlayingCallback;
class CMobblerRadioPlayer;

class MMobblerMusicAppListenerObserver
	{
public:
	virtual void HandleMusicAppChangeL() = 0;
	};

class CMobblerMusicAppListener : public CBase, public MMobblerMusicAppObserver
	{
public:
	static CMobblerMusicAppListener* NewL(CMobblerLastFmConnection& aSubmitter);
	~CMobblerMusicAppListener();
	
	void AddObserverL(MMobblerMusicAppListenerObserver* aObserver);
	void RemoveObserver(MMobblerMusicAppListenerObserver* aObserver);
	
	CMobblerTrack* CurrentTrack();
	void NowPlayingL();
	HBufC* MusicAppNameL() const;
	TBool IsPlaying() const;
	
	TBool ControlsSupported();
	void PlayL();
	void StopL();
	void SkipL();
	
private:
	CMobblerMusicAppListener(CMobblerLastFmConnection& aSubmitter);
	void ConstructL();
	
	void NotifyChangeL();
	
private: 
	void HandleTrackChangeL(const TDesC& aTrack);
	void HandleMusicStateChangeL(TInt aState);
	
	void ScheduleNowPlayingL();
	
private: // from MMobblerMusicAppObserver
	void PlayerStateChangedL(TMobblerMusicAppObserverState aState);
	void TrackInfoChangedL(const TDesC& aTitle, const TDesC& aArtist);
	void CommandReceivedL(TMobblerMusicAppObserverCommand aCommand);
	void PlayerPositionL(TTimeIntervalSeconds aPlayerPosition);
    
private:
	// Music app observer
	RPointerArray<CMobblerMusicApp> iMobblerMusicApps;
	RArray<TUid> iDtorIdKeys;

	CMobblerLastFmConnection& iLastFmConnection;
	
	CMobblerNowPlayingCallback* iNowPlayingCallback;
	
	CMobblerTrack* iCurrentTrack;

	TMobblerMusicAppObserverState iMusicPlayerState;
	
	RPointerArray<MMobblerMusicAppListenerObserver> iObservers;
	};

#endif // __MOBBLERMUSICLISTENER_H__

// End of file
