/*
mobblerlastfmconnectionobserver.h

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

#ifndef __MOBBLERLASTFMCONNECTIONOBSERVER_H__
#define __MOBBLERLASTFMCONNECTIONOBSERVER_H__

#include "mobblerlastfmconnection.h"
#include "mobblerlastfmerror.h"

class CMobblerTrack;

class MMobblerLastFmConnectionObserver
	{
public:
	virtual void HandleConnectCompleteL(TInt aError) = 0;
	virtual void HandleLastFmErrorL(CMobblerLastFmError& aError) = 0;
	virtual void HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus) = 0;
	
	virtual void HandleTrackNowPlayingL(const CMobblerTrackBase& aTrack) = 0;
	virtual void HandleTrackQueuedL(const CMobblerTrackBase& aTrack) = 0;
	virtual void HandleTrackDequeued(const CMobblerTrackBase& aTrack) = 0;
	virtual void HandleTrackSubmitted(const CMobblerTrackBase& aTrack) = 0;
	
	virtual TBool GoOnlineL() = 0;
	};
	
#endif
	
// End of file
