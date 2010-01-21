/*
mobblerstatusview.h

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

#ifndef __MOBBLERSTATUSVIEW_H__
#define __MOBBLERSTATUSVIEW_H__

#include <aknview.h>

class CMobblerStatusView : public CAknView, public MMobblerBitmapObserver
	{
public:
	static CMobblerStatusView* NewL();
	~CMobblerStatusView();
	
	void DisplayPlusMenuL();
	
	TUid Id() const;
	void HandleCommandL(TInt aCommand);
	
	void DrawDeferred() const;
	void DrawNow() const;
	
	CMobblerStatusControl* StatusControl();
	
private:
	CMobblerStatusView();        
	void ConstructL();
	
	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);

	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void DoDeactivate();
	void HandleStatusPaneSizeChange();
	
	TBool HandleChangeSelectedSettingItemL(TInt aCommand);
	
	void SetupStatusPaneL();
	
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);

	void SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, 
						  TInt aCommandId);
	void SettingsWizardL();
	
private:
	CMobblerStatusControl* iMobblerStatusControl;
	
	TBool iDisplayPlusMenu;
	};

#endif // __MOBBLERSTATUSVIEW_H__

// End of file
