/*
mobblersleeptimer.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#include "mobblersleeptimer.h"
#include "mobblertracer.h"

CMobblerSleepTimer::CMobblerSleepTimer(const TInt aPriority, 
									   MMobblerSleepTimerNotify& aNotify)
:CActive(aPriority), iNotify(aNotify)
	{
    TRACER_AUTO;
	}

CMobblerSleepTimer::~CMobblerSleepTimer()
	{
    TRACER_AUTO;
	Cancel();
	iTimer.Close();
	}

CMobblerSleepTimer* CMobblerSleepTimer::NewL(const TInt aPriority,
											 MMobblerSleepTimerNotify& aNotify)
	{
    TRACER_AUTO;
	CMobblerSleepTimer* timer(new (ELeave) CMobblerSleepTimer(aPriority, 
																aNotify));
	CleanupStack::PushL(timer);
	timer->ConstructL();
	CleanupStack::Pop();
    return timer;
	}

void CMobblerSleepTimer::ConstructL(void)
	{
    TRACER_AUTO;
	CActiveScheduler::Add(this);
	iTimer.CreateLocal();
	}

void CMobblerSleepTimer::After(TTimeIntervalMicroSeconds32 aInterval)
	{
    TRACER_AUTO;
	Cancel();
	iTimer.After(iStatus, aInterval);
	SetActive();
	}

void CMobblerSleepTimer::At(const TTime& aTime)
	{
    TRACER_AUTO;
	Cancel();
	iTimer.At(iStatus, aTime);
	SetActive();
	}

void CMobblerSleepTimer::AtUTC(const TTime& aUtcTime)
	{
    TRACER_AUTO;
	Cancel();
	iTimer.AtUTC(iStatus, aUtcTime);
	SetActive();
	}

void CMobblerSleepTimer::Inactivity(TTimeIntervalSeconds aSeconds)
	{
    TRACER_AUTO;
	Cancel();
	iTimer.Inactivity(iStatus, aSeconds);
	SetActive();
	}

void CMobblerSleepTimer::DoCancel()
	{
    TRACER_AUTO;
	iTimer.Cancel();
	}
 
void CMobblerSleepTimer::RunL()
	{
    TRACER_AUTO;
	iNotify.TimerExpiredL(this, iStatus.Int());
	}

// End of file
