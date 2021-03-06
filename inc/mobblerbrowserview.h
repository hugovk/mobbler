/*
mobblerbrowserview.h

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

#ifndef __MOBBLERBROWSERVIEW_H_
#define __MOBBLERBROWSERVIEW_H_

#include <aknview.h>

class CMobblerBrowserControl;

class CMobblerBrowserView : public CAknView
	{
public:
	static CMobblerBrowserView* NewL();
	~CMobblerBrowserView();

	TUid Id() const;
	void HandleCommandL(TInt aCommand);

private:
	CMobblerBrowserView();
	void ConstructL();

	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);

	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void DoDeactivate();
	void HandleStatusPaneSizeChange();

	void SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, TInt aCommandId);

private:
	CMobblerBrowserControl* iBrowserControl;
	};

#endif //__MOBBLERBROWSERVIEW_H_

// End of file
