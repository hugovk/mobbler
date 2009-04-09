/*
mobblerparser.h

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

#ifndef __MOBBLERPARSER_H__
#define __MOBBLERPARSER_H__

#include <e32base.h>

class CMobblerAlbumList;
class CMobblerArtistList;
class CMobblerEventList;
class CMobblerFriendList;
class CMobblerListItem;
class CMobblerPlaylistList;
class CMobblerRadioPlaylist;
class CMobblerShoutbox;
class CMobblerTagList;
class CMobblerTrackList;

class CMobblerParser : public CBase
	{
public:
	static CMobblerLastFMError* ParseHandshakeL(const TDesC8& aHandshakeResponse, HBufC8*& aSessionId, HBufC8*& aNowPlayingUrl, HBufC8*& aSubmitUrl);
	static CMobblerLastFMError* ParseRadioHandshakeL(const TDesC8& aRadioHandshakeResponse, HBufC8*& aRadioSessionID, HBufC8*& aRadioBaseUrl, HBufC8*& aRadioBasePath);
	static CMobblerLastFMError* ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey);
#ifdef BETA_BUILD
	static CMobblerLastFMError* ParseBetaTestersHandshakeL(const TDesC8& aHandshakeResponse, const TDesC8& aUsername, TBool& aIsBetaTester);
#endif
	static CMobblerLastFMError* ParseScrobbleResponseL(const TDesC8& aScrobbleResponse);
	
	static CMobblerLastFMError* ParseRadioSelectStationL(const TDesC8& aXml);
	static CMobblerLastFMError* ParseRadioPlaylistL(const TDesC8& aXml, CMobblerRadioPlaylist*& aPlaylist);

	static TInt ParseUpdateResponseL(const TDesC8& aXml, TVersion& aVersion, TDes8& location);
	
	static void ParseFriendListL(const TDesC8& aXml, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseTopArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecommendedArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseEventsL(const TDesC8& aXml, CMobblerEventList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseTopAlbumsL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList);
	
	static void ParseUserTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseArtistTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecentTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
		
	static void ParseTopTagsL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParsePlaylistsL(const TDesC8& aXml, CMobblerPlaylistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseShoutboxL(const TDesC8& aXml, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList);
		
private:
	static HBufC8* DecodeURIStringLC(const TDesC8& aString);
	};

#endif // __MOBBLERPARSER_H__

// End of file
