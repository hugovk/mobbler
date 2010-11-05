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

#ifndef __MOBBLERAUDIOCONTROL_H__
#define __MOBBLERAUDIOCONTROL_H__

// INCLUDES
#include <E32Base.h>
#include "mobblersegdataobserver.h"
#include "mobblerlastfmconnection.h"
#include "mobblershareddata.h"

class CMobblerAudioControl;

class MMobblerAudioControlObserver
	{
public:
	virtual void HandleAudioPositionChangeL() = 0;
	virtual void HandleAudioFinishedL(CMobblerAudioControl* aAudioControl, 
										TBool aAbnormalTermination) = 0;
	};

class CMobblerAudioControl : public CActive, public MMobblerSegDataObserver
	{
public:
	static CMobblerAudioControl* NewL(MMobblerAudioControlObserver& aObserver,
										CMobblerTrack& aTrack,
										TTimeIntervalSeconds aPreBufferSize, 
										TInt aVolume,
										TInt aEqualizerIndex,
										TInt aSampleRate);
	~CMobblerAudioControl();
	
	void SetVolume(TInt aVolume);
	void SetEqualizerIndex(TInt aIndex);
	void SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize);
	void SetCurrent();

	TInt Volume() const;
	TInt MaxVolume() const;
	TBool Playing() const;
	TBool DownloadComplete() const;
	TTimeIntervalSeconds PreBufferSize() const;
	
private:
	CMobblerAudioControl(MMobblerAudioControlObserver& aObserver);
	void ConstructL(CMobblerTrack& aTrack, TTimeIntervalSeconds aPreBufferSize, TInt aVolume, TInt aEqualizerIndex, TInt aSampleRate);
	
	void SendCmd(TMobblerAudioCmd aCmd);
	
	static TInt HandleAudioPositionChangeL(TAny* aSelf);
	
private: // from CActive
	void RunL();
	void DoCancel();
	
private: // from 
	void DataPart(const TDesC8& aData, TInt aTotalSize);
	void DataCompleteL(TInt aTransactionError, TInt aHTTPStatusCode, const TDesC8& aStatusText);
	
private:
	TMobblerSharedData iShared;
	RThread iAudioThread;
	
	TBool iDestroyCmdSent;
	
	CPeriodic* iTimer;
	
	MMobblerAudioControlObserver& iObserver;
	};
  
#endif

// End of File
