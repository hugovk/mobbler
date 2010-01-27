/*
mobblerwebservicescontrol.h

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

#ifndef __MOBBLERWEBSERVICESCONTROL_H__
#define __MOBBLERWEBSERVICESCONTROL_H__

#include <e32base.h>

class CAknNavigationControlContainer;
class CAknNavigationDecorator;
class CMobblerAppUi;
class CMobblerListControl;

class CMobblerWebServicesControl : public CCoeControl, public MMobblerConnectionStateObserver
	{
public:
	static CMobblerWebServicesControl* NewL(CMobblerAppUi& aAppUi, const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	~CMobblerWebServicesControl();
	
	CMobblerListControl* TopControl();
	
	void HandleListCommandL(TInt aCommand);
	
	void ForwardL(CMobblerListControl* aListControl);
	void BackL();
	
	void HandleListControlStateChangedL();
	
private:
	CMobblerWebServicesControl(CMobblerAppUi& aAppUi);
	void ConstructL(const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		
	void ChangePaneTextL();
	
private: // from MMobblerConnectionStateObserver
	void HandleConnectionStateChangedL();
	
private: // from CCoeControl
	void Draw(const TRect& aRect) const;
	
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode);
	
private:
	CMobblerAppUi& iAppUi;
	RPointerArray<CMobblerListControl> iControls;
	
	CAknNavigationControlContainer *iNaviContainer;
	CAknNavigationDecorator* iNaviLabelDecorator;
	};

#endif // __MOBBLERWEBSERVICESCONTROL_H__

// End of file
