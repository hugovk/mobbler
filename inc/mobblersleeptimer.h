/*
mobblersleeptimer.h

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

#ifndef __MOBBLERSLEEPTIMER_H__
#define __MOBBLERSLEEPTIMER_H__

#include <e32base.h>
 
class MMobblerSleepTimerNotify
{
public:
	virtual void TimerExpiredL(TAny* aTimer, TInt aError) = 0;
	};

class CMobblerSleepTimer: public CActive
	{
public:
	static CMobblerSleepTimer* NewL(const TInt aPriority, 
									MMobblerSleepTimerNotify& aNotify);
	~CMobblerSleepTimer();
public:
	void At(const TTime& aTime);
	void AtUTC(const TTime& aUtcTime);
	void After(TTimeIntervalMicroSeconds32 aInterval);
	void Inactivity(TTimeIntervalSeconds aSeconds);

protected:
	void RunL();
	void DoCancel();

private:
	CMobblerSleepTimer(const TInt aPriority, MMobblerSleepTimerNotify& aNotify);
	void ConstructL(void);

private:
	RTimer iTimer;
	MMobblerSleepTimerNotify& iNotify;
	TTime iTime;
};

#endif // __MOBBLERSLEEPTIMER_H__

// End of file
