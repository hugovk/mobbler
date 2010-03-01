/*
mobblernowplayingcallback.cpp

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

#include "mobblermusiclistener.h"
#include "mobblernowplayingcallback.h"
#include "mobblertracer.h"

CMobblerNowPlayingCallback* CMobblerNowPlayingCallback::NewL(CMobblerMusicAppListener& aMusicListener)
	{
    TRACER_AUTO;
	CMobblerNowPlayingCallback* self(new(ELeave) CMobblerNowPlayingCallback(aMusicListener));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerNowPlayingCallback::~CMobblerNowPlayingCallback()
	{
    TRACER_AUTO;
	Cancel();
	}

CMobblerNowPlayingCallback::CMobblerNowPlayingCallback(CMobblerMusicAppListener& aMusicListener)
	:CTimer(EPriorityStandard), iMusicListener(aMusicListener)
	{
    TRACER_AUTO;
	CActiveScheduler::Add(this);
	}

void CMobblerNowPlayingCallback::ConstructL()
	{
    TRACER_AUTO;
	this->CTimer::ConstructL();
	After(KNowPlayingCallbackDelay);
	}

void CMobblerNowPlayingCallback::RunL()
	{
    TRACER_AUTO;
	if (iStatus.Int() == KErrNone)
		{
		iMusicListener.NowPlayingL();
		}
	}

// End of file
