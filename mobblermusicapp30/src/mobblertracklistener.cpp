/*
mobblertracklistener.cpp

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
#include <e32property.h> 

#include "mobblertracklistener.h"

const TUid KMusicAppUid = {0x102072c3};
const TInt KTrackKey(2);

CMobblerTrackListener* CMobblerTrackListener::NewL(MMobblerTrackObserver& aObserver)
	{
	CMobblerTrackListener* self(new(ELeave) CMobblerTrackListener(aObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrackListener::~CMobblerTrackListener()
	{
	Cancel();
	iProperty.Close();
	}

CMobblerTrackListener::CMobblerTrackListener(MMobblerTrackObserver& aObserver)
	:CActive(EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerTrackListener::ConstructL()
	{
	User::LeaveIfError(iProperty.Attach(KMusicAppUid, KTrackKey));
	iProperty.Subscribe(iStatus);
	SetActive();
	}

void CMobblerTrackListener::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		iProperty.Subscribe(iStatus);
		SetActive();
		
		TBuf<255> track;
		iProperty.Get(track);
		iObserver.HandleTrackChangeL(track);
		}
	}
void CMobblerTrackListener::DoCancel()
	{
	iProperty.Close();
	}

// End of file
