/*
mobblertrackbase.h

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

#ifndef __MOBBLERTRACKBASE_H__
#define __MOBBLERTRACKBASE_H__

#include <e32base.h>
#include <s32strm.h>

class CMobblerString;

class CMobblerTrackBase : public CBase
	{
public:
	static CMobblerTrackBase* NewL(const CMobblerTrackBase& aTrack);
	static CMobblerTrackBase* NewL(RReadStream& aReadStream);
	~CMobblerTrackBase();
	
public:
	const CMobblerString& Artist() const;
	const CMobblerString& Title() const;
	const CMobblerString& Album() const;
	const TDesC8& RadioAuth() const;
	
	void SetAlbumL(const TDesC& aAlbum);
	
	TBool IsMusicPlayerTrack() const;
	
	TInt TrackNumber() const;
	void SetTrackNumber(const TInt aTrackNumber);
	
	void SetTrackLength(TTimeIntervalSeconds aTrackLength);
	TTimeIntervalSeconds TrackLength() const;
	
	void SetStartTimeUTC(const TTime& aStartTimeUTC);
	const TTime& StartTimeUTC() const;
	
	void SetLove(TBool aLove);
	TBool Love() const;
	
	TTimeIntervalSeconds ScrobbleDuration() const;
	TTimeIntervalSeconds InitialPlaybackPosition() const;
	TTimeIntervalSeconds PlaybackPosition() const;
	void SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition);
	void SetTotalPlayed(TTimeIntervalSeconds aTotalPlayed);
	TTimeIntervalSeconds TotalPlayed() const;
	void SetTrackPlaying(TBool aTrackPlaying);
	TBool TrackPlaying() const;
	
	void SetScrobbled();
	TBool Scrobbled() const;
	
	TBool operator==(const CMobblerTrackBase& aTrack) const;
	
	void InternalizeL(RReadStream& aReadStream);
	void ExternalizeL(RWriteStream& aWriteStream) const;
	
protected:
	CMobblerTrackBase(TTimeIntervalSeconds iTrackLength);
	void BaseConstructL(const TDesC8& aTitle, const TDesC8& aArtist, const TDesC8& aAlbum, const TDesC8& aRadioAuth);
	
private:
	CMobblerTrackBase();
	void BaseConstructL(const CMobblerTrackBase& aTrack);
	
private:
	CMobblerString* iArtist;
	CMobblerString* iTitle;
	CMobblerString* iAlbum;
	
	TInt iTrackNumber;
	TTime iStartTimeUTC;
	
	TBool iTrackPlaying;
	
	TTimeIntervalSeconds iTrackLength;
	
	HBufC8* iRadioAuth;
	TBool iLove;

	TTimeIntervalSeconds iTotalPlayed;
	TTimeIntervalSeconds iInitialPlaybackPosition;
	TTimeIntervalSeconds iPlaybackPosition;
	
	TBool iScrobbled;
	};

#endif // __MOBBLERTRACKBASE_H__

// End of file
