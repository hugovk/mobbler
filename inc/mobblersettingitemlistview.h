/*
mobblersettingitemlistview.h

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

#ifndef __MOBBLERSETTINGITEMLISTVIEW_H__
#define __MOBBLERSETTINGITEMLISTVIEW_H__

#include <aknview.h>

#include "mobblersettingitemlistsettings.h"

class CAknTextSettingItem;
class CMobblerResourceReader;
class CMobblerSettingItemList;

class CMobblerSettingItemListView : public CAknView
	{
public:
	enum TSettingsToSet
		{
		ENormalSettings,
		ESleepTimer,
		EAlarm
		};
	
public:
	static CMobblerSettingItemListView* NewL();
	~CMobblerSettingItemListView();
	
	TUid Id() const;
	void HandleCommandL(TInt aCommand);
	
	CMobblerSettingItemListSettings& Settings() { return *iSettings; }
	
private:
	CMobblerSettingItemListView();        
	void ConstructL();

	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void DoDeactivate();
	void HandleStatusPaneSizeChange();
	
	TBool HandleChangeSelectedSettingItemL(TInt aCommand);
	
	void LoadListL();
	void CreateTextItemL(TDes& aText, 
							const TInt aTitleResource, 
							const TInt aPageResource);
	void CreatePasswordItemL(TDes& aPassword, 
							const TInt aTitleResource, 
							const TInt aPageResource);
	void CreateIapItemL(TInt& aIapId, 
							const TInt aTitleResource, 
							const TInt aPageResource,
							const TBool aAlwaysAsk = ETrue);
	void CreateSliderItemL(TInt& aSliderValue, 
							const TInt aTitleResource, 
							const TInt aPageResource, 
							const TInt aResourceSingular, 
							const TInt aResourcePlural);
	void CreateBinaryItemL(TBool& aBinaryValue, 
							const TInt aTitleResource, 
							const TInt aPageResource,
							const TInt aFirstEnumResource,
							const TInt aSecondEnumResource);
	void CreateEnumItemL(TInt& aEnumId,
							const TInt aTitleResource,
							const TInt aPageResource,
							RArray<TInt>& aEnumResources);
	void CreateTimeItemL(TTime& aTime,
							const TInt aTitleResource,
							const TInt aPageResource);
	void CreateVolumeItemL(TInt& aVolume,
							const TInt aTitleResource,
							const TInt aPageResource);
	void CreateIntegerItemL(TInt& aIntegerValue, 
							const TInt aTitleResource, 
							const TInt aPageResource);

private:
	CMobblerSettingItemList* iMobblerSettingItemList;
	CMobblerSettingItemListSettings* iSettings;
	
	TInt iOrdinal;
	TBool iIsNumberedStyle;
	CArrayPtr<CGulIcon>* iIcons;
	
	TSettingsToSet iSettingsToSet;
	};

#endif

// End of file
