/*
mobblertimeout.cpp

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
#include "mobblertimeout.h"
#include "mobblertracer.h"

CMobblerTimeout* CMobblerTimeout::NewL(const TTimeIntervalMicroSeconds32& aTimeoutTime)
	{
    TRACER_AUTO;
	CMobblerTimeout* self(new(ELeave) CMobblerTimeout(aTimeoutTime));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTimeout::CMobblerTimeout(const TTimeIntervalMicroSeconds32& aTimeoutTime)
	:CActive(CActive::EPriorityStandard), iTimeoutTime(aTimeoutTime)
	{
    TRACER_AUTO;
	CActiveScheduler::Add(this);
	}

void CMobblerTimeout::ConstructL()
	{
    TRACER_AUTO;
	User::LeaveIfError(iTimer.CreateLocal());
	}

CMobblerTimeout::~CMobblerTimeout()
	{
    TRACER_AUTO;
	Cancel();
	iTimer.Close();
	}
	
void CMobblerTimeout::Reset()
	{
    TRACER_AUTO;
	Cancel();
	iTimer.After(iStatus, iTimeoutTime);
	SetActive();
	}

TBool CMobblerTimeout::TimedOut() const
	{
    TRACER_AUTO;
	return !IsActive();
	}
	
void CMobblerTimeout::RunL()
	{
    TRACER_AUTO;
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerTimeout::DoCancel()
	{
    TRACER_AUTO;
	iTimer.Cancel();
	}

// End of file
