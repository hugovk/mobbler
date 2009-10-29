/*
mobblermarquee.h

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

#ifndef __MOBBLERMARQUEE_H__
#define __MOBBLERMARQUEE_H__

#include <e32base.h>

class CMobblerStatusControl;

class CMobblerMarquee : public CBase
	{
private:
	enum TState
		{
		EStart,
		EForward
		};
	
public:
	static CMobblerMarquee* NewL(CMobblerStatusControl& aStatusControl);
	~CMobblerMarquee();
	
	void Start(const TDesC& aText, TInt aInitialOffset, TInt aTextWidth, TInt aDisplayWidth);
	
	TInt GetPosition1() const;
	TInt GetPosition2() const;
	
	void Reset();

private:
	CMobblerMarquee(CMobblerStatusControl& aStatusControl);
	void ConstructL();
	
private:
	static TInt CallBack(TAny* aRef);
	void Update();
	
private:
	CMobblerStatusControl& iStatusControl;
	
	TBuf<255> iText;
	
	TInt iTextWidth;
	TInt iDisplayWidth;
	TInt iInitialOffset;
	
	TUint iStartTickCount;
	
	CPeriodic* iTimer;
	
	TState iState;
	};

#endif // __MOBBLERPARSER_H__
