/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010, 2011  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade
Copyright (C) 2009, 2010  gw111zz

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

#include <aknnotewrappers.h>
#include <sendomfragment.h>
#include <sennamespace.h>
#include <senxmlutils.h>

#include "mobbler_strings.rsg.h"
#include "mobbleralbumlist.h"
#include "mobblerappui.h"
#include "mobblerartistlist.h"
#include "mobblereventlist.h"
#include "mobblerfriendlist.h"
#include "mobblerlogging.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerparser.h"
#include "mobblerplaylistlist.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistsettings.h"
#include "mobblersettingitemlistview.h"
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertaglist.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"
#include "mobblerutility.h"

_LIT8(KCompareOkNewLine, "OK\n");
_LIT8(KCompareBadSessionNewLine, "BADSESSION\n");

_LIT8(KAlbumMatches, "albummatches");
_LIT8(KArtistMatches, "artistmatches");
_LIT8(KAuthor, "author");
_LIT8(KBody, "body");
_LIT8(KCity, "city");
_LIT8(KCreator, "creator");
_LIT8(KCode, "code");
_LIT8(KCount, "count");
_LIT8(KCountry, "country");
_LIT8(KDate, "date");
_LIT8(KDuration, "duration");
_LIT8(KExtension, "extension");
_LIT8(KKey, "key");
_LIT8(KIdentifier, "identifier");
_LIT8(KLocation, "location");
_LIT8(KLink, "link");
_LIT8(KLoved, "loved");
_LIT8(KMatch, "match");
_LIT8(KNowPlaying, "nowplaying");
_LIT8(KPlayCount, "playcount");
_LIT8(KRealName, "realname");
_LIT8(KRecommendations, "recommendations");
_LIT8(KSession, "session");
_LIT8(KShouts, "shouts");
_LIT8(KSimilarArtists, "similarartists");
_LIT8(KSimilarTracks, "similartracks");
_LIT8(KStartDate, "startDate");
_LIT8(KSubscriber, "subscriber");
_LIT8(KTagMatches, "tagmatches");
_LIT8(KTopArtists, "topartists");
_LIT8(KTopAlbums, "topalbums");
_LIT8(KTopTags, "toptags");
_LIT8(KTopTracks, "toptracks");
_LIT8(KStreamId, "streamid");
_LIT8(KTrackList, "trackList");
_LIT8(KTrackMatches, "trackmatches");
_LIT8(KTrue, "true");
_LIT8(KUts, "uts");
_LIT8(KGeoNamespaceUri, "http://www.w3.org/2003/01/geo/wgs84_pos#");
_LIT8(KPoint, "point");
_LIT8(KLat, "lat");
_LIT8(KLong, "long");

_LIT8(KFindResponseEqualsOk, "response=OK");
_LIT8(KFindSessionEqualsFailed, "session=FAILED");

_LIT8(KMatchBadAuthStar, "BADAUTH*");
_LIT8(KMatchBadTimeStar, "BADTIME*");
_LIT8(KMatchBannedStar, "BANNED*");
_LIT8(KMatchFailedStar, "FAILED*");
_LIT8(KMatchOkStar, "OK*");
_LIT8(KMatchSessionStar, "session*");

_LIT8(KNewLine, "\n");

_LIT8(KUpdateVersionMajor,		"major");
_LIT8(KUpdateVersionMinor,		"minor");
_LIT8(KUpdateVersionBuild,		"build");
_LIT8(KUpdateLocation,			"location");

_LIT8(KNamespace, "http://www.audioscrobbler.net/dtd/xspf-lastfm");

CMobblerLastFmError* CMobblerParser::ParseScrobbleResponseL(const TDesC8& aScrobbleResponse)
	{
    TRACER_AUTO;
	DUMPDATA(aScrobbleResponse, _L("scrobbleresponse.txt"));

	CMobblerLastFmError* error(NULL);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aScrobbleResponse));
	
	CSenElement& lfm(domFragment->AsElement());
	
	const TDesC8* status(lfm.AttrValue(KStatus));
	
	if (status && status->CompareF(KOk) != 0)
		{
		// there was an error with the scrobble
		TPtrC8 code(lfm.Element(KCode)->Content());
		TPtrC8 message(lfm.Element(KError)->Content());
		
		TInt errorCode;
		TLex8 lex(code);
		lex.Val(errorCode);
		
		error = CMobblerLastFmError::NewL(message, errorCode);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

CMobblerLastFmError* CMobblerParser::ParseRadioTuneL(const TDesC8& aXml, CMobblerString*& aStationName)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("RadioSelectResponse.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	CMobblerLastFmError* error(NULL);

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KStatus));

	if (statusText && (statusText->CompareF(KOk) == 0))
		{
		aStationName = CMobblerString::NewL(domFragment->AsElement().Element(KStation)->Element(KName)->Content());
		}
	else
		{
		CSenElement* errorElement(domFragment->AsElement().Element(KError));

		TLex8 lex(*errorElement->AttrValue(KCode));
		TInt errorCode;
		lex.Val(errorCode);

		error = CMobblerLastFmError::NewL(errorElement->Content(), errorCode);

		aStationName = CMobblerString::NewL(KNullDesC);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

CMobblerLastFmError* CMobblerParser::ParseRadioPlaylistL(const TDesC8& aXml, CMobblerRadioPlaylist& aPlaylist)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("playlist.xml"));

	CMobblerLastFmError* error(NULL);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KStatus));

	if (statusText && (statusText->CompareF(KOk) == 0))
		{
		// The Last.fm error status was ok so get the tracks from the playlist

		RPointerArray<CSenElement>* tracks(NULL);

		CSenElement& domElement(domFragment->AsElement());

		CSenElement* playlistElement(domElement.Child(0));
		
		TTime expiryTime(Time::NullTTime());

		if (playlistElement)
			{
			CSenElement* titleElement(playlistElement->Element(KTitle));
			if (titleElement)
				{
				aPlaylist.SetTitleL(titleElement->Content());
				}
			
			CSenElement* linkElement(playlistElement->Element(KLink));
			if (linkElement)
				{
				TInt expirySeconds;
				TLex8 lex(linkElement->Content());
				lex.Val(expirySeconds);
				expiryTime.UniversalTime();
				expiryTime += TTimeIntervalSeconds(expirySeconds);
				}
		
			CSenElement* trackListElement(playlistElement->Element(KTrackList));

			if (trackListElement)
				{
				tracks = &trackListElement->ElementsL();
				}
			}

		if (tracks)
			{
			const TInt KTrackCount(tracks->Count());
			for (TInt i(0); i < KTrackCount; ++i)
				{
				// get the duration as a number
				TLex8 lex((*tracks)[i]->Element(KDuration)->Content());
				TInt durationMilliSeconds;
				lex.Val(durationMilliSeconds);
				TTimeIntervalSeconds durationSeconds(durationMilliSeconds / 1000);

				TPtrC8 creator((*tracks)[i]->Element(KCreator)->Content());
				HBufC8* creatorBuf(HBufC8::NewLC(creator.Length()));
				SenXmlUtils::DecodeHttpCharactersL(creator, creatorBuf);
				TPtrC8 title((*tracks)[i]->Element(KTitle)->Content());
				HBufC8* titleBuf(HBufC8::NewLC(title.Length()));
				SenXmlUtils::DecodeHttpCharactersL(title, titleBuf);
				TPtrC8 album((*tracks)[i]->Element(KAlbum)->Content());
				HBufC8* albumBuf(HBufC8::NewLC(album.Length()));
				SenXmlUtils::DecodeHttpCharactersL(album, albumBuf);

				TPtrC8 image((*tracks)[i]->Element(KImage)->Content());
				TPtrC8 location((*tracks)[i]->Element(KLocation)->Content());
				TPtrC8 identifier((*tracks)[i]->Element(KIdentifier)->Content());
				TPtrC8 streamId((*tracks)[i]->Element(KExtension)->Element(KStreamId)->Content());

				TBool loved((*tracks)[i]->Element(KExtension)->Element(KLoved)->Content().Compare(K0) != 0);
				
				CMobblerTrack* track(CMobblerTrack::NewL(*creatorBuf, *titleBuf, *albumBuf, identifier, image, location, durationSeconds, streamId, loved, ETrue));
				CleanupStack::PushL(track);
				track->FindLocalTrackL();
				track->SetExpiry(expiryTime);
				aPlaylist.AppendTrackL(track);
				CleanupStack::Pop(track);

				CleanupStack::PopAndDestroy(3, creatorBuf);
				}
			}

		CleanupStack::PopAndDestroy(2, xmlReader);
		}
	else
		{
		CSenElement* errorElement(domFragment->AsElement().Element(KError));

		TLex8 lex(*errorElement->AttrValue(KCode));
		TInt errorCode;
		lex.Val(errorCode);

		error = CMobblerLastFmError::NewL(errorElement->Content(), errorCode);
		}

	return error;
	}


HBufC8* CMobblerParser::ParseTwitterAuthL(const TDesC8& aData)
	{
	TRACER_AUTO;
	// Get the token and token secret out of the response.
	// Example:
	// oauth_token=XXX&oauth_token_secret=XXX&user_id=12345678&screen_name=<username>&x_auth_expires=0

	TPtrC8 data(aData);
	
	_LIT8(KOauthToken,       "oauth_token");
	_LIT8(KOauthTokenSecret, "oauth_token_secret");
	
	while (data.Length() != 0)
		{
		TInt ampPos(data.Find(KAmpersand));
		if (ampPos == KErrNotFound)
			{
			ampPos = data.Length();
			}
			
		TPtrC8 pair(data.Left(ampPos));
		TInt eqlPos(pair.Find(KEquals));
		
		if (eqlPos != KErrNotFound)
			{
			TPtrC8 key(pair.Left(eqlPos));
			TPtrC8 value(pair.Right(pair.Length() - (eqlPos + 1)));
			
			if (key.Compare(KOauthToken) == 0)
				{
				static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->SettingView().Settings().SetTwitterAuthToken(value);
				}
			else if (key.Compare(KOauthTokenSecret) == 0)
				{
				static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->SettingView().Settings().SetTwitterAuthTokenSecret(value);
				}
			
			// Move to the rest of the data
			if (ampPos == data.Length())
				{
				data.Set(KNullDesC8);
				}
			else
				{
				data.Set(data.Right(data.Length() - (ampPos + 1)));
				}
			}
		else
			{
			// There was an error!
			return aData.AllocL();
			}
		}
	return NULL;
	}

CMobblerLastFmError* CMobblerParser::ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey, CMobblerLastFmConnection::TLastFmMemberType& aMemberType)
	{
    TRACER_AUTO;
	DUMPDATA(aWebServicesHandshakeResponse, _L("webserviceshandshakeresponse.xml"));

	CMobblerLastFmError* error(NULL);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aWebServicesHandshakeResponse));

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KStatus));

	if (statusText && (statusText->CompareF(KOk) == 0))
		{
		aWebServicesSessionKey = domFragment->AsElement().Element(KSession)->Element(KKey)->Content().AllocL();
		if (domFragment->AsElement().Element(KSession)->Element(KSubscriber)->Content().Compare(K1) == 0)
			{
			aMemberType = CMobblerLastFmConnection::ESubscriber;
			}
		else
			{
			aMemberType = CMobblerLastFmConnection::EMember;
			}
		}
	else
		{
		CSenElement* errorElement(domFragment->AsElement().Element(KError));

		TLex8 lex(*errorElement->AttrValue(KCode));
		TInt errorCode;
		lex.Val(errorCode);

		error = CMobblerLastFmError::NewL(errorElement->Content(), errorCode);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

#ifdef FULL_BETA_BUILD
CMobblerLastFmError* CMobblerParser::ParseBetaTestersHandshakeL(const TDesC8& aHandshakeResponse, const TDesC8& aUsername, TBool& aIsBetaTester)
	{
    TRACER_AUTO;
	CMobblerLastFmError* error(NULL);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aHandshakeResponse));

	RPointerArray<CSenElement>& testers(domFragment->AsElement().ElementsL());

	aIsBetaTester = EFalse;

	const TInt KTesterCount(testers.Count());
	for (TInt i(0); i < KTesterCount; ++i)
		{
		if (testers[i]->Content().CompareF(aUsername) == 0)
			{
			aIsBetaTester = ETrue;
			break;
			}
		}

	if (!aIsBetaTester)
		{
		_LIT8(KBetaError, "Sorry. You're not registered to use this private beta version. Please visit http://code.google.com/p/mobbler");
		error = CMobblerLastFmError::NewL(KBetaError, CMobblerLastFmError::EFailed);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}
#endif

void CMobblerParser::ParseSearchTrackL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("searchtrack.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KResults)->Element(KTrackMatches)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Content()),
														*image));

		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseSearchAlbumL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("searchalbum.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KResults)->Element(KAlbumMatches)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Content()),
														*image));

		CleanupStack::PopAndDestroy(3);

		item->SetIdL(items[i]->Element(KId)->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	}

void CMobblerParser::ParseSearchArtistL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("searchartist.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KResults)->Element(KArtistMatches)->ElementsL());

    const TInt KItemCount(items.Count());
    for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														KNullDesC8,
														*image));

		CleanupStack::PopAndDestroy(2);

		item->SetIdL(items[i]->Element(KMbid)->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseSearchTagL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("searchtag.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KResults)->Element(KTagMatches)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														items[i]->Element(KCount)->Content(),
														*image));

		CleanupStack::PopAndDestroy(2);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

TInt CMobblerParser::FriendOrder(const CMobblerListItem& aLeft, const CMobblerListItem& aRight)
	{
    TRACER_AUTO;
	return aLeft.Title()->String().CompareF(aRight.Title()->String());
	}

void CMobblerParser::ParseFriendListL(const TDesC8& aXml, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList, TInt& aTotal, TInt& aPage, TInt& aPerPage, TInt& aTotalPages)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("usergetfriends.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	CSenElement* friends(domFragment->AsElement().Element(KFriends));
	
	_LIT8(KTotal, "total");
	_LIT8(KPage, "page");
	_LIT8(KPerPage, "perPage");
	_LIT8(KTotalPages, "totalPages");
	TLex8 lex;
	lex.Assign(*friends->AttrValue(KTotal));
	lex.Val(aTotal);
	lex.Assign(*friends->AttrValue(KPage));
	lex.Val(aPage);
	lex.Assign(*friends->AttrValue(KPerPage));
	lex.Val(aPerPage);
	lex.Assign(*friends->AttrValue(KTotalPages));
	lex.Val(aTotalPages);
	
	RPointerArray<CSenElement>& items(friends->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());
		
		HBufC8* description(NULL);
		
		if (items[i]->Element(KRecentTrack))
			{
			HBufC8* title(SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KRecentTrack)->Element(KName)->Content()));
			HBufC8* artist(SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KRecentTrack)->Element(KArtist)->Element(KName)->Content()));
			
			_LIT8(KEnDashFormat, "%S - %S");
			description = HBufC8::NewL(title->Length() + artist->Length() + KEnDashFormat().Length());
			
			TPtrC8 titlePtr(title->Des());
			TPtrC8 artistPtr(artist->Des());
			description->Des().Format(KEnDashFormat, &titlePtr, &artistPtr);
			
			CleanupStack::PopAndDestroy(2, title);
			CleanupStack::PushL(description);
			}
		else
			{
			// they have not listened to any tracks yet so just put their realname in
			description = items[i]->Element(KRealName)->Content().AllocLC();
			}
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														items[i]->Element(KName)->Content(),
														*description,
														*image));
		
		CleanupStack::PopAndDestroy(description);
		CleanupStack::PopAndDestroy(image);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	TLinearOrder<CMobblerListItem> friendOrder(FriendOrder);
	aList.Sort(friendOrder);

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("usergettopartists.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KTopArtists)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KPlayCount)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}


void CMobblerParser::ParseRecommendedArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("recommendedartists.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KRecommendations)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														KNullDesC8,
														*image));
		CleanupStack::PopAndDestroy(2);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseSimilarArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("similarartists.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KSimilarArtists)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KMatch)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseEventsL(const TDesC8& aXml, CMobblerEventList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("usergetevents.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KEvents)->ElementsL());

	for (TInt i(0); i < items.Count(); ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KImage)->Content().AllocLC());

		// Format the description line
		TPtrC8 venue(items[i]->Element(KVenue)->Element(KName)->Content());
		TPtrC8 city(items[i]->Element(KVenue)->Element(KLocation)->Element(KCity)->Content());
		TPtrC8 country(items[i]->Element(KVenue)->Element(KLocation)->Element(KCountry)->Content());

		_LIT8(KEventDescriptionFormat, "%S, %S, %S");

		HBufC8* location(HBufC8::NewLC(KEventDescriptionFormat().Length() + venue.Length() + city.Length() + country.Length()));
		location->Des().Format(KEventDescriptionFormat, &venue, &city, &country);
		HBufC8* locationDecoded(SenXmlUtils::DecodeHttpCharactersLC(*location));

		// Format the title line
		TPtrC8 title(items[i]->Element(KTitle)->Content());
		TPtrC8 startDate(items[i]->Element(KStartDate)->Content().Mid(0, 11));

		_LIT8(KEventTitleFormat, "%S (%S)");
		HBufC8* eventTitle(HBufC8::NewLC(KEventTitleFormat().Length() + title.Length() + startDate.Length()));
		eventTitle->Des().Format(KEventTitleFormat, &title, &startDate);
		HBufC8* eventTitleDecoded(SenXmlUtils::DecodeHttpCharactersLC(*eventTitle));

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*eventTitleDecoded,
														*locationDecoded,
														*image));

		item->SetIdL(items[i]->Element(KId)->Content());

		// Get the location of the event
		CSenElement* geoPoint(items[i]->Element(KVenue)->Element(KLocation)->Element(KGeoNamespaceUri, KPoint));
		
		if (geoPoint)
			{
			TPtrC8 latitude(geoPoint->Element(KGeoNamespaceUri, KLat)->Content());
			TPtrC8 longitude(geoPoint->Element(KGeoNamespaceUri, KLong)->Content());
			item->SetLongitudeL(longitude);
			item->SetLatitudeL(latitude);
			}
		
		CleanupStack::PopAndDestroy(5);
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopAlbumsL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("usergettopalbums.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KTopAlbums)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Element(KName)->Content()),
														*image));

		item->SetIdL(items[i]->Element(KMbid)->Content());

		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseArtistTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("artisttoptracks.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KTopTracks)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KImage)->Content().AllocLC());

		HBufC8* playcount((items[i]->Element(KPlayCount) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KPlayCount)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*playcount,
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseUserTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("usertoptracks.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KTopTracks)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Element(KName)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParsePlaylistL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("fetchplaylist.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Child(0)->Element(KTrackList)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KTitle)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KCreator)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	}

void CMobblerParser::ParseSimilarTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("similartracks.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KSimilarTracks)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* artistName((items[i]->Element(KArtist) == NULL) ?
				KNullDesC8().AllocLC() :
				SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Element(KName)->Content()));

		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*artistName,
														*image));
		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseRecentTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("recenttracks.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KRecentTracks)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
				KNullDesC8().AllocLC():
				items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KArtist)->Content()),
														*image));

		if (items[i]->AttrValue(KNowPlaying)
				&& items[i]->AttrValue(KNowPlaying)->Compare(KTrue) == 0)
			{
			// We set null as a constant meaning 'now'
			item->SetTimeL(KNullDesC8);
			}
		else
			{
			item->SetTimeL(*items[i]->Element(KDate)->AttrValue(KUts));
			}

		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParsePlaylistsL(const TDesC8& aXml, CMobblerPlaylistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("userplaylists.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KPlaylists)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		HBufC8* image((items[i]->Element(KImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KTitle)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KDescription)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		item->SetIdL(items[i]->Element(KId)->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseShoutboxL(const TDesC8& aXml, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("shoutbox.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KShouts)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < Min(KItemCount, 25); ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KAuthor)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KBody)->Content()),
														KNullDesC8));
		CleanupStack::PopAndDestroy(2);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopTagsL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("toptags.xml"));

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KTopTags)->ElementsL());

	const TInt KItemCount(items.Count());
	for (TInt i(0); i < KItemCount; ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KName)->Content()),
														items[i]->Element(KCount)->Content(),
														KNullDesC8));
		CleanupStack::PopAndDestroy(1);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

TInt CMobblerParser::ParseUpdateResponseL(const TDesC8& aXml, TVersion& aVersion, TDes8& aLocation)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("update.xml"));

	TInt error(KErrNone);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	TLex8 lex8;
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMajor)->Content());
	error |= lex8.Val(aVersion.iMajor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMinor)->Content());
	error |= lex8.Val(aVersion.iMinor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionBuild)->Content());
	error |= lex8.Val(aVersion.iBuild);
	aLocation.Copy(domFragment->AsElement().Element(KUpdateLocation)->Content());

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

/**
 * Ownership of aArtistBio, aImageUrl, aTagsText remain with the calling function.
 */
void CMobblerParser::ParseArtistInfoL(const TDesC8& aXml, HBufC8*& aArtist, HBufC8*& aArtistBio, HBufC8*& aImageUrl, HBufC8*& aTagsText, HBufC8*& aSimilarArtistsText)
	{
    TRACER_AUTO;
	DUMPDATA(aXml, _L("artistgetinfo.xml"));
	_LIT8(KSimilar, "similar");
	_LIT8(KCommaSpace, ", ");
	_LIT8(KBio, "bio");
	_LIT8(KContent, "content");
	_LIT8(KSummary, "summary");

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aXml));

	// Create HTML formatted output for display in the browser control

	CSenElement* artistElement(domFragment->AsElement().Element(KArtist));
	User::LeaveIfNull(artistElement); // No point continuing if there is no data at all

	// Get the artist name
	TPtrC8 artistPtr(artistElement->Element(KName)->Content());

	aArtist = HBufC8::NewLC(artistPtr.Length());
	SenXmlUtils::DecodeHttpCharactersL(artistPtr, aArtist);

	// Get the actual artist's biography
	// Display summary if there is no extended content
	TPtrC8 bioPtr(artistElement->Element(KBio)->Element(KContent)->Content());
	if (bioPtr.Length() == 0)
		{
		bioPtr.Set(artistElement->Element(KBio)->Element(KSummary)->Content());
		}

	aArtistBio = HBufC8::NewLC(bioPtr.Length());
	SenXmlUtils::DecodeHttpCharactersL(bioPtr, aArtistBio);

	MobblerUtility::StripUnwantedTagsFromHtmlL(aArtistBio);
	CleanupStack::Pop(); 	// StripUnwantedTagsFromHtmlL may reallocate the buffer
							// hence the pointer may change
	CleanupStack::PushL(aArtistBio);

	// Get the image URL
	RPointerArray<CSenElement> imageArray;
	CleanupClosePushL(imageArray);
	TInt err(artistElement->ElementsL(imageArray, KImage));

	// If we get an error, skip tags and get other info, only the biography itself
	// is essential.
	if (err == KErrNone)
		{
		const TInt KImageCount(imageArray.Count());
		for (TInt i(0); i < KImageCount; ++i)
			{
			if (imageArray[i]->AttrValue(KSize)->Compare(KLarge) == 0)
				{
				aImageUrl = HBufC8::NewL(imageArray[i]->Content().Length());
				TPtr8 imageUrlPtr(aImageUrl->Des());
				imageUrlPtr.Copy(imageArray[i]->Content());
				ASSERT(aImageUrl->Length() == imageUrlPtr.Length());
				break;
				}
			}
		}
	CleanupStack::PushL(aImageUrl);

	// Get the tags
	RPointerArray<CSenElement> tagsArray;
	CleanupClosePushL(tagsArray);
	err = artistElement->Element(KTags)->ElementsL(tagsArray, KTag);

	const TInt KTagCount(tagsArray.Count());
	TInt lengthOfTagText(0);

	if (err == KErrNone && KTagCount > 0)
		{
		lengthOfTagText = (KTagCount-1) * 2; // Number of commas + space in between tags
		for (TInt i(0); i < KTagCount; ++i)
			{
			if (tagsArray[i]->Element(KName))
				{
				lengthOfTagText += tagsArray[i]->Element(KName)->Content().Length();
				}
			}
		}

	aTagsText = HBufC8::NewLC(lengthOfTagText);
	TPtr8 tagsTextPtr(aTagsText->Des());

	if (err == KErrNone && lengthOfTagText > 0)
		{
		for (TInt i(0); i < KTagCount; ++i)
			{
			if (tagsArray[i]->Element(KName))
				{
				tagsTextPtr.Append(tagsArray[i]->Element(KName)->Content());
				if (i != (KTagCount-1))
					{
					tagsTextPtr.Append(KCommaSpace);
					}
				}
			}
		}

	// Get similar artists
	// Get the tags
	RPointerArray<CSenElement> similarArtistsArray;
	CleanupClosePushL(similarArtistsArray);
	err = artistElement->Element(KSimilar)->ElementsL(similarArtistsArray, KArtist);

	const TInt KSimilarArtistsCount(similarArtistsArray.Count());
	TInt lengthOfSimilarArtistsText(0);

	if (err == KErrNone && KSimilarArtistsCount > 0)
		{
		lengthOfSimilarArtistsText = (KSimilarArtistsCount-1) * 2; // Number of commas + space in between SimilarArtists
		for (TInt i(0); i < KSimilarArtistsCount; ++i)
			{
			if (similarArtistsArray[i]->Element(KName))
				{
				lengthOfSimilarArtistsText += similarArtistsArray[i]->Element(KName)->Content().Length();
				}
			}
		}

	aSimilarArtistsText = HBufC8::NewLC(lengthOfSimilarArtistsText);
	TPtr8 similarArtistsTextPtr(aSimilarArtistsText->Des());

	if (err == KErrNone && lengthOfSimilarArtistsText > 0)
		{
		for (TInt i(0); i < KSimilarArtistsCount; ++i)
			{
			if (similarArtistsArray[i]->Element(KName))
				{
				similarArtistsTextPtr.Append(similarArtistsArray[i]->Element(KName)->Content());
				if (i != (KSimilarArtistsCount-1))
					{
					similarArtistsTextPtr.Append(KCommaSpace);
					}
				}
			}
		}

	// Must have something to display
	__ASSERT_DEBUG(aArtistBio, User::Invariant());

	CleanupStack::Pop(aSimilarArtistsText);
	CleanupStack::PopAndDestroy(&similarArtistsArray);
	CleanupStack::Pop(aTagsText);
	CleanupStack::PopAndDestroy(&tagsArray);
	CleanupStack::Pop(aImageUrl);
	CleanupStack::PopAndDestroy(&imageArray);
	CleanupStack::Pop(aArtistBio);
	CleanupStack::Pop(aArtist);
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

// End of file
