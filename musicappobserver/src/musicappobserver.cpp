/*
musicappobserver.cpp

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

#include <apgcli.h>
#include <ecom/implementationproxy.h>
#include "musicappobserver.h"

const TUid KMusicAppUID = {0x102072C3};

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x2002655F}; 
#else
const TInt KImplementationUid = {0xA0007CAC}; 
#endif

const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerMusicAppObserver::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerMusicAppObserver* CMobblerMusicAppObserver::NewL(TAny* aObserver)
	{
	CMobblerMusicAppObserver* self(new(ELeave) CMobblerMusicAppObserver(aObserver));
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

	RApaLsSession apaLsSession;
	User::LeaveIfError(apaLsSession.Connect());
	CleanupClosePushL(apaLsSession);

	TApaAppInfo info;
	User::LeaveIfError(apaLsSession.GetAppInfo(info, KMusicAppUID));
	CleanupStack::PopAndDestroy(); // apaLsSession

	// The caption is the app's name (e.g. EN: "Music player" or FI: "Soitin")
	iName = info.iCaption;
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
	TRAP_IGNORE(iObserver->CommandReceivedL(aCmd));
	}

void CMobblerMusicAppObserver::PlayerStateChanged(TMPlayerRemoteControlState aState)
	{
	TRAP_IGNORE(iObserver->PlayerStateChangedL(aState));
	}

void CMobblerMusicAppObserver::TrackInfoChanged(const TDesC& aTitle, const TDesC& aArtist)
	{
	TRAP_IGNORE(iObserver->TrackInfoChangedL(aTitle, aArtist));
	}

void CMobblerMusicAppObserver::PlaylistChanged()
	{
	}

void CMobblerMusicAppObserver::PlaybackPositionChanged(TInt aPosition)
	{
	TRAP_IGNORE(iObserver->PlayerPositionL(aPosition));
	}

void CMobblerMusicAppObserver::EqualizerPresetChanged(TInt /*aPresetNameKey*/)
	{
	}

void CMobblerMusicAppObserver::PlaybackModeChanged(TBool /*aRandom*/, TMPlayerRepeatMode /*aRepeat*/)
	{
	}

void CMobblerMusicAppObserver::PlayerUidChanged(TInt /*aPlayerUid*/)
	{
	}

void CMobblerMusicAppObserver::VolumeChanged(TInt /*aVolume*/)
	{
	}

HBufC* CMobblerMusicAppObserver::NameL()
	{
	return iName.AllocL();
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

// End of file
