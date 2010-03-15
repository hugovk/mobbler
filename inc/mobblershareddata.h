/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009  Steve Punter
Copyright (C) 2009  Michael Coffey
Copyright (C) 2010  Hugo van Kemenade

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

#ifndef __MOBBLERSHAREDDATA_H__
#define __MOBBLERSHAREDDATA_H__

#include <e32std.h>
#include <mda/common/audio.h>

class CAudioEqualizerUtility;
class CMobblerTrack;

enum TMobblerAudioCmd
	{
	ECmdDestroyAudio,
	ECmdSetVolume,
	ECmdSetEqualizer,
	ECmdWriteData,
	ECmdSetCurrent
	};

class TMobblerSharedData
	{
public:
	// communication request statuses
	TRequestStatus* iCmdStatus;
	
	// audio stream settings and etc
	TInt iMaxVolume;
	TBool iPlaying;
	TBool iDownloadComplete;
	TTimeIntervalSeconds iPreBufferSize;
	TBool iCurrent;
	TMdaAudioDataSettings iAudioDataSettings;
	
	// data that is sent for ECmdServiceBuffer
	TInt iTotalDataSize;
	TPtrC8 iAudioData;
	
	// Data for ECmdSetEqualizer
	TInt iEqualizerIndex;
	
	// Has the equaliser killed the audio thread yet?
	TBool iAbnormalTermination;
	
	CMobblerTrack* iTrack;
	};

#endif

// End of file
