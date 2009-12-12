/*
tracklistener.h

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

#ifndef __TRACKLISTENER_H__
#define __TRACKLISTENER_H__

#include <e32base.h>
#include <e32property.h> 

class CMobblerTrackListener : public CActive
	{
public:
	class MMobblerTrackObserver
		{
	public:
		virtual void HandleTrackChangeL(const TDesC& aTrack) = 0;
		};
	
public:
	static CMobblerTrackListener* NewL(MMobblerTrackObserver& aObserver);
	~CMobblerTrackListener();
	
private:
	CMobblerTrackListener(MMobblerTrackObserver& aObserver);
	void ConstructL();

private: // from CActive
	void RunL();
	void DoCancel();
	
private:
	RProperty iProperty;
	MMobblerTrackObserver& iObserver;
	};

#endif // __TRACKLISTENER_H__
