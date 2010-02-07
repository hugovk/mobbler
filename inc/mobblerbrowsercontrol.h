/*
mobblerbrowsercontrol.h

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#ifndef __MOBBLERBROWSERCONTROL_H__
#define __MOBBLERBROWSERCONTROL_H__

#include "mobblerbrowsercontrolspecialloadobserver.h"

class CBrCtlInterface;
class CMobblerAppUi;

class CMobblerBrowserControl : public CCoeControl
	{
public:
	static CMobblerBrowserControl* NewL(const TRect& aRect, CMobblerAppUi& aMobblerAppUi, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	~CMobblerBrowserControl();

private:
	CMobblerBrowserControl(CMobblerAppUi& aAppUi);
	void ConstructL(const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage);

	void SizeChanged();

private: // from CCoeControl
	void Draw(const TRect& aRect) const;

	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void HandleResourceChange(TInt aType);

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode);

private:
	CMobblerAppUi& iAppUi;

	TBrowserLoadObserver iBrowserLoadObserver;

	CBrCtlInterface* iBrCtlInterface; // Owned
	};

#endif // __MOBBLERBROWSERCONTROL_H__

// End of file
