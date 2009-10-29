/*
mobblertrackbase.cpp

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

#include <bautils.h>

#include "mobblerappui.h"
#include "mobblerlogging.h"
#include "mobblerstring.h"
#include "mobblertrackbase.h"
#include "mobblerutility.h"


CMobblerTrackBase* CMobblerTrackBase::NewL(const CMobblerTrackBase& aTrack)
	{
	CMobblerTrackBase* self(new(ELeave) CMobblerTrackBase);
	CleanupStack::PushL(self);
	self->BaseConstructL(aTrack);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrackBase* CMobblerTrackBase::NewL(RReadStream& aReadStream)
	{
	CMobblerTrackBase* self(new(ELeave) CMobblerTrackBase);
	CleanupStack::PushL(self);
	self->InternalizeL(aReadStream);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrackBase::CMobblerTrackBase()
	{
	}

CMobblerTrackBase::CMobblerTrackBase(TTimeIntervalSeconds aTrackLength)
	:iTrackNumber(KErrUnknown), iStartTimeUTC(Time::NullTTime()), iTrackLength(aTrackLength), iTotalPlayed(0), iInitialPlaybackPosition(KErrUnknown)
	{
	}

void CMobblerTrackBase::BaseConstructL(const TDesC8& aTitle,
		const TDesC8& aArtist,
		const TDesC8& aAlbum,
		const TDesC8& aRadioAuth)
	{
	iArtist = CMobblerString::NewL(aArtist);
	iTitle = CMobblerString::NewL(aTitle);
	iAlbum = CMobblerString::NewL(aAlbum);
	iRadioAuth = aRadioAuth.AllocL();
	}

void CMobblerTrackBase::BaseConstructL(const CMobblerTrackBase& aTrack)
	{
	iArtist = CMobblerString::NewL(aTrack.Artist().String());
	iTitle = CMobblerString::NewL(aTrack.Title().String());
	iAlbum = CMobblerString::NewL(aTrack.Album().String());
	iTrackLength = aTrack.iTrackLength;
	iRadioAuth = aTrack.iRadioAuth->AllocL();
	iTrackNumber = aTrack.iTrackNumber;
	iStartTimeUTC = aTrack.iStartTimeUTC;
	iLove = aTrack.iLove;
	iTotalPlayed = aTrack.iTotalPlayed;
	iInitialPlaybackPosition = aTrack.iInitialPlaybackPosition;
	iPlaybackPosition = aTrack.iPlaybackPosition;
	iScrobbled = aTrack.iScrobbled;
	iTrackPlaying = aTrack.iTrackPlaying;
	}

CMobblerTrackBase::~CMobblerTrackBase()
	{
	delete iArtist;
	delete iTitle;
	delete iAlbum;
	delete iRadioAuth;
	}

TBool CMobblerTrackBase::operator==(const CMobblerTrackBase& aTrack) const
	{
	// check that the artist, album, and track name are the same
	return (iArtist->String().Compare(aTrack.iArtist->String()) == 0) &&
		   (iTitle->String().Compare(aTrack.iTitle->String()) == 0) &&
		   (iAlbum->String().Compare(aTrack.iAlbum->String()) == 0);
	}

void CMobblerTrackBase::InternalizeL(RReadStream& aReadStream)
	{
	TInt high(aReadStream.ReadInt32L());
	TInt low(aReadStream.ReadInt32L());
	iStartTimeUTC = MAKE_TINT64(high, low);
	iTrackLength = aReadStream.ReadInt32L();
	delete iArtist;
	delete iTitle;
	iArtist = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	iTitle = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	iLove = aReadStream.ReadInt8L();
	delete iRadioAuth;
	iRadioAuth = HBufC8::NewL(aReadStream, KMaxTInt);
	delete iAlbum;
	iAlbum = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	}

void CMobblerTrackBase::ExternalizeL(RWriteStream& aWriteStream) const
	{
	aWriteStream.WriteInt32L(I64HIGH(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(I64LOW(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(iTrackLength.Int());
	aWriteStream << iArtist->String8();
	aWriteStream << iTitle->String8();
	aWriteStream.WriteInt8L(iLove);
	aWriteStream << *iRadioAuth;
	aWriteStream << iAlbum->String8();
	}

void CMobblerTrackBase::SetAlbumBaseL(const TDesC& aAlbum)
	{
	delete iAlbum;
	iAlbum = CMobblerString::NewL(aAlbum);
	}

void CMobblerTrackBase::SetStartTimeUTC(const TTime& aStartTimeUTC)
	{
	iStartTimeUTC = aStartTimeUTC;
	iTrackPlaying = ETrue;
	}

const TTime& CMobblerTrackBase::StartTimeUTC() const
	{
	return iStartTimeUTC;
	}

const CMobblerString& CMobblerTrackBase::Artist() const
	{
	return *iArtist;
	}

const CMobblerString& CMobblerTrackBase::Title() const
	{
	return *iTitle;
	}

const CMobblerString& CMobblerTrackBase::Album() const
	{
	return *iAlbum;
	}

TInt CMobblerTrackBase::TrackNumber() const
	{
	return iTrackNumber;
	}

void CMobblerTrackBase::SetTrackNumber(TInt aTrackNumber)
	{
	iTrackNumber = aTrackNumber;
	}

const TDesC8& CMobblerTrackBase::RadioAuth() const
	{
	return *iRadioAuth;
	}

void CMobblerTrackBase::SetLove(TBool aLove)
	{
	iLove = aLove;
	}

TBool CMobblerTrackBase::Love() const
	{
	return iLove;
	}

TTimeIntervalSeconds CMobblerTrackBase::TrackLength() const
	{
	return iTrackLength.Int() == 0 ? 1 : iTrackLength;
	}
	
TBool CMobblerTrackBase::IsMusicPlayerTrack() const
	{
	return (iRadioAuth->Compare(KNullDesC8) == 0);
	}

TTimeIntervalSeconds CMobblerTrackBase::ScrobbleDuration() const
	{
	TInt scrobblePercent(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ScrobblePercent());
	return (TTimeIntervalSeconds)Min(240, (TrackLength().Int() *  scrobblePercent / 100));
	}

TTimeIntervalSeconds CMobblerTrackBase::InitialPlaybackPosition() const
	{
	if (iInitialPlaybackPosition.Int() == KErrUnknown)
		{
		return 0;
		}
	else
		{
		return iInitialPlaybackPosition;
		}
	}

TTimeIntervalSeconds CMobblerTrackBase::PlaybackPosition() const
	{
	return iPlaybackPosition;
	}

void CMobblerTrackBase::SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition)
	{
	iPlaybackPosition = aPlaybackPosition;

	if (iInitialPlaybackPosition.Int() == KErrUnknown)
		{
		iInitialPlaybackPosition = aPlaybackPosition;

		// Only need to correct music player tracks, because
		// iInitialPlaybackPosition may be up to 5 seconds off
		if (IsMusicPlayerTrack())
			{
			TTimeIntervalSeconds offset(0);
			TTime now;
			now.UniversalTime();
			TInt error(now.SecondsFrom(StartTimeUTC(), offset));
			if (error == KErrNone && offset.Int() > 0)
				{
				iInitialPlaybackPosition = Max(0, iInitialPlaybackPosition.Int() - offset.Int());
				}
			}
		}
	}

void CMobblerTrackBase::SetTotalPlayed(TTimeIntervalSeconds aTotalPlayed)
	{
	iTotalPlayed = aTotalPlayed;
	iTrackPlaying = EFalse;
	}

TTimeIntervalSeconds CMobblerTrackBase::TotalPlayed() const
	{
	return iTotalPlayed;
	}

void CMobblerTrackBase::SetScrobbled()
	{
	iScrobbled = ETrue;
	}

TBool CMobblerTrackBase::Scrobbled() const
	{
	return iScrobbled;
	}

void CMobblerTrackBase::SetTrackPlaying(TBool aTrackPlaying)
	{
	iTrackPlaying = aTrackPlaying;
	}

TBool CMobblerTrackBase::TrackPlaying() const
	{
	return iTrackPlaying;
	}
	
// End of file
