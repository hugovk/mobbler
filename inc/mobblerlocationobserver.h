/*
mobblerlocationobserver.h

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2010  Michael Coffey

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

#ifndef __MOBBLERLOCATIONOBSERVER_H__
#define __MOBBLERLOCATIONOBSERVER_H__

#include <e32base.h>

class MMobblerLocationObserver
	{
public:
	virtual void HandleLocationCompleteL(const TDesC8& aAccuracy,
											const TDesC8& aLatitude,
											const TDesC8& aLongitude,
											const TDesC8& aName) = 0;
	};

#endif // __MOBBLERLOCATIONOBSERVER_H__

// End of file
