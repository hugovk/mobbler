/*
mobblerutility.h

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

#ifndef __MOBBLERUTILITY_H__
#define __MOBBLERUTILITY_H__

#include <e32base.h>

class MobblerUtility
	{
public:
	static TBool EqualizerSupported();
	
	static HBufC8* MD5LC(const TDesC8& aSource);
	static HBufC8* URLEncodeLC(const TDesC8& aString);
	
	static TBuf8<2> LanguageL();
	static TBuf<30> LocalLastFmDomainL();

	static void FixLyricsSpecialCharacters(TDes8& aText);
	static void FixLyricsLineBreaks(TDes8& aText);
	
	static void StripUnwantedTagsFromHtmlL(HBufC8*& aHtml);

	};

#endif // __MOBBLERUTILITY_H__

// End of file
