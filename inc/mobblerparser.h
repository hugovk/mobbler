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
	static CMobblerLastFmError* ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey, CMobblerLastFmConnection::TLastFmMemberType& aMemberType);
	
#ifdef BETA_BUILD
	static CMobblerLastFmError* ParseBetaTestersHandshakeL(const TDesC8& aHandshakeResponse, const TDesC8& aUsername, TBool& aIsBetaTester);
#endif
	static CMobblerLastFmError* ParseScrobbleResponseL(const TDesC8& aScrobbleResponse);

	static CMobblerLastFmError* ParseRadioTuneL(const TDesC8& aXml, CMobblerString*& aStationName);
	static CMobblerLastFmError* ParseRadioPlaylistL(const TDesC8& aXml, CMobblerRadioPlaylist& aPlaylist);
	
	static HBufC8* CMobblerParser::ParseTwitterAuthL(const TDesC8& aData);

	static TInt ParseUpdateResponseL(const TDesC8& aXml, TVersion& aVersion, TDes8& location);
	
	static TInt FriendOrder(const CMobblerListItem& aLeft, const CMobblerListItem& aRight);

	static void ParseFriendListL(const TDesC8& aXml, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList, TInt& aTotal, TInt& aPage, TInt& aPerPage, TInt& aTotalPages);
	static void ParseTopArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecommendedArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseEventsL(const TDesC8& aXml, CMobblerEventList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseTopAlbumsL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList);

    static void ParseSearchTrackL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
    static void ParseSearchAlbumL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList);
    static void ParseSearchArtistL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
    static void ParseSearchTagL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList);

	static void ParseUserTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseArtistTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecentTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParsePlaylistL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);

	static void ParseTopTagsL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParsePlaylistsL(const TDesC8& aXml, CMobblerPlaylistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseShoutboxL(const TDesC8& aXml, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList);

	static void ParseArtistInfoL(const TDesC8& aXml, HBufC8*& aArtistBio, HBufC8*& aImageUrl, HBufC8*& aTagsText, HBufC8*& aSimilarArtistsText);

//private:
//	static HBufC8* DecodeURIStringLC(const TDesC8& aString);
	};

#endif // __MOBBLERPARSER_H__

// End of file
