/*
mobblermarquee.cpp

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

#include "mobblerappui.h"
#include "mobblermarquee.h"
#include "mobblerstatuscontrol.h"

const TTimeIntervalMicroSeconds32 KDelay(2000000);
const TTimeIntervalMicroSeconds32 KInterval(100000);

CMobblerMarquee* CMobblerMarquee::NewL(CMobblerStatusControl& aStatusControl)
	{
	CMobblerMarquee* self(new(ELeave) CMobblerMarquee(aStatusControl));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMarquee::CMobblerMarquee(CMobblerStatusControl& aStatusControl)
	:iStatusControl(aStatusControl)
	{
	}

void CMobblerMarquee::ConstructL()
	{
	iTimer = CPeriodic::NewL(CActive::EPriorityLow);
	}

CMobblerMarquee::~CMobblerMarquee()
	{
	delete iTimer;
	delete iText;
	}

void CMobblerMarquee::Start(const TDesC& aText, TInt aInitialOffset, TInt aTextWidth, TInt aDisplayWidth)
	{
	if (!iText || iText && iText->Compare(aText) != 0)
		{
		delete iText;
		iText = aText.AllocL();
		
		iTextWidth = aTextWidth;
		iDisplayWidth = aDisplayWidth;
		iInitialOffset = aInitialOffset;
		
		iTimer->Cancel();
		
		if (iTextWidth + aInitialOffset > iDisplayWidth)
			{
			TCallBack callBack(CallBack, this);
			iTimer->Start(KDelay, KInterval, callBack);
			}
		
		iState = EStart;
		}
	}

TInt CMobblerMarquee::GetPosition1() const
	{
	TInt offset(iInitialOffset);
	TInt calcOffset(0);
	TInt maxOffset(0);
	
	switch (iState)
		{
		case EStart:
			// do nothing
			break;
		case EForward:
			calcOffset = iInitialOffset - (User::TickCount() - iStartTickCount);
			maxOffset =  iInitialOffset - iTextWidth - (iInitialOffset * 5);
			offset = Max(calcOffset, maxOffset);
			break;
		}
	
	return offset;
	}

TInt CMobblerMarquee::GetPosition2() const
	{
	TInt offset(KMaxTInt);
	
	if (iTextWidth + iInitialOffset > iDisplayWidth)
		{
		offset = GetPosition1() + iTextWidth + (iInitialOffset * 5);
		}
	
	return offset;
	}

void CMobblerMarquee::Reset()
	{
	delete iText;
	iText = NULL;
	}

TInt CMobblerMarquee::CallBack(TAny* aRef)
	{
	static_cast<CMobblerMarquee*>(aRef)->Update();
	return KErrNone;
	}

void CMobblerMarquee::Update()
	{
	if (!static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->Foreground())
		{
		iTimer->Cancel();
		Reset();
		}
	
	switch (iState)
		{
		case EStart:
			iState = EForward;
			iStartTickCount = User::TickCount();
			break;
		case EForward:
			if (GetPosition1() > iInitialOffset - iTextWidth - (iInitialOffset * 5))
				{
				// do nothing
				}
			else
				{
				iState = EStart;
				
				delete iTimer;
				iTimer = CPeriodic::NewL(CActive::EPriorityLow);
				TCallBack callBack(CallBack, this);
				iTimer->Start(KDelay, KInterval, callBack);
				}
			break;
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

// End of file
