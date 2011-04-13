/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade
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

#ifndef __MOBBLERUTILITY_H__
#define __MOBBLERUTILITY_H__

#include <e32base.h>

class CSenDomFragment;
class CSenXmlReader;

class MobblerUtility
	{
public:
	static TBool EqualizerSupported();
	static void SetEqualizerNotSupported();
	
	static HBufC8* MD5LC(const TDesC8& aSource);
	static HBufC8* URLEncodeLC(const TDesC8& aString, TBool aEncodeAll = ETrue);
	
	static TBuf8<2> LanguageL();
	static TBuf<30> LocalLastFmDomainL(const TInt aMobile = ETrue);

	static void FixLyricsSpecialCharacters(TDes8& aText);
	static void FixLyricsLineBreaks(TDes8& aText);
	
	static void StripUnwantedTagsFromHtmlL(HBufC8*& aHtml);

	static CSenDomFragment* PrepareDomFragmentLC(CSenXmlReader& aXmlReader, const TDesC8& aXml);

private:
	static TBool iEqualizerSupported;
	};

#endif // __MOBBLERUTILITY_H__

// End of file
