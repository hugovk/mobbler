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

CMobblerSleepTimer::CMobblerSleepTimer(const TInt aPriority, 
									   MMobblerSleepTimerNotify& aNotify)
:CActive(aPriority), iNotify(aNotify)
	{
	}

CMobblerSleepTimer::~CMobblerSleepTimer()
	{
	Cancel();
	iTimer.Close();
	}

CMobblerSleepTimer* CMobblerSleepTimer::NewL(const TInt aPriority,
											 MMobblerSleepTimerNotify& aNotify)
	{
	CMobblerSleepTimer* timer(new (ELeave) CMobblerSleepTimer(aPriority, 
																aNotify));
	CleanupStack::PushL(timer);
	timer->ConstructL();
	CleanupStack::Pop();
    return timer;
	}

void CMobblerSleepTimer::ConstructL(void)
	{
	CActiveScheduler::Add(this);
	iTimer.CreateLocal();
	}

void CMobblerSleepTimer::After(TTimeIntervalMicroSeconds32 aInterval)
	{
	Cancel();
	iTimer.After(iStatus, aInterval);
	SetActive();
	}

void CMobblerSleepTimer::At(const TTime& aTime)
	{
	Cancel();
	iTimer.At(iStatus, aTime);
	SetActive();
	}

void CMobblerSleepTimer::AtUTC(const TTime& aUtcTime)
	{
	Cancel();
	iTimer.AtUTC(iStatus, aUtcTime);
	SetActive();
	}

void CMobblerSleepTimer::Inactivity(TTimeIntervalSeconds aSeconds)
	{
	Cancel();
	iTimer.Inactivity(iStatus, aSeconds);
	SetActive();
	}

void CMobblerSleepTimer::DoCancel()
	{
	iTimer.Cancel();
	}
 
void CMobblerSleepTimer::RunL()
	{
	iNotify.TimerExpiredL(this, iStatus.Int());
	}

// End of file
