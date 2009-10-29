/*
mobblerlastfmerror.cpp

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

#include "mobblerlastfmerror.h"

CMobblerLastFmError* CMobblerLastFmError::NewL(const TDesC8& aText, TInt aLastFmErrorCode)
	{
	CMobblerLastFmError* self(new(ELeave) CMobblerLastFmError(aLastFmErrorCode));
	CleanupStack::PushL(self);
	self->ConstructL(aText);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLastFmError* CMobblerLastFmError::NewL(const TDesC& aText, TInt aLastFmErrorCode)
	{
	CMobblerLastFmError* self(new(ELeave) CMobblerLastFmError(aLastFmErrorCode));
	CleanupStack::PushL(self);
	self->ConstructL(aText);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLastFmError::CMobblerLastFmError(TInt aLastFmErrorCode)
	:iLastFmErrorCode(aLastFmErrorCode)
	{
	}

void CMobblerLastFmError::ConstructL(const TDesC8& aText)
	{
	iText = HBufC::NewL(aText.Length());
	iText->Des().Copy(aText);
	}

void CMobblerLastFmError::ConstructL(const TDesC& aText)
	{
	iText = aText.AllocL();
	}

CMobblerLastFmError::~CMobblerLastFmError()
	{
	delete iText;
	}

const TDesC& CMobblerLastFmError::Text() const
	{
	return *iText;
	}

TInt CMobblerLastFmError::LastFmErrorCode() const
	{
	return iLastFmErrorCode;
	}

// End of file
