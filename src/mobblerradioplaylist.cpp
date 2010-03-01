/*
mobblerradioplaylist.cpp

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

#include "mobblerradioplaylist.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"

CMobblerRadioPlaylist* CMobblerRadioPlaylist::NewL()
	{
    TRACER_AUTO;
	CMobblerRadioPlaylist* self(new(ELeave) CMobblerRadioPlaylist());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerRadioPlaylist::CMobblerRadioPlaylist()
	{
    TRACER_AUTO;
	}

void CMobblerRadioPlaylist::ConstructL()
	{
    TRACER_AUTO;
	}

CMobblerRadioPlaylist::~CMobblerRadioPlaylist()
	{
    TRACER_AUTO;
	Reset();
	}

void CMobblerRadioPlaylist::AppendTrackL(CMobblerTrack* aTrack)
	{
    TRACER_AUTO;
	iPlaylist.AppendL(aTrack);
	}

void CMobblerRadioPlaylist::RemoveAndReleaseTrack(TInt aTrackIndex)
	{
    TRACER_AUTO;
	iPlaylist[aTrackIndex]->Release();
	iPlaylist.Remove(aTrackIndex);
	}

void CMobblerRadioPlaylist::Reset()
	{
    TRACER_AUTO;
	for (TInt i(iPlaylist.Count() - 1) ; i >= 0 ; --i)
		{
		iPlaylist[i]->Release();
		}
	
	iPlaylist.Reset();
	}

TInt CMobblerRadioPlaylist::Count() const
	{
    TRACER_AUTO;
	return iPlaylist.Count();
	}

const CMobblerTrack* CMobblerRadioPlaylist::operator[](TInt aCount) const
	{
	return iPlaylist[aCount];
	}

CMobblerTrack* CMobblerRadioPlaylist::operator[](TInt aCount)
	{
	return iPlaylist[aCount];
	}

// End of file
