/*
mobblerlastfmerror.h

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

#ifndef __MOBBLERLASTFMERROR_H__
#define __MOBBLERLASTFMERROR_H__

#include <e32base.h>

/** This represents a Last.fm error code

Last.fm error codes can be found in their API documentation here http://www.last.fm/api

*/
class CMobblerLastFmError : public CBase
	{
public:
	static CMobblerLastFmError* NewL(const TDesC8& aText, TInt aLastFmErrorCode);
	static CMobblerLastFmError* NewL(const TDesC& aText, TInt aLastFmErrorCode);
	~CMobblerLastFmError();
	
	const TDesC& Text() const;
	
	TInt LastFmErrorCode() const;
	
private:
	CMobblerLastFmError(TInt aLastFmErrorCode);
	void ConstructL(const TDesC8& aText);
	void ConstructL(const TDesC& aText);
	
private:
	HBufC* iText;
	TInt iLastFmErrorCode;
	};

#endif // __MOBBLERLASTFMERROR_H__

// End of file
