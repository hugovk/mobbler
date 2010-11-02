/*
mobblerincomingcallmonitor.h

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

#ifndef __MOBBLERINCOMINGCALLMONITOR_H__
#define __MOBBLERINCOMINGCALLMONITOR_H__

#include <e32base.h>
#include <e32property.h>

#include "mobblerincomingcallmonitorobserver.h" 

class CMobblerIncomingCallMonitor : public CActive
	{
public:
	static CMobblerIncomingCallMonitor* NewL(MMobblerIncomingCallMonitorObserver& aObserver);
	~CMobblerIncomingCallMonitor();
	
private:
	CMobblerIncomingCallMonitor(MMobblerIncomingCallMonitorObserver& aObserver);
	void ConstructL();
	
private:
	void RunL();
	void DoCancel();
	
private:
	RProperty iProperty;
	MMobblerIncomingCallMonitorObserver& iObserver;
	};
	
#endif // __MOBBLERINCOMINGCALLMONITOR_H__
