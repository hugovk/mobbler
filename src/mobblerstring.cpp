/*
mobblerstring.cpp

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

#include <utf.h> 

#include "mobblerstring.h"
//#include "mobblertracer.h"

const TChar KForbiddenCharacterArray[] =
	{
	'<',
	'>',
	'"',
	'/',
	'|',
	'\\',
	'?',
	'*',
	':',
	};

CMobblerString* CMobblerString::NewL(const TDesC& aString)
	{
//	TRACER_AUTO;
	CMobblerString* self(NewLC(aString));
	CleanupStack::Pop(self);
	return self;
	}

CMobblerString* CMobblerString::NewL(const TDesC8& aString)
	{
//	TRACER_AUTO;
	CMobblerString* self(NewLC(aString));
	CleanupStack::Pop(self);
	return self;
	}

CMobblerString* CMobblerString::NewLC(const TDesC& aString)
	{
//	TRACER_AUTO;
	CMobblerString* self(new(ELeave) CMobblerString);
	CleanupStack::PushL(self);
	self->ConstructL(aString);
	return self;
	}

CMobblerString* CMobblerString::NewLC(const TDesC8& aString)
	{
//	TRACER_AUTO;
	CMobblerString* self(new(ELeave) CMobblerString);
	CleanupStack::PushL(self);
	self->ConstructL(aString);
	return self;
	}

CMobblerString::CMobblerString()
	{
//	TRACER_AUTO;
	}

void CMobblerString::ConstructL(const TDesC& aString)
	{
//	TRACER_AUTO;
	iString = aString.AllocL();
	iString8 = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aString);
	
	iStringPtr.Set(*iString);
	iString8Ptr.Set(*iString8);
	}

void CMobblerString::ConstructL(const TDesC8& aString)
	{
//	TRACER_AUTO;
	iString8 = aString.AllocL();
	iString = CnvUtfConverter::ConvertToUnicodeFromUtf8L(aString);
	
	iStringPtr.Set(*iString);
	iString8Ptr.Set(*iString8);
	}

CMobblerString::~CMobblerString()
	{
//	TRACER_AUTO;
	delete iString;
	delete iString8;
	}

const TPtrC& CMobblerString::String() const
	{
//	TRACER_AUTO;
	return iStringPtr;
	}

const TPtrC8& CMobblerString::String8() const
	{
//	TRACER_AUTO;
	return iString8Ptr;
	}

const TPtrC CMobblerString::SafeFsString(const TInt aKnownPathLength) const
	{
//	TRACER_AUTO;
	TFileName stripped;
	stripped.Copy(SafeFsString8(aKnownPathLength));
	return stripped;
	}

const TPtrC8 CMobblerString::SafeFsString8(const TInt aKnownPathLength) const
	{
//	TRACER_AUTO;
	TBuf8<KMaxFileName> stripped8(iString8Ptr);

	const TInt arraySize(sizeof(KForbiddenCharacterArray) / sizeof(TChar));
	for (TInt i(0); i < arraySize; ++i)
		{
		TInt position(stripped8.Locate(KForbiddenCharacterArray[i]));
		 while (position != KErrNotFound)
			{
			stripped8.Delete(position, 1);
			position = stripped8.Locate(KForbiddenCharacterArray[i]);
			}
		}

	TInt fullPathLength(stripped8.Length() + aKnownPathLength);
	if (fullPathLength > KMaxFileName)
		{
		TInt pos(KMaxFileName - fullPathLength);
		TInt length(stripped8.Length() - pos);
		stripped8.Delete(pos, length);
		}

	return stripped8;
	}

// End of file
