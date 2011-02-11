/*
musicappobservermpx.cpp

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

#include <apaid.h>
#include <apgcli.h>
#include <ecom/implementationproxy.h>
#include <mpxmedia.h>
#include <mpxmediageneraldefs.h>
#include <mpxmediamusicdefs.h>
#include <mpxmessage2.h>
#include <mpxmessagegeneraldefs.h>
#include <mpxplaybackmessage.h>
#include <mpxplaybackutility.h>

#include "musicappobservermpx.h"


const TUid KMusicAppUID = {0x102072C3};

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x20039B0A}; 
#else
const TInt KImplementationUid = {0xA000D9F5}; 
#endif

const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerMusicAppObserverMPX::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerMusicAppObserverMPX* CMobblerMusicAppObserverMPX::NewL(TAny* aObserver)
	{
	CMobblerMusicAppObserverMPX* self(new(ELeave) CMobblerMusicAppObserverMPX(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppObserverMPX::CMobblerMusicAppObserverMPX(TAny* aObserver)
	:iObserver(static_cast<MMobblerMusicAppObserver*>(aObserver))
	{
	}

void CMobblerMusicAppObserverMPX::ConstructL()
	{
#ifndef __WINS__
	iPlaybackUtility = MMPXPlaybackUtility::NewL(KPbModeActivePlayer, this);
#endif

	RApaLsSession apaLsSession;
	User::LeaveIfError(apaLsSession.Connect());
	CleanupClosePushL(apaLsSession);

	TApaAppInfo info;
	User::LeaveIfError(apaLsSession.GetAppInfo(info, KMusicAppUID));
	CleanupStack::PopAndDestroy(); // apaLsSession

	// The caption is the app's name (e.g. EN: "Music player" or FI: "Soitin")
	iName = info.iCaption;
	
	iTitle = KNullDesC().AllocL();
	iArtist = KNullDesC().AllocL();
	
	if(iPlaybackUtility &&
			iPlaybackUtility->StateL() != EPbStateNotInitialised &&
			iPlaybackUtility->StateL() != EPbStateInitialising)
		{
		// Playback is already ongoing. We aren't going to receive EMediaChanged
		// for the current song so we need manually update the media info   
		RequestMediaL();
		}
	}

CMobblerMusicAppObserverMPX::~CMobblerMusicAppObserverMPX()
	{
	delete iArtist;
	delete iTitle;
	
#ifndef __WINS__
	if (iPlaybackUtility)
		{
		iPlaybackUtility->Close();
		}
#endif
	}

#ifdef __WINS__
void CMobblerMusicAppObserverMPX::HandlePlaybackMessage(CMPXMessage* /*aMessage*/, TInt /*aError*/)
	{
	}
#else
void CMobblerMusicAppObserverMPX::HandlePlaybackMessage(CMPXMessage* aMessage, TInt aError)
	{
	TMPXMessageId id(aMessage->ValueTObjectL<TMPXMessageId>(KMPXMessageGeneralId));
	 
	if (KMPXMessageGeneral == id)
		{
		TInt event(aMessage->ValueTObjectL<TInt>(KMPXMessageGeneralEvent));
		
		switch (event)
			{
			case TMPXPlaybackMessage::EPropertyChanged:
				{
				TInt error(KErrNone);
				
				HandlePropertyL(aMessage->ValueTObjectL<TMPXPlaybackProperty>(KMPXMessageGeneralType), aMessage->ValueTObjectL<TInt>(KMPXMessageGeneralData), error);
				break;
				}
			case TMPXPlaybackMessage::EStateChanged:
				{
				TMobblerMusicAppObserverState state(MPlayerState(aMessage->ValueTObjectL<TMPXPlaybackState>(KMPXMessageGeneralType)));
				
				iObserver->PlayerStateChangedL(state);
				break;
				}
			case TMPXPlaybackMessage::EMediaChanged:
			case TMPXPlaybackMessage::EPlaylistUpdated:
				{
				iPlaybackUtility->PropertyL(*this, EPbPropertyPosition);
				iPlaybackUtility->PropertyL(*this, EPbPropertyDuration);
				
				RequestMediaL();
				break;
				}
			case TMPXPlaybackMessage::EActivePlayerChanged:
				{
				iPlaybackUtility->PropertyL(*this, EPbPropertyPosition);
				iPlaybackUtility->PropertyL(*this, EPbPropertyDuration);
				//iPlaybackUtility->PropertyL(*this, EPbPropertyMaxVolume);
				//iPlaybackUtility->PropertyL(*this, EPbPropertyVolume);
				
				iObserver->PlayerStateChangedL(MPlayerState(iPlaybackUtility->StateL()));
				
				RequestMediaL();
				break;
				}
			default:
				{
				break;
				}
			}
		}
	}
#endif // __WINS__
TMobblerMusicAppObserverState CMobblerMusicAppObserverMPX::MPlayerState(TMPXPlaybackState aState)
	{
	TMobblerMusicAppObserverState state(EPlayerNotRunning);
	
	switch (aState)
		{
		case EPbStateNotInitialised:
			state = EPlayerNotInitialised;
			break;
		case EPbStateInitialising:
			state = EPlayerInitialising;
			break;
		case EPbStatePlaying:
			state = EPlayerPlaying;
			break;
		case EPbStatePaused:
			state = EPlayerPaused;
			break;
		case EPbStateStopped:
			state = EPlayerStopped;
			break;
		case EPbStateSeekingForward:
			state = EPlayerSeekingForward;
			break;
		case EPbStateSeekingBackward:
			state = EPlayerSeekingBackward;
			break;
		case EPbStateShuttingDown:
			state = EPlayerNotRunning;
			break;
		case EPbStateBuffering:
			break;
		case EPbStateDownloading:
			break;
		}
	
	return state;
	}

void CMobblerMusicAppObserverMPX::HandlePropertyL(TMPXPlaybackProperty aProperty, TInt aValue, TInt aError)
	{
	if (KErrNone == aError)
		{
		switch (aProperty)
			{
			case EPbPropertyPosition:
				{
				iPosition = aValue / 1000;
				iObserver->PlayerPositionL(iPosition);
				break;
				}
			case EPbPropertyDuration:
				{
				iDuration = aValue / 1000;
				break;
				}
			default:
				{
				break;
				}
			}
		}
	}

void CMobblerMusicAppObserverMPX::HandleSubPlayerNamesL(TUid /*aPlayer*/, const MDesCArray* /*aSubPlayers*/, TBool /*aComplete*/, TInt /*aError*/)
	{
	
	}


void CMobblerMusicAppObserverMPX::RequestMediaL()
	{
#ifndef __WINS__
	MMPXSource* s(iPlaybackUtility->Source());
	if (s)
		{
		RArray<TMPXAttribute> attrs;
		CleanupClosePushL(attrs);
		attrs.Append(KMPXMediaGeneralUri);
		attrs.Append(KMPXMediaGeneralTitle);
		attrs.Append(KMPXMediaMusicArtist);
		s->MediaL(attrs.Array(), *this);
		CleanupStack::PopAndDestroy(&attrs);
		}
#endif
	}

#ifdef __WINS__
void CMobblerMusicAppObserverMPX::HandleMediaL(const CMPXMedia& /*aMedia*/, TInt /*aError*/)
	{
	}
#else
void CMobblerMusicAppObserverMPX::HandleMediaL(const CMPXMedia& aMedia, TInt aError)
	{
	if (KErrNone == aError)
		{
		delete iTitle;
		iTitle = NULL;
		if (aMedia.IsSupported(KMPXMediaGeneralTitle))
			{
			iTitle = (aMedia.ValueText(KMPXMediaGeneralTitle)).AllocL();
			}
		else if (aMedia.IsSupported(KMPXMediaGeneralUri))
			{
			TParsePtrC filePath(aMedia.ValueText(KMPXMediaGeneralUri));
			iTitle = (filePath.Name()).AllocL();
			}
		delete iArtist;
		iArtist = NULL;
		iArtist = (aMedia.ValueText(KMPXMediaMusicArtist)).AllocL();
		
		iObserver->TrackInfoChangedL(*iTitle, *iArtist);
		}
	}
#endif // __ WINS__

void CMobblerMusicAppObserverMPX::HandlePlaybackCommandComplete(CMPXCommand* /*aCommandResult*/, TInt /*aError*/)
	{
	
	}

HBufC* CMobblerMusicAppObserverMPX::NameL()
	{
	return iName.AllocL();
	}

TMobblerMusicAppObserverState CMobblerMusicAppObserverMPX::PlayerState()
	{
#ifndef __WINS__
	return MPlayerState(iPlaybackUtility->StateL());
#else
	return EPlayerNotRunning;
#endif
	}

const TDesC& CMobblerMusicAppObserverMPX::Title()
	{
	return *iTitle; 
	}

const TDesC& CMobblerMusicAppObserverMPX::Artist()
	{
	return *iArtist; 
	}

const TDesC& CMobblerMusicAppObserverMPX::Album()
	{
	return KNullDesC;
	}

TTimeIntervalSeconds CMobblerMusicAppObserverMPX::Duration()
	{
	return iDuration;
	}

TBool CMobblerMusicAppObserverMPX::ControlsSupported()
	{
#ifndef __WINS__
	return ETrue;
#else
	return EFalse;
#endif
	}

void CMobblerMusicAppObserverMPX::PlayL()
	{
#ifndef __WINS__
	iPlaybackUtility->CommandL(EPbCmdPlay);
#endif
	}

void CMobblerMusicAppObserverMPX::StopL()
	{
#ifndef __WINS__
	iPlaybackUtility->CommandL(EPbCmdStop);
#endif
	}

void CMobblerMusicAppObserverMPX::SkipL()
	{
#ifndef __WINS__
	iPlaybackUtility->CommandL(EPbCmdNext);
#endif
	}

// End of file
