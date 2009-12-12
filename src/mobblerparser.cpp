/*
mobblerparser.cpp

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
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertaglist.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"
#include "mobblerutility.h"

_LIT8(KElementCreator, "creator");
_LIT8(KElementDuration, "duration");
_LIT8(KElementKey, "key");
_LIT8(KElementLink, "link");
_LIT8(KElementLocation, "location");
_LIT8(KElementRadioAuth, "trackauth");
_LIT8(KElementSession, "session");
_LIT8(KElementTrackList, "trackList");
_LIT8(KElementSubscriber, "subscriber");
_LIT8(KElementEvents, "events");
_LIT8(KElementArtist, "artist");
_LIT8(KElementAuthor, "author");
_LIT8(KElementBody, "body");
_LIT8(KElementNowPlaying, "nowplaying");

_LIT8(KUpdateVersionMajor,		"major");
_LIT8(KUpdateVersionMinor,		"minor");
_LIT8(KUpdateVersionBuild,		"build");
_LIT8(KUpdateLocation,			"location");

_LIT8(KNamespace, "http://www.audioscrobbler.net/dtd/xspf-lastfm");

/*
OK
    This indicates that the handshake was successful. Three lines will follow the OK response:

       1. Session ID
       2. Now-Playing URL
       3. Submission URL

BANNED
    This indicates that this client version has been banned from the server. This usually happens if the client is violating the protocol in a destructive way. Users should be asked to upgrade their client application.
BADAUTH
    This indicates that the authentication details provided were incorrect. The client should not retry the handshake until the user has changed their details.
BADTIME
    The timestamp provided was not close enough to the current time. The system clock must be corrected before re-handshaking.
FAILED <reason>
    This indicates a temporary server failure. The reason indicates the cause of the failure. The client should proceed as directed in the Hard Failures section.
All other responses should be treated as a hard failure.
    An error may be reported to the user, but as with other messages this should be kept to a minimum.
*/
CMobblerLastFmError* CMobblerParser::ParseHandshakeL(const TDesC8& aHandshakeResponse, HBufC8*& aSessionId, HBufC8*& aNowPlayingUrl, HBufC8*& aSubmitUrl)
	{
	DUMPDATA(aHandshakeResponse, _L("scrobblehandshake.txt"));

	CMobblerLastFmError* error(NULL);

	if (aHandshakeResponse.MatchF(_L8("OK*")) == 0)
		{
		TInt position(aHandshakeResponse.Find(_L8("\n")));

		// get the session ID
		TPtrC8 last3Lines(aHandshakeResponse.Mid(position + 1, aHandshakeResponse.Length() - (position + 1)));
		position = last3Lines.Find(_L8("\n"));
		delete aSessionId;
		aSessionId = last3Lines.Mid(0, position).AllocL();

		// get the now playing URL
		TPtrC8 last2Lines(last3Lines.Mid(position + 1, last3Lines.Length() - (position + 1)));
		position = last2Lines.Find(_L8("\n"));
		delete aNowPlayingUrl;
		aNowPlayingUrl = last2Lines.Mid(0, position).AllocL();

		// get the submit URL
		TPtrC8 last1Lines(last2Lines.Mid(position + 1, last2Lines.Length() - (position + 1)));
		position = last1Lines.Find(_L8("\n"));
		delete aSubmitUrl;
		aSubmitUrl = last1Lines.Mid(0, position).AllocL();
		}
	else if (aHandshakeResponse.MatchF(_L8("BANNED*")) == 0)
		{
		error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BANNED), CMobblerLastFmError::EBanned);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADAUTH*")) == 0)
		{
		error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_AUTH), CMobblerLastFmError::EBadAuth);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADTIME*")) == 0)
		{
		error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_TIME), CMobblerLastFmError::EBadTime);
		}
	else if (aHandshakeResponse.MatchF(_L8("FAILED*")) == 0)
		{
		error = CMobblerLastFmError::NewL(aHandshakeResponse.Mid(7), CMobblerLastFmError::EFailed);
		}
	else
		{
		error = CMobblerLastFmError::NewL(aHandshakeResponse, CMobblerLastFmError::EFailed);
		}

	return error;
	}

/*
session
stream_url=http://87.117.229.85:80/last.mp3?Session=ae1eb54a11615e605d61d6e83dde71bc
subscriber=0
framehack=0
base_url=ws.audioscrobbler.com
base_path=/radio
info_message=
fingerprint_upload_url=http://ws.audioscrobbler.com/fingerprint/upload.php
*/
CMobblerLastFmError* CMobblerParser::ParseOldRadioHandshakeL(const TDesC8& aRadioHandshakeResponse, HBufC8*& aRadioSessionID, HBufC8*& aRadioBaseUrl, HBufC8*& aRadioBasePath)
	{
	DUMPDATA(aRadioHandshakeResponse, _L("radiohandshakeresponse.txt"));

	CMobblerLastFmError* error(NULL);

	if (aRadioHandshakeResponse.MatchF(_L8("session*")) == 0)
		{
		if (aRadioHandshakeResponse.Find(_L8("session=FAILED")) == 0)
			{
			error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_AUTH), CMobblerLastFmError::EBadAuth);
			}
		else
			{
			RDesReadStream readStream(aRadioHandshakeResponse);
			CleanupClosePushL(readStream);

			const TInt KLines(7);
			for (TInt i(0); i < KLines; ++i)
				{
				_LIT(KDelimeter, "\n");

				TBuf8<255> line;
				readStream.ReadL(line, TChar(KDelimeter()[0]));

				_LIT8(KEquals, "=");
				TInt equalPosition(KErrNotFound);

				equalPosition = line.Find(KEquals);

				if (i == 0)
					{
					//session id
					delete aRadioSessionID;
					aRadioSessionID = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 1)
					{
					// stream URL
					//delete aRadioStreamUrl;
					//aRadioStreamUrl = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 4)
					{
					//base URL
					delete aRadioBaseUrl;
					aRadioBaseUrl = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 5)
					{
					//base path
					delete aRadioBasePath;
					aRadioBasePath = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				}

			CleanupStack::PopAndDestroy(&readStream);
			}
		}
	else if (aRadioHandshakeResponse.MatchF(_L8("FAILED*")) == 0)
		{
		error = CMobblerLastFmError::NewL(aRadioHandshakeResponse.Mid(7), CMobblerLastFmError::EFailed);
		}
	else
		{
		error = CMobblerLastFmError::NewL(aRadioHandshakeResponse, CMobblerLastFmError::EFailed);
		}

	return error;
	}

CMobblerLastFmError* CMobblerParser::ParseScrobbleResponseL(const TDesC8& aScrobbleResponse)
	{
	DUMPDATA(aScrobbleResponse, _L("scrobbleresponse.txt"));

	CMobblerLastFmError* error(NULL);

	if (aScrobbleResponse.Compare(_L8("OK\n")) == 0)
		{
		// do nothing
		}
	else if (aScrobbleResponse.Compare(_L8("BADSESSION\n")) == 0)
		{
		error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_SESSION), CMobblerLastFmError::EBadSession);
		}
	else if (aScrobbleResponse.MatchF(_L8("FAILED*")) == 0)
		{
		error = CMobblerLastFmError::NewL(aScrobbleResponse.Mid(7), CMobblerLastFmError::EFailed);
		}
	else
		{
		error = CMobblerLastFmError::NewL(aScrobbleResponse, CMobblerLastFmError::EFailed);
		}

	return error;
	}

HBufC8* CMobblerParser::DecodeURIStringLC(const TDesC8& aString)
	{
	HBufC8* result(aString.AllocLC());

	_LIT8(KPlus, "+");
	_LIT8(KSpace, " ");
	_LIT8(KPercent, "%");

	TInt pos(result->Find(KPlus));
	while (pos != KErrNotFound)
		{
		// replace the plus with a space
		result->Des().Delete(pos, 1);
		result->Des().Insert(pos, KSpace);

		// try to find the next one
		pos = result->Find(KPlus);
		}

	pos = result->Find(KPercent);
	while (pos != KErrNotFound)
		{
		// get the two numbers after the percent
		TLex8 lex(result->Mid(pos + 1, 2));

		TUint8 value;
		if (lex.Val(value, EHex) == KErrNone)
			{
			TBuf8<1> replaceChar;
			replaceChar.Append(value);

			result->Des().Delete(pos, 3);
			result->Des().Insert(pos, replaceChar);

			// try to find the next one
			pos = result->Find(KPercent());
			}
		else
			{
			// There was an error converting the number into
			// a character so leave the string half done
			break;
			}
		}

	result->Des().Trim();

	return result;
	}

CMobblerLastFmError* CMobblerParser::ParseOldRadioTuneL(const TDesC8& aXml)
	{
	DUMPDATA(aXml, _L("oldradioselectresponse.xml"));

	CMobblerLastFmError* error(NULL);

	if (aXml.Find(_L8("response=OK")) != 0)
		{
		error = CMobblerLastFmError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION), CMobblerLastFmError::EFailed);
		}

	return error;
	}

CMobblerLastFmError* CMobblerParser::ParseOldRadioPlaylistL(const TDesC8& aXml, CMobblerRadioPlaylist& aPlaylist)
	{
	DUMPDATA(aXml, _L("playlist.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& tracks(domFragment->AsElement().Element(KElementTrackList)->ElementsL());

	const TInt KTrackCount(tracks.Count());
	for (TInt i(0); i < KTrackCount; ++i)
		{
		// get the duration as a number
		TLex8 lex(tracks[i]->Element(KElementDuration)->Content());
		TInt durationMilliSeconds;
		lex.Val(durationMilliSeconds);
		TTimeIntervalSeconds durationSeconds(durationMilliSeconds / 1000);

		TPtrC8 creator(tracks[i]->Element(KElementCreator)->Content());
		HBufC8* creatorBuf(HBufC8::NewLC(creator.Length()));
		SenXmlUtils::DecodeHttpCharactersL(creator, creatorBuf);
		TPtrC8 title(tracks[i]->Element(KElementTitle)->Content());
		HBufC8* titleBuf(HBufC8::NewLC(title.Length()));
		SenXmlUtils::DecodeHttpCharactersL(title, titleBuf);
		TPtrC8 album(tracks[i]->Element(KElementAlbum)->Content());
		HBufC8* albumBuf(HBufC8::NewLC(album.Length()));
		SenXmlUtils::DecodeHttpCharactersL(album, albumBuf);
		TPtrC8 image(tracks[i]->Element(KElementImage)->Content());
		TPtrC8 location(tracks[i]->Element(KElementLocation)->Content());
		TPtrC8 radioAuth(tracks[i]->Element(KNamespace, KElementRadioAuth)->Content());
		TPtrC8 musicBrainzId(tracks[i]->Element(KElementId)->Content());

		CMobblerTrack* track(CMobblerTrack::NewL(*creatorBuf, *titleBuf, *albumBuf, /**albumIDBuf,*/ musicBrainzId, image, location, durationSeconds, radioAuth));
		CleanupStack::PushL(track);
		aPlaylist.AppendTrackL(track);
		CleanupStack::Pop(track);

		if (track->Album().String().Length() == 0)
			{
			// We do this so that the track knows for sure
			// that there is no album name and will use the
			// album art from the playlist
			track->SetAlbumL(KNullDesC);
			}

		CleanupStack::PopAndDestroy(3, creatorBuf);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return NULL;
	}

CMobblerLastFmError* CMobblerParser::ParseRadioTuneL(const TDesC8& aXml, CMobblerString*& aStationName)
	{
	DUMPDATA(aXml, _L("RadioSelectResponse.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	CMobblerLastFmError* error(NULL);

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));

	if (statusText && (statusText->CompareF(_L8("ok")) == 0))
		{
		aStationName = CMobblerString::NewL(domFragment->AsElement().Element(_L8("station"))->Element(KElementName)->Content());
		}
	else
		{
		CSenElement* errorElement(domFragment->AsElement().Element(_L8("error")));

		TLex8 lex(*errorElement->AttrValue(_L8("code")));
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
	DUMPDATA(aXml, _L("playlist.xml"));

	CMobblerLastFmError* error(NULL);

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));

	if (statusText && (statusText->CompareF(_L8("ok")) == 0))
		{
		// The Last.fm error status was ok so get the tracks from the playlist

		RPointerArray<CSenElement>* tracks(NULL);

		CSenElement& domElement(domFragment->AsElement());

		CSenElement* playlistElement(domElement.Child(0));

		if (playlistElement)
			{
			CSenElement* trackListElement(playlistElement->Element(KElementTrackList));

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
				TLex8 lex((*tracks)[i]->Element(KElementDuration)->Content());
				TInt durationMilliSeconds;
				lex.Val(durationMilliSeconds);
				TTimeIntervalSeconds durationSeconds(durationMilliSeconds / 1000);

				TPtrC8 creator((*tracks)[i]->Element(KElementCreator)->Content());
				HBufC8* creatorBuf(HBufC8::NewLC(creator.Length()));
				SenXmlUtils::DecodeHttpCharactersL(creator, creatorBuf);
				TPtrC8 title((*tracks)[i]->Element(KElementTitle)->Content());
				HBufC8* titleBuf(HBufC8::NewLC(title.Length()));
				SenXmlUtils::DecodeHttpCharactersL(title, titleBuf);
				TPtrC8 album((*tracks)[i]->Element(KElementAlbum)->Content());
				HBufC8* albumBuf(HBufC8::NewLC(album.Length()));
				SenXmlUtils::DecodeHttpCharactersL(album, albumBuf);

				TPtrC8 image((*tracks)[i]->Element(KElementImage)->Content());
				TPtrC8 location((*tracks)[i]->Element(KElementLocation)->Content());
				TPtrC8 identifier((*tracks)[i]->Element(_L8("identifier"))->Content());
				TPtrC8 trackauth((*tracks)[i]->Element(_L8("extension"))->Element(_L8("trackauth"))->Content());

				CMobblerTrack* track(CMobblerTrack::NewL(*creatorBuf, *titleBuf, *albumBuf, identifier, image, location, durationSeconds, trackauth));
				CleanupStack::PushL(track);
				aPlaylist.AppendTrackL(track);
				CleanupStack::Pop(track);

				if (track->Album().String().Length() == 0)
					{
					// We do this so that the track knows for sure
					// that there is no album name and will use the
					// album art from the playlist
					track->SetAlbumL(KNullDesC);
					}

				CleanupStack::PopAndDestroy(3, creatorBuf);
				}
			}

		CleanupStack::PopAndDestroy(2, xmlReader);
		}
	else
		{
		CSenElement* errorElement(domFragment->AsElement().Element(_L8("error")));

		TLex8 lex(*errorElement->AttrValue(_L8("code")));
		TInt errorCode;
		lex.Val(errorCode);

		error = CMobblerLastFmError::NewL(errorElement->Content(), errorCode);
		}

	return error;
	}

CMobblerLastFmError* CMobblerParser::ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey, CMobblerLastFmConnection::TLastFmMemberType& aMemberType)
	{
	DUMPDATA(aWebServicesHandshakeResponse, _L("webserviceshandshakeresponse.xml"));

	CMobblerLastFmError* error(NULL);

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aWebServicesHandshakeResponse);

	// Get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));

	if (statusText && (statusText->CompareF(_L8("ok")) == 0))
		{
		aWebServicesSessionKey = domFragment->AsElement().Element(KElementSession)->Element(KElementKey)->Content().AllocL();
		if (domFragment->AsElement().Element(KElementSession)->Element(KElementSubscriber)->Content().Compare(_L8("1")) == 0)
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
		CSenElement* errorElement(domFragment->AsElement().Element(_L8("error")));

		TLex8 lex(*errorElement->AttrValue(_L8("code")));
		TInt errorCode;
		lex.Val(errorCode);

		error = CMobblerLastFmError::NewL(errorElement->Content(), errorCode);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

#ifdef BETA_BUILD
CMobblerLastFmError* CMobblerParser::ParseBetaTestersHandshakeL(const TDesC8& aHandshakeResponse, const TDesC8& aUsername, TBool& aIsBetaTester)
	{
	CMobblerLastFmError* error(NULL);

	// create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// parse the XML into the DOM fragment
	xmlReader->ParseL(aHandshakeResponse);

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
		error = CMobblerLastFmError::NewL(_L8("Sorry. You're not registered to use this private beta version. Please visit http://code.google.com/p/mobbler"), CMobblerLastFmError::EFailed);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}
#endif

void CMobblerParser::ParseSearchTrackL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("searchtrack.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("results"))->Element(_L8("trackmatches"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Content()),
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
	DUMPDATA(aXml, _L("searchalbum.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("results"))->Element(_L8("albummatches"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Content()),
														*image));

		CleanupStack::PopAndDestroy(3);

		item->SetIdL(items[i]->Element(KElementId)->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);

	}

void CMobblerParser::ParseSearchArtistL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("searchartist.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("results"))->Element(_L8("artistmatches"))->ElementsL());

    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														KNullDesC8,
														*image));

		CleanupStack::PopAndDestroy(2);

		item->SetIdL(items[i]->Element(_L8("mbid"))->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseSearchTagL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("searchtag.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("results"))->Element(_L8("tagmatches"))->ElementsL());

    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														items[i]->Element(_L8("count"))->Content(),
														*image));

		CleanupStack::PopAndDestroy(2);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseFriendListL(const TDesC8& aXml, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergetfriends.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("friends"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														items[i]->Element(KElementName)->Content(),
														items[i]->Element(_L8("realname"))->Content(),
														*image));

		CleanupStack::PopAndDestroy(image);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergettopartists.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("topartists"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("playcount"))->Content()),
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
	DUMPDATA(aXml, _L("recommendedartists.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("recommendations"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
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
	DUMPDATA(aXml, _L("similarartists.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("similarartists"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("match"))->Content()),
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
	DUMPDATA(aXml, _L("usergetevents.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(KElementEvents)->ElementsL());

	for (TInt i(0); i < items.Count(); ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());

		// Format the description line
		TPtrC8 venue(items[i]->Element(_L8("venue"))->Element(KElementName)->Content());
		TPtrC8 city(items[i]->Element(_L8("venue"))->Element(_L8("location"))->Element(_L8("city"))->Content());
		TPtrC8 country(items[i]->Element(_L8("venue"))->Element(_L8("location"))->Element(_L8("country"))->Content());

		_LIT8(KEventDescriptionFormat, "%S, %S, %S");

		HBufC8* location(HBufC8::NewLC(KEventDescriptionFormat().Length() + venue.Length() + city.Length() + country.Length()));
		location->Des().Format(KEventDescriptionFormat, &venue, &city, &country);
		HBufC8* locationDecoded(SenXmlUtils::DecodeHttpCharactersLC(*location));

		// Format the title line
		TPtrC8 title(items[i]->Element(KElementTitle)->Content());
		TPtrC8 startDate(items[i]->Element(_L8("startDate"))->Content().Mid(0, 11));

		_LIT8(KEventTitleFormat, "%S (%S)");
		HBufC8* eventTitle(HBufC8::NewLC(KEventTitleFormat().Length() + title.Length() + startDate.Length()));
		eventTitle->Des().Format(KEventTitleFormat, &title, &startDate);
		HBufC8* eventTitleDecoded(SenXmlUtils::DecodeHttpCharactersLC(*eventTitle));

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*eventTitleDecoded,
														*locationDecoded,
														*image));

		item->SetIdL(items[i]->Element(KElementId)->Content());

		CleanupStack::PopAndDestroy(5);
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopAlbumsL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergettopalbums.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("topalbums"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Element(KElementName)->Content()),
														*image));

		item->SetIdL(items[i]->Element(_L8("mbid"))->Content());

		CleanupStack::PopAndDestroy(3);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseArtistTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("artisttoptracks.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("toptracks"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(_L8("image")) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(_L8("image"))->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														items[i]->Element(_L8("playcount"))->Content(),
														*image));
		CleanupStack::PopAndDestroy(2);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseUserTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usertoptracks.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("toptracks"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Element(KElementName)->Content()),
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
	DUMPDATA(aXml, _L("fetchplaylist.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Child(0)->Element(_L8("trackList"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementTitle)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("creator"))->Content()),
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
	DUMPDATA(aXml, _L("similartracks.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("similartracks"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* artistName((items[i]->Element(KElementArtist) == NULL) ?
				KNullDesC8().AllocLC() :
				SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Element(KElementName)->Content()));

		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
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
	DUMPDATA(aXml, _L("recenttracks.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("recenttracks"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(_L8("image")) == NULL) ?
				KNullDesC8().AllocLC():
				items[i]->Element(_L8("image"))->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementArtist)->Content()),
														*image));

		if (items[i]->AttrValue(KElementNowPlaying)
				&& items[i]->AttrValue(KElementNowPlaying)->Compare(_L8("true")) == 0)
			{
			// We set null as a constant meaning 'now'
			item->SetTimeL(KNullDesC8);
			}
		else
			{
			item->SetTimeL(*items[i]->Element(_L8("date"))->AttrValue(_L8("uts")));
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
	DUMPDATA(aXml, _L("userplaylists.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("playlists"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());

		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementTitle)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementDescription)->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);

		item->SetIdL(items[i]->Element(KElementId)->Content());

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseShoutboxL(const TDesC8& aXml, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("shoutbox.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("shouts"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < Min(KCount, 25); ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementAuthor)->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementBody)->Content()),
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
	DUMPDATA(aXml, _L("toptags.xml"));

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	RPointerArray<CSenElement>& items(domFragment->AsElement().Element(_L8("toptags"))->ElementsL());

	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(KElementName)->Content()),
														items[i]->Element(_L8("count"))->Content(),
														KNullDesC8));
		CleanupStack::PopAndDestroy(1);

		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}

	CleanupStack::PopAndDestroy(2, xmlReader);
	}

TInt CMobblerParser::ParseUpdateResponseL(const TDesC8& aXml, TVersion& aVersion, TDes8& location)
	{
	DUMPDATA(aXml, _L("update.xml"));

	TInt error(KErrNone);

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	TLex8 lex8;
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMajor)->Content());
	error |= lex8.Val(aVersion.iMajor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMinor)->Content());
	error |= lex8.Val(aVersion.iMinor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionBuild)->Content());
	error |= lex8.Val(aVersion.iBuild);
	location.Copy(domFragment->AsElement().Element(KUpdateLocation)->Content());

	CleanupStack::PopAndDestroy(2, xmlReader);

	return error;
	}

/**
 * Ownership of aArtistBio, aImageUrl, aTagsText remain with the calling function.
 */
void CMobblerParser::ParseArtistInfoL(const TDesC8& aXml, HBufC8*& aArtistBio, HBufC8*& aImageUrl, HBufC8*& aTagsText, HBufC8*& aSimilarArtistsText)
	{
	DUMPDATA(aXml, _L("artistgetinfo.xml"));
	_LIT8(KSimilar, "similar");
	_LIT8(KCommaSpace, ", ");
	_LIT8(KElementBio, "bio");
	_LIT8(KElementContent, "content");
	_LIT8(KElementSummary, "summary");
	_LIT8(KElementTag, "tag");
	_LIT8(KElementTags, "tags");

	// Create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);

	// Create HTML formatted output for display in the browser control

	CSenElement* artistElement(domFragment->AsElement().Element(KElementArtist));
	User::LeaveIfNull(artistElement); // No point continuing if there is no data at all

	// Get the actual artist's biography
	// Display summary if there is no extended content
	TPtrC8 contentPtr(artistElement->Element(KElementBio)->Element(KElementContent)->Content());
	if (contentPtr.Length() == 0)
		{
		contentPtr.Set(artistElement->Element(KElementBio)->Element(KElementSummary)->Content());
		}

	aArtistBio = HBufC8::NewLC(contentPtr.Length());
	SenXmlUtils::DecodeHttpCharactersL(contentPtr, aArtistBio);

	TPtr8 artistInfoPtr(aArtistBio->Des());
	MobblerUtility::StripUnwantedTagsFromHtml(artistInfoPtr);

	// Get the image url
	RPointerArray<CSenElement> imageArray;
	CleanupClosePushL(imageArray);
	TInt err(artistElement->ElementsL(imageArray, KElementImage));

	// If we get an error, skip tags and get other info, only the biography itself
	// is essential.
	if (err == KErrNone)
		{
		const TInt KImageCount(imageArray.Count());
		for (TInt i(0); i < KImageCount; ++i)
			{
			if (imageArray[i]->AttrValue(KElementSize)->Compare(KElementLarge) == 0)
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
	err = artistElement->Element(KElementTags)->ElementsL(tagsArray, KElementTag);

	const TInt KTagCount(tagsArray.Count());
	TInt lengthOfTagText(0);

	if (err == KErrNone && KTagCount > 0)
		{
		lengthOfTagText = (KTagCount-1) * 2; // Number of commas + space in between tags
		for (TInt i(0); i < KTagCount; ++i)
			{
			if (tagsArray[i]->Element(KElementName))
				{
				lengthOfTagText += tagsArray[i]->Element(KElementName)->Content().Length();
				}
			}
		}

	aTagsText = HBufC8::NewLC(lengthOfTagText);
	TPtr8 tagsTextPtr(aTagsText->Des());

	if (err == KErrNone && lengthOfTagText > 0)
		{
		for (TInt i(0); i < KTagCount; ++i)
			{
			if (tagsArray[i]->Element(KElementName))
				{
				tagsTextPtr.Append(tagsArray[i]->Element(KElementName)->Content());
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
	err = artistElement->Element(KSimilar)->ElementsL(similarArtistsArray, KElementArtist);

	const TInt KSimilarArtistsCount(similarArtistsArray.Count());
	TInt lengthOfSimilarArtistsText(0);

	if (err == KErrNone && KSimilarArtistsCount > 0)
		{
		lengthOfSimilarArtistsText = (KSimilarArtistsCount-1) * 2; // Number of commas + space in between SimilarArtists
		for (TInt i(0); i < KSimilarArtistsCount; ++i)
			{
			if (similarArtistsArray[i]->Element(KElementName))
				{
				lengthOfSimilarArtistsText += similarArtistsArray[i]->Element(KElementName)->Content().Length();
				}
			}
		}

	aSimilarArtistsText = HBufC8::NewLC(lengthOfSimilarArtistsText);
	TPtr8 similarArtistsTextPtr(aSimilarArtistsText->Des());

	if (err == KErrNone && lengthOfSimilarArtistsText > 0)
		{
		for (TInt i(0); i < KSimilarArtistsCount; ++i)
			{
			if (similarArtistsArray[i]->Element(KElementName))
				{
				similarArtistsTextPtr.Append(similarArtistsArray[i]->Element(KElementName)->Content());
				if (i != (KSimilarArtistsCount-1))
					{
					similarArtistsTextPtr.Append(KCommaSpace);
					}
				}
			}
		}

	CleanupStack::Pop(aSimilarArtistsText);
	CleanupStack::PopAndDestroy(&similarArtistsArray);
	CleanupStack::Pop(aTagsText);
	CleanupStack::PopAndDestroy(&tagsArray);
	CleanupStack::Pop(aImageUrl);
	CleanupStack::PopAndDestroy(&imageArray);
	CleanupStack::Pop(aArtistBio);
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

// End of file
