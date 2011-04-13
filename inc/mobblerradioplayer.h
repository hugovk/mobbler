/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009  Steve Punter
Copyright (C) 2009  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade

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

#ifndef __MOBBLERRADIOPLAYER_H__
#define __MOBBLERRADIOPLAYER_H__

#include "mobbleraudiocontrol.h"
#include "mobblerincomingcallmonitorobserver.h"

class CMobblerIncomingCallMonitor;
class CMobblerRadioPlaylist;
class CMobblerString;

class MMobblerRadioStateChangeObserver
	{
public:
	virtual void HandleRadioStateChangedL() = 0;
	};

class CMobblerRadioPlayer : public CActive,
							public MMobblerIncomingCallMonitorObserver,
							public MMobblerAudioControlObserver,
							public MMobblerFlatDataObserver,
							public MMobblerConnectionStateObserver 

	{
public:
	enum TState
		{
		EIdle,
		EStarting,
		EPlaying,
		EPaused
		};
	
	enum TTransactionState
		{
		ENone,
		EFetchingPlaylist,
		ESelectingStation
		};
	
public:
	static CMobblerRadioPlayer* NewL(CMobblerLastFmConnection& aLastFmConnection,
										TTimeIntervalSeconds aPreBufferSize, 
										TInt aEqualizerIndex, 
										TInt aVolume,
										TInt aBitRate);
	~CMobblerRadioPlayer();
	
	void AddObserverL(MMobblerRadioStateChangeObserver* aObserver);
	void RemoveObserver(MMobblerRadioStateChangeObserver* aObserver);
	
	void StartL(CMobblerLastFmConnection::TRadioStation aRadioStation, const CMobblerString* aRadioText);
	
	CMobblerTrack* CurrentTrack();
	CMobblerTrack* NextTrack();
	
	TState State() const;
	TTransactionState TransactionState() const;

	void SkipTrackL();
	void StopL();
	void PauseL();
	
	void VolumeUp();
	void VolumeDown();
	void SetVolume(TInt aVolume);
	void SetEqualizer(TInt aIndex);
	void SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize);
	void SetBitRateL(TInt aBitRate);
	
	TInt Volume() const;
	TInt MaxVolume() const;
	TInt EqualizerIndex() const;
	const CMobblerString& Station() const;
	TBool HasPlaylist() const;
	
private: // from MMobblerAudioControlObserver
	void HandleAudioPositionChangeL();
	void HandleAudioFinishedL(CMobblerAudioControl* aAudioControl, 
								TBool aAbnormalTermination);
	void HandleAudioTryAgainL(CMobblerAudioControl* aAudioControl);

private: // from MMobblerConnectionStateObserver
	void HandleConnectionStateChangedL();
	
private:
	CMobblerRadioPlayer(CMobblerLastFmConnection& aSubmitter, 
							TTimeIntervalSeconds aPreBufferSize, 
							TInt aEqualizerIndex, 
							TInt aVolume,
							TInt aBitRate);
	void ConstructL();
	
	void SubmitCurrentTrackL();
	
	void DoStopL(TBool aFullStop);
	
	void DoChangeStateL(TState aState);
	void DoChangeTransactionStateL(TTransactionState aState);
	
	void RequestPlaylistL(TBool aCancelPrevious);
	
	void PurgeExpiredTracksL( TInt aFromTrack );
	
private:
	void DialogDismissedL(TInt aButtonId);
	void UpdateVolume();

private:
	void DataL(const TDesC8& aData, TInt aTransactionError);
	
private:
	void HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState);
	
private:
	void RunL();
	void DoCancel();

private:
	CMobblerRadioPlaylist* iPlaylist;
	
	CMobblerLastFmConnection& iLastFmConnection;
	
	CMobblerIncomingCallMonitor* iIncomingCallMonitor;
	
	CMobblerAudioControl* iCurrentAudioControl;
	CMobblerAudioControl* iNextAudioControl;
	
	TTimeIntervalSeconds iPreBufferSize;
	TInt iVolume;
	TInt iMaxVolume;
	TInt iVolumeSteps;
	TInt iEqualizerIndex;
	TInt iBitRate;
	
	TState iState;
	TTransactionState iTransactionState;
	
	CMobblerString* iStation;
	
	RPointerArray<MMobblerRadioStateChangeObserver> iObservers;
	
	RTimer iTimer;
	
	TBool iRestartRadioOnCallDisconnect;
	
	TBool iRestart;
	
	// Count how many times the equalizer has killed audio threads
	TInt iAbnormalTerminations;
	};

#endif // __MOBBLERRADIOPLAYER_H__

// End of file
