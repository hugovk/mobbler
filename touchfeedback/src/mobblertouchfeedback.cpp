/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009  Michael Coffey
Copyright (C) 2009  Hugo van Kemenade

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

#include <ecom/implementationproxy.h>
#include "mobblertouchfeedback.h"

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x20039AFB};
#else
const TInt KImplementationUid = {0xA000B6CD};
#endif


const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerTouchFeedback::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerTouchFeedback* CMobblerTouchFeedback::NewL()
	{
	CMobblerTouchFeedback* self(new(ELeave) CMobblerTouchFeedback());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTouchFeedback::CMobblerTouchFeedback()
	{
	}

void CMobblerTouchFeedback::ConstructL()
	{
#ifdef  __S60_50__
	iTouchFeedback = MTouchFeedback::Instance();
	iTouchFeedback->SetFeedbackEnabledForThisApp(ETrue);
#else
	User::Leave(KErrNotSupported);
#endif
	}

CMobblerTouchFeedback::~CMobblerTouchFeedback()
	{
	}

void CMobblerTouchFeedback::InstantFeedback(TInt aType)
	{
	(void)aType; // Remove compiler warning
#ifdef  __S60_50__
	iTouchFeedback->InstantFeedback(static_cast<TTouchLogicalFeedback>(aType));
#endif
	}

// End of file
