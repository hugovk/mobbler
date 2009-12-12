/*
musicplaybackpositionlistener.h

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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

#include "mobblerplaybackpositionlistener.h"

const TUid KMusicAppUID = {0x102072c3};
const TInt KPlaybackPositionKey = 6;

CMobblerPlaybackPositionListener* CMobblerPlaybackPositionListener::NewL(MMobblerPlaybackPositionObserver& aObserver)
	{
	CMobblerPlaybackPositionListener* self = new(ELeave) CMobblerPlaybackPositionListener(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerPlaybackPositionListener::~CMobblerPlaybackPositionListener()
	{
	Cancel();
	iProperty.Close();
	}

CMobblerPlaybackPositionListener::CMobblerPlaybackPositionListener(MMobblerPlaybackPositionObserver& aObserver)
	:CActive(EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerPlaybackPositionListener::ConstructL()
	{
	User::LeaveIfError(iProperty.Attach(KMusicAppUID, KPlaybackPositionKey));
	iProperty.Subscribe(iStatus);
	SetActive();
	}

void CMobblerPlaybackPositionListener::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		iProperty.Subscribe(iStatus);
		SetActive();
		
		TInt playbackPosition(0);
		iProperty.Get(playbackPosition);
		iObserver.HandlePlaybackPositionChangeL(playbackPosition);
		}
	}
void CMobblerPlaybackPositionListener::DoCancel()
	{
	iProperty.Close();
	}
