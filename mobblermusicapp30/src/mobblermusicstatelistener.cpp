/*
mobblermusicstatelistener.cpp

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

#include <e32base.h>

#include "mobblermusicstatelistener.h"

const TUid KMusicAppUid = {0x102072c3};
const TInt KStateKey(3);

CMobblerMusicAppStateListener* CMobblerMusicAppStateListener::NewL(MMobblerMusicAppStateObserver& aObserver)
	{
	CMobblerMusicAppStateListener* self(new(ELeave) CMobblerMusicAppStateListener(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppStateListener::~CMobblerMusicAppStateListener()
	{
	Cancel();
	iProperty.Close();
	}

CMobblerMusicAppStateListener::CMobblerMusicAppStateListener(MMobblerMusicAppStateObserver& aObserver)
	:CActive(EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerMusicAppStateListener::ConstructL()
	{
	User::LeaveIfError(iProperty.Attach(KMusicAppUid, KStateKey));
	iProperty.Subscribe(iStatus);
	SetActive();
	}

void CMobblerMusicAppStateListener::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		iProperty.Subscribe(iStatus);
		SetActive();
		
		TInt state(0);
		iProperty.Get(state);
		iObserver.HandleMusicStateChangeL(state);
		}
	}
void CMobblerMusicAppStateListener::DoCancel()
	{
	iProperty.Close();
	}

// End of file
