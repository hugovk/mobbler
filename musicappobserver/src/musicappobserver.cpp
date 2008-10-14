/*
musicappobserver.cpp

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

#include <ecom/implementationproxy.h>
#include "musicappobserver.h"

const TImplementationProxy ImplementationTable[] =
    {
    {{0xA0007CAC}, TProxyNewLPtr(CMobblerMusicAppObserver::NewL)}
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
    }

CMobblerMusicAppObserver* CMobblerMusicAppObserver::NewL(TAny* aObserver)
	{
	CMobblerMusicAppObserver* self = new(ELeave) CMobblerMusicAppObserver(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CMobblerMusicAppObserver::CMobblerMusicAppObserver(TAny* aObserver)
	:iObserver(static_cast<MMobblerMusicAppObserver*>(aObserver))
	{
	}
	
void CMobblerMusicAppObserver::ConstructL()
	{
#ifndef __WINS__
	iEngine = MPlayerRemoteControlFactory::NewRemoteControlL();	
	iEngine->RegisterObserver(this);
	iEngine->RegisterCommandObserver(this);
#endif
	}
	
CMobblerMusicAppObserver::~CMobblerMusicAppObserver()
	{
#ifndef __WINS__
	iEngine->UnregisterObserver(this);
	iEngine->UnregisterCommandObserver(this);
#endif
	}


void CMobblerMusicAppObserver::CommandReceived(TMPlayerRemoteControlCommands aCmd)
	{
	iObserver->CommandReceivedL(aCmd);
	}

void CMobblerMusicAppObserver::PlayerStateChanged(TMPlayerRemoteControlState aState)
	{
	iObserver->PlayerStateChangedL(aState);
	}
	
void CMobblerMusicAppObserver::TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist)
	{
	iObserver->TrackInfoChangedL(aTitle, aArtist);
	}
	
void CMobblerMusicAppObserver::PlaylistChanged()
	{
	}
	
void CMobblerMusicAppObserver::PlaybackPositionChanged(TInt aPosition)
	{
	iObserver->PlayerPositionL(aPosition);
	}
	
void CMobblerMusicAppObserver::EqualizerPresetChanged(TInt /*aPresetNameKey*/)
	{
	}
	
void CMobblerMusicAppObserver::PlaybackModeChanged(TBool /*aRandom*/, TMPlayerRepeatMode /*aRepeat*/)
	{
	}
	
void CMobblerMusicAppObserver::PlayerUidChanged(TInt /*aPlayerUid*/ )
	{
	}
	
void CMobblerMusicAppObserver::VolumeChanged(TInt /*aVolume*/)
	{
	}

HBufC* CMobblerMusicAppObserver::NameL()
	{
	return _L("Music Player").AllocL();
	}

TMPlayerRemoteControlState CMobblerMusicAppObserver::PlayerState()
	{
#ifndef __WINS__
	return iEngine->PlayerState();
#else
	return EMPlayerRCtrlNotRunning;
#endif
	}
const TDesC& CMobblerMusicAppObserver::Title()
	{
#ifndef __WINS__
	return iEngine->Title();
#else
	return KNullDesC;
#endif
	}

const TDesC& CMobblerMusicAppObserver::Artist()
	{
#ifndef __WINS__
	return iEngine->Artist();
#else
	return KNullDesC;
#endif
	}

const TDesC& CMobblerMusicAppObserver::Album()
	{
	return KNullDesC;
	}

TTimeIntervalSeconds CMobblerMusicAppObserver::Duration()
	{
#ifndef __WINS__
	return iEngine->Duration();
#else
	return 0;
#endif
	}
	