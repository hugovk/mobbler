/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008  Michael Coffey

http://code.google.com/p/mobbler

This file is part of Mobbler.

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MOBBLERTOUCHFEEDBACK_H__
#define __MOBBLERTOUCHFEEDBACK_H__

#include <e32base.h>
#include <mobbler\mobblertouchfeedbackinterface.h>

#ifdef  __S60_50__
#include <touchfeedback.h>
#endif

class CMobblerTouchFeedback : public CMobblerTouchFeedbackInterface
	{
public:
	static CMobblerTouchFeedback* NewL();
	~CMobblerTouchFeedback();
	
private:
	CMobblerTouchFeedback();
	void ConstructL();

private: // from CMobblerTouchFeedbackInterface
	void InstantFeedback(TInt aType);

private:
#ifdef  __S60_50__
	MTouchFeedback* iTouchFeedback;
#endif
	};

#endif // __MOBBLERTOUCHFEEDBACK_H__
