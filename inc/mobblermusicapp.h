/*
mobblermusicapp.h

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

#ifndef __MOBBLERMUSICAPP_H__
#define __MOBBLERMUSICAPP_H__

#include <mplayerremotecontrol.h>

class MMobblerMusicAppObserver
	{
public:
	virtual void PlayerStateChangedL(TMPlayerRemoteControlState aState) = 0;
	virtual void TrackInfoChangedL(const TDesC& aTitle, const TDesC& aArtist) = 0;
	virtual void CommandReceivedL(TMPlayerRemoteControlCommands aCommand) = 0;
	virtual void PlayerPositionL(TTimeIntervalSeconds aPlayerPosition) = 0;
	};

class CMobblerMusicApp : public CBase
	{
public:
	virtual HBufC* NameL() = 0;
	virtual TMPlayerRemoteControlState PlayerState() = 0;
	virtual const TDesC& Title() = 0;
	virtual const TDesC& Artist() = 0;
	virtual const TDesC& Album() = 0;
	virtual TTimeIntervalSeconds Duration() = 0;
	};

#endif // __MOBBLERMUSICAPP_H__
