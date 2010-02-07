/*
mobblerstring.h

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

#ifndef __MOBBLERSTRING_H__
#define __MOBBLERSTRING_H__

#include <e32base.h>

class CMobblerString : public CBase
	{
public:
	static CMobblerString* NewL(const TDesC& aString);
	static CMobblerString* NewL(const TDesC8& aString);
	~CMobblerString();
	
	const TPtrC& String() const;
	const TPtrC8& String8() const;

	const TPtrC SafeFsString(const TInt aKnownPathLength = 0) const;
	const TPtrC8 SafeFsString8(const TInt aKnownPathLength = 0) const;
	
private:
	CMobblerString();
	void ConstructL(const TDesC& aString);
	void ConstructL(const TDesC8& aString);
	
private:
	HBufC* iString;
	TPtrC iStringPtr;
	HBufC8* iString8;
	TPtrC8 iString8Ptr;
	};
	
#endif // __MOBBLERSTRING_H__

// End of file
