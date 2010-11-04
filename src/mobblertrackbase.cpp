/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade

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

#include "mobblerappui.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrackbase.h"
#include "mobblerutility.h"

CMobblerTrackBase* CMobblerTrackBase::NewL(const CMobblerTrackBase& aTrack)
	{
    TRACER_AUTO;
	CMobblerTrackBase* self(new(ELeave) CMobblerTrackBase);
	CleanupStack::PushL(self);
	self->BaseConstructL(aTrack);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrackBase* CMobblerTrackBase::NewL(RReadStream& aReadStream)
	{
    TRACER_AUTO;
	CMobblerTrackBase* self(new(ELeave) CMobblerTrackBase);
	CleanupStack::PushL(self);
	self->InternalizeL(aReadStream);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrackBase::CMobblerTrackBase()
	{
//	TRACER_AUTO;
	}

CMobblerTrackBase::CMobblerTrackBase(TTimeIntervalSeconds aTrackLength, TBool aLoved)
	:iLove(aLoved ? ELoved : ENoLove),
	iTrackNumber(KErrUnknown),
	iStartTimeUTC(Time::NullTTime()), 
	iTrackLength(aTrackLength),
	iTotalPlayed(0), 
	iInitialPlaybackPosition(KErrUnknown)
	{
//	TRACER_AUTO;
	}

void CMobblerTrackBase::BaseConstructL(const TDesC8& aTitle,
		const TDesC8& aArtist,
		const TDesC8& aAlbum,
		const TDesC8& aStreamId)
	{
    TRACER_AUTO;
	iArtist = CMobblerString::NewL(aArtist);
	iTitle = CMobblerString::NewL(aTitle);
	iAlbum = CMobblerString::NewL(aAlbum);
	iStreamId = aStreamId.AllocL();
	}

void CMobblerTrackBase::BaseConstructL(const CMobblerTrackBase& aTrack)
	{
    TRACER_AUTO;
	iArtist = CMobblerString::NewL(aTrack.Artist().String());
	iTitle = CMobblerString::NewL(aTrack.Title().String());
	iAlbum = CMobblerString::NewL(aTrack.Album().String());
	iTrackLength = aTrack.iTrackLength;
	iStreamId = aTrack.iStreamId->AllocL();
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
    TRACER_AUTO;
	if (iLoveObserverHelper)
		{
		iLoveObserverHelper->SetNotOwned();
		}
	
	delete iArtist;
	delete iTitle;
	delete iAlbum;
	delete iStreamId;
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
    TRACER_AUTO;
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
	iLove = static_cast<TMobblerLove>(aReadStream.ReadInt8L());
	delete iStreamId;
	iStreamId = HBufC8::NewL(aReadStream, KMaxTInt);
	delete iAlbum;
	iAlbum = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	}

void CMobblerTrackBase::ExternalizeL(RWriteStream& aWriteStream) const
	{
    TRACER_AUTO;
	aWriteStream.WriteInt32L(I64HIGH(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(I64LOW(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(iTrackLength.Int());
	aWriteStream << iArtist->String8();
	aWriteStream << iTitle->String8();
	aWriteStream.WriteInt8L(iLove);
	aWriteStream << *iStreamId;
	aWriteStream << iAlbum->String8();
	}

HBufC8* CMobblerTrackBase::UrlLC(const TInt aCommand)
	{
	// Get the local Last.fm domain for this phone's language
	TBuf8<KMaxMobblerTextSize> base8;
	base8.Copy(MobblerUtility::LocalLastFmDomainL(EFalse));
	HBufC8* base(base8.AllocLC());

	// Format in the names
	TInt urlLength(base->Length() + iArtist->String8().Length());
	_LIT8(KArtistUrlFormat, "%Smusic/%S");
	_LIT8(KTrackUrlFormat,  "%Smusic/%S/_/%S");
	_LIT8(KAlbumUrlFormat,  "%Smusic/%S/%S");
	switch (aCommand)
		{
		case EMobblerCommandArtistShare:
			urlLength += KArtistUrlFormat().Length();
			break;
		case EMobblerCommandTrackShare:
			urlLength += KTrackUrlFormat().Length() + 
						 iTitle->String8().Length();
			break;
		case EMobblerCommandAlbumShare:
			urlLength += KAlbumUrlFormat().Length() + 
						 iAlbum->String8().Length();
			break;
		default:
			break;
		}
	HBufC8* url(HBufC8::NewL(urlLength));
	TPtr8 basePtr(base->Des());
	switch (aCommand)
		{
		case EMobblerCommandArtistShare:
			url->Des().Format(KArtistUrlFormat, &basePtr, 
								&iArtist->String8());
			break;
		case EMobblerCommandTrackShare:
			url->Des().Format(KTrackUrlFormat, &basePtr, 
								&iArtist->String8(), &iTitle->String8());
			break;
		case EMobblerCommandAlbumShare:
			url->Des().Format(KAlbumUrlFormat, &basePtr, 
								&iArtist->String8(), &iAlbum->String8());
			break;
		default:
			break;
		}
	CleanupStack::Pop(base);

	// Replace space with '+' to make the URL prettier 
	_LIT8(KSpace, " ");
	_LIT8(KPlus, "+");
	TPtr8 urlPtr(url->Des());
	TInt position(urlPtr.Find(KSpace));
	while (position != KErrNotFound)
		{
		urlPtr.Replace(position, 1, KPlus);
		position = urlPtr.Find(KSpace);
		}

	// Encode and return the URL
	HBufC8* urlEncoded(MobblerUtility::URLEncodeLC(urlPtr));
	return urlEncoded;
	}

void CMobblerTrackBase::SetAlbumL(const TDesC& aAlbum)
	{
//	TRACER_AUTO;
	delete iAlbum;
	iAlbum = CMobblerString::NewL(aAlbum);
	}

void CMobblerTrackBase::SetStartTimeUTC(const TTime& aStartTimeUTC)
	{
//	TRACER_AUTO;
	iStartTimeUTC = aStartTimeUTC;
	iTrackPlaying = ETrue;
	}

const TTime& CMobblerTrackBase::StartTimeUTC() const
	{
//	TRACER_AUTO;
	return iStartTimeUTC;
	}

const CMobblerString& CMobblerTrackBase::Artist() const
	{
//	TRACER_AUTO;
	return *iArtist;
	}

const CMobblerString& CMobblerTrackBase::Title() const
	{
//	TRACER_AUTO;
	return *iTitle;
	}

const CMobblerString& CMobblerTrackBase::Album() const
	{
//	TRACER_AUTO;
	return *iAlbum;
	}

TInt CMobblerTrackBase::TrackNumber() const
	{
//	TRACER_AUTO;
	return iTrackNumber;
	}

void CMobblerTrackBase::SetTrackNumber(TInt aTrackNumber)
	{
//	TRACER_AUTO;
	iTrackNumber = aTrackNumber;
	}

const TDesC8& CMobblerTrackBase::StreamId() const
	{
    TRACER_AUTO;
	return *iStreamId;
	}

void CMobblerTrackBase::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& /*aData*/, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aObserver == iLoveObserverHelper && aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		iLove = ELoved;
		}
	}

void CMobblerTrackBase::LoveTrackL()
	{
    TRACER_AUTO;
	if (iLove != ELoved)
		{
		iLove = ELove;
	
		// we have not already told Last.fm about this so do it now
		delete iLoveObserverHelper;
		iLoveObserverHelper = CMobblerFlatDataObserverHelper::NewL(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->LastFmConnection(), *this, EFalse);
		static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->LastFmConnection().QueryLastFmL(EMobblerCommandTrackLove, Artist().String8(), KNullDesC8, Title().String8(), KNullDesC8, *iLoveObserverHelper);
		}
	}

CMobblerTrackBase::TMobblerLove CMobblerTrackBase::Love() const
	{
//	TRACER_AUTO;
	return iLove;
	}

void CMobblerTrackBase::SetTrackLength(TTimeIntervalSeconds aTrackLength)
	{
//	TRACER_AUTO;
	iTrackLength = aTrackLength;
	}

TTimeIntervalSeconds CMobblerTrackBase::TrackLength() const
	{
//	TRACER_AUTO;
	return iTrackLength.Int() == 0 ? 1 : iTrackLength;
	}
	
TBool CMobblerTrackBase::IsMusicPlayerTrack() const
	{
//	TRACER_AUTO;
	return (iStreamId->Compare(KNullDesC8) == 0);
	}

TTimeIntervalSeconds CMobblerTrackBase::ScrobbleDuration() const
	{
    TRACER_AUTO;
	TInt scrobblePercent(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ScrobblePercent());
	return (TTimeIntervalSeconds)Min(240, (TrackLength().Int() *  scrobblePercent / 100));
	}

TTimeIntervalSeconds CMobblerTrackBase::InitialPlaybackPosition() const
	{
    TRACER_AUTO;
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
//	TRACER_AUTO;
	return iPlaybackPosition;
	}

void CMobblerTrackBase::SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition)
	{
    TRACER_AUTO;
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
//	TRACER_AUTO;
	iTotalPlayed = aTotalPlayed;
	iTrackPlaying = EFalse;
	}

TTimeIntervalSeconds CMobblerTrackBase::TotalPlayed() const
	{
//	TRACER_AUTO;
	return iTotalPlayed;
	}

void CMobblerTrackBase::SetScrobbled()
	{
//	TRACER_AUTO;
	iScrobbled = ETrue;
	}

TBool CMobblerTrackBase::Scrobbled() const
	{
//	TRACER_AUTO;
	return iScrobbled;
	}

void CMobblerTrackBase::SetTrackPlaying(TBool aTrackPlaying)
	{
//	TRACER_AUTO;
	iTrackPlaying = aTrackPlaying;
	}

TBool CMobblerTrackBase::TrackPlaying() const
	{
//	TRACER_AUTO;
	return iTrackPlaying;
	}
	
// End of file
