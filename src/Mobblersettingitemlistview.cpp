/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade
Copyright (C) 2009  Steve Punter
Copyright (C) 2009  James Aley
Copyright (C) 2010  gw111zz

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

#include <AknTextSettingPage.h>
#include <eikfrlbd.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobbleraccesspointsettingitem.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlist.h"
#include "mobblersettingitemlistview.h"
#include "mobblerslidersettingitem.h"
#include "mobblertracer.h"

CMobblerSettingItemListView* CMobblerSettingItemListView::NewL()
	{
    TRACER_AUTO;
	CMobblerSettingItemListView* self(new(ELeave) CMobblerSettingItemListView);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerSettingItemListView::CMobblerSettingItemListView()
	: iOrdinal(1)
	{
    TRACER_AUTO;
	}

CMobblerSettingItemListView::~CMobblerSettingItemListView()
	{
    TRACER_AUTO;
	delete iMobblerSettingItemList;
	delete iSettings;
	}

void CMobblerSettingItemListView::ConstructL()
	{
    TRACER_AUTO;
	BaseConstructL(R_MOBBLER_SETTING_ITEM_LIST_VIEW);
	iSettings = CMobblerSettingItemListSettings::NewL();
	iSettings->LoadSettingValuesL();
	}

TUid CMobblerSettingItemListView::Id() const
	{
    TRACER_AUTO;
	return TUid::Uid(KMobblerSettingsViewUid);
	}

void CMobblerSettingItemListView::HandleCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	// let the app ui handle the event
	if (aCommand == EAknSoftkeyOk)
		{
		// save and set details then switch back to the status view
		iSettings->SaveSettingValuesL();
		if (iSettingsToSet == ENormalSettings)
			{
			static_cast<CMobblerAppUi*>(AppUi())->SetDetailsL(iSettings->Username(), iSettings->Password());
			static_cast<CMobblerAppUi*>(AppUi())->SetIapIDL(iSettings->IapId());
			static_cast<CMobblerAppUi*>(AppUi())->SetBufferSize(iSettings->BufferSize());
			static_cast<CMobblerAppUi*>(AppUi())->UpdateAccelerometerGesturesL();
			static_cast<CMobblerAppUi*>(AppUi())->SetBitRateL(iSettings->BitRate());
			}
		else if (iSettingsToSet == ESleepTimer)
			{
			static_cast<CMobblerAppUi*>(AppUi())->SetSleepTimerL(iSettings->SleepTimerMinutes());
			}
		else if (iSettingsToSet == EAlarm)
			{
			static_cast<CMobblerAppUi*>(AppUi())->SetAlarmTimerL(iSettings->AlarmTime());
			}
		AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid));
		}
	else if (aCommand == EAknSoftkeyCancel)
		{
		// reset the details then switch back to the status view
		iSettings->LoadSettingValuesL();
		AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid));
		}
	else if (aCommand == EMobblerCommandRemove)
		{
		// reset the details then switch back to the status view
		iSettings->LoadSettingValuesL();
		
		// TODO or send custom message to remove sleep/alarm?
		if (iSettingsToSet == ESleepTimer)
			{
			static_cast<CMobblerAppUi*>(AppUi())->RemoveSleepTimerL();
			}
		else if (iSettingsToSet == EAlarm)
			{
			static_cast<CMobblerAppUi*>(AppUi())->RemoveAlarmL();
			}
		
		AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid));
		}
	else
		{
		AppUi()->HandleCommandL(aCommand);
		}
	}

void CMobblerSettingItemListView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/)
	{
    TRACER_AUTO;
	iSettingsToSet = (TSettingsToSet)aCustomMessageId.iUid;
	
	if ((iSettingsToSet == ESleepTimer && 
		static_cast<CMobblerAppUi*>(AppUi())->SleepTimerActive())
			||
		(iSettingsToSet == EAlarm && 
		static_cast<CMobblerAppUi*>(AppUi())->AlarmActive()))
		{
		// Change the Cancel softkey to Remove
		TInt pos(Cba()->PositionById(EAknSoftkeyCancel));
		if (pos != KErrNotFound)
			{
			Cba()->AddCommandToStackL(pos, EMobblerCommandRemove, 
				static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(R_MOBBLER_SOFTKEY_REMOVE));
			Cba()->DrawNow();
			}
		}
	else
		{
		TInt pos(Cba()->PositionById(EMobblerCommandRemove));
		if (pos != KErrNotFound)
			{
			Cba()->RemoveCommandFromStack(pos, EMobblerCommandRemove);
			Cba()->DrawNow();
			}
		}
	
	if (!iMobblerSettingItemList)
		{
		iMobblerSettingItemList = new (ELeave) CMobblerSettingItemList(*iSettings, this);
		iMobblerSettingItemList->SetMopParent(this);
		iMobblerSettingItemList->ConstructFromResourceL(R_MOBBLER_SETTING_ITEM_LIST);
		iMobblerSettingItemList->LoadSettingsL();
		
		iSettings->LoadSettingValuesL();
		LoadListL();
		iMobblerSettingItemList->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerSettingItemList);
		}
	}

void CMobblerSettingItemListView::DoDeactivate()
	{
    TRACER_AUTO;
	if (iMobblerSettingItemList)
		{
		AppUi()->RemoveFromStack(iMobblerSettingItemList);
		delete iMobblerSettingItemList;
		iMobblerSettingItemList = NULL;
		}
	}

void CMobblerSettingItemListView::HandleStatusPaneSizeChange()
	{
    TRACER_AUTO;
	CAknView::HandleStatusPaneSizeChange();
	}

TBool CMobblerSettingItemListView::HandleChangeSelectedSettingItemL(TInt /*aCommand*/)
	{
    TRACER_AUTO;
	return ETrue;
	}

void CMobblerSettingItemListView::LoadListL()
	{
    TRACER_AUTO;
	iIsNumberedStyle = iMobblerSettingItemList->IsNumberedStyle();
	iIcons = iMobblerSettingItemList->ListBox()->ItemDrawer()->FormattedCellData()->IconArray();

	if (iSettingsToSet == ENormalSettings)
		{
		// Username text setting item
		CreateTextItemL(iSettings->Username(),
						R_MOBBLER_USERNAME, 
						R_MOBBLER_SETTING_PAGE_USERNAME);
		
		// Password setting item
		CreatePasswordItemL(iSettings->Password(),
							R_MOBBLER_PASSWORD,
							R_MOBBLER_SETTING_PAGE_PASSWORD);
		
		// IAP enumerated text setting item
		CreateIapItemL(iSettings->IapId(),
					   R_MOBBLER_IAP,
					   R_MOBBLER_SETTING_PAGE_ENUM);
		
		// Bitrate (64 kbps or 128 kbps)
		CreateBinaryItemL(iSettings->BitRate(),
						  R_MOBBLER_BIT_RATE,
						  R_MOBBLER_BINARY_SETTING_PAGE,
						  R_MOBBLER_64_KBPS,
						  R_MOBBLER_128_KBPS);
		
		// Buffer size slider setting item
		CreateSliderItemL(iSettings->BufferSize(),
						  R_MOBBLER_BUFFER_SIZE,
						  R_MOBBLER_SLIDER_SETTING_PAGE_BUFFER_SIZE,
						  R_MOBBLER_BUFFER_SIZE_SECOND,
						  R_MOBBLER_BUFFER_SIZE_SECONDS);
		
		// Download album art enumerated setting item
		RArray<TInt> downloadAlbumArtArray;
		CleanupClosePushL(downloadAlbumArtArray);
		downloadAlbumArtArray.AppendL(R_MOBBLER_DOWNLOAD_ALBUM_ART_NEVER);
		downloadAlbumArtArray.AppendL(R_MOBBLER_DOWNLOAD_ALBUM_ART_RADIO_ONLY);
		downloadAlbumArtArray.AppendL(R_MOBBLER_DOWNLOAD_ALBUM_ART_ALWAYS_AND_KEEP);
		downloadAlbumArtArray.AppendL(R_MOBBLER_DOWNLOAD_ALBUM_ART_ALWAYS_AND_DITCH);
		CreateEnumItemL(iSettings->DownloadAlbumArt(),
						R_MOBBLER_DOWNLOAD_ALBUM_ART,
						R_MOBBLER_SETTING_PAGE_ENUM,
						downloadAlbumArtArray);
		CleanupStack::PopAndDestroy(&downloadAlbumArtArray);
		
		// Automatic wallpaper binary popup setting item
#ifdef __SYMBIAN_SIGNED__
		CreateBinaryItemL(iSettings->AutomaticWallpaper(),
						  R_MOBBLER_AUTOMATICALLY_SET_WALLPAPER,
						  R_MOBBLER_BINARY_SETTING_PAGE,
						  R_MOBBLER_SET_WALLPAPER_MANUALLY,
						  R_MOBBLER_SET_WALLPAPER_AUTOMATICALLY);
#endif
		
		// Scrobble percent slider setting item
		CreateSliderItemL(iSettings->ScrobblePercent(),
						  R_MOBBLER_SCROBBLE_PERCENT,
						  R_MOBBLER_SLIDER_SETTING_PAGE_SCROBBLE_PERCENT,
						  R_MOBBLER_PERCENT,
						  R_MOBBLER_PERCENT);
		
		// Check for updates binary popup setting item
		CreateBinaryItemL(iSettings->CheckForUpdates(),
						  R_MOBBLER_CHECK_FOR_UPDATES_ONCE_A_WEEK,
						  R_MOBBLER_BINARY_SETTING_PAGE,
						  R_MOBBLER_CHECK_FOR_UPDATES_NO,
						  R_MOBBLER_CHECK_FOR_UPDATES_YES);
		
		// Backlight binary popup setting item
		CreateBinaryItemL(iSettings->Backlight(),
						  R_MOBBLER_BACKLIGHT,
						  R_MOBBLER_BINARY_SETTING_PAGE,
						  R_MOBBLER_BACKLIGHT_SYSTEM_DEFAULT,
						  R_MOBBLER_BACKLIGHT_ON_WHEN_ACTIVE);
		
		// Accelerometer gestures binary popup setting item
		if (static_cast<CMobblerAppUi*>(AppUi())->AccelerometerGesturesAvailable())
			{
			CreateBinaryItemL(iSettings->AccelerometerGestures(),
								  R_MOBBLER_ACCELEROMETER_GESTURES,
								  R_MOBBLER_BINARY_SETTING_PAGE,
								  R_MOBBLER_ACCELEROMETER_GESTURES_OFF,
								  R_MOBBLER_ACCELEROMETER_GESTURES_ON);
			}
		}
	else if (iSettingsToSet == ESleepTimer)
		{
		// Minutes until sleep setting item
		if (iSettings->SleepTimerMinutes() < 1)
			{
			iSettings->SetSleepTimerMinutes(1);
			}
		CreateIntegerItemL(iSettings->SleepTimerMinutes(),
						  R_MOBBLER_SLEEP_TIMER_TIME_MINUTES,
						  R_MOBBLER_INTEGER_SETTING_PAGE_SLEEP_MINUTES);
		
		// Sleep timer action enumerated setting item
		RArray<TInt> array;
		CleanupClosePushL(array);
		array.AppendL(R_MOBBLER_SLEEP_TIMER_ACTION_STOP);
		array.AppendL(R_MOBBLER_SLEEP_TIMER_ACTION_OFFLINE);
		array.AppendL(R_MOBBLER_SLEEP_TIMER_ACTION_EXIT);
		
		CreateEnumItemL(iSettings->SleepTimerAction(),
						R_MOBBLER_SLEEP_TIMER_SETTING_ACTION,
						R_MOBBLER_SETTING_PAGE_ENUM,
						array);
		
		CleanupStack::PopAndDestroy(&array);
		
		// Sleep immediacy binary setting item
		CreateBinaryItemL(iSettings->SleepTimerImmediacy(),
						  R_MOBBLER_SLEEP_TIMER_IMMEDIACY,
						  R_MOBBLER_BINARY_SETTING_PAGE,
						  R_MOBBLER_SLEEP_TIMER_IMMEDIATE,
						  R_MOBBLER_SLEEP_TIMER_END_OF_TRACK);
		}
	else if (iSettingsToSet == EAlarm)
		{
		// Alarm time setting item
		CreateTimeItemL(iSettings->AlarmTime(),
						R_MOBBLER_ALARM_PROMPT,
						R_MOBBLER_TIME_SETTING_PAGE);
		
		// Alarm IAP enumerated text setting item
		CreateIapItemL(iSettings->AlarmIapId(),
					   R_MOBBLER_IAP,
					   R_MOBBLER_SETTING_PAGE_ENUM,
					   EFalse);
		
		// Alarm volume seting item
		CreateVolumeItemL(iSettings->AlarmVolume(),
						  R_MOBBLER_VOLUME,
						  R_MOBBLER_VOLUME_SETTING_PAGE);
		
		// Alarm station enumerated setting item
		RArray<TInt> alarmStationArray;
		CleanupClosePushL(alarmStationArray);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_ARTIST);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_TAG);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_USER);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_RECOMMENDATIONS);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_PERSONAL);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_LOVED);
		alarmStationArray.AppendL(R_MOBBLER_RADIO_NEIGHBOURHOOD);
		CreateEnumItemL(iSettings->AlarmStation(),
						R_MOBBLER_STATION,
						R_MOBBLER_SETTING_PAGE_ENUM,
						alarmStationArray);
		CleanupStack::PopAndDestroy(&alarmStationArray);
		
		// Alarm option text setting item
		CreateTextItemL(iSettings->AlarmOption(),
						R_MOBBLER_STATION_TEXT,
						R_MOBBLER_SETTING_PAGE_TEXT);
		}
	
	// Required when there is only one setting item
	iMobblerSettingItemList->SettingItemArray()->RecalculateVisibleIndicesL();

	iMobblerSettingItemList->HandleChangeInItemArrayOrVisibilityL();
	}

void CMobblerSettingItemListView::CreateTextItemL(TDes& aText,
												  const TInt aTitleResource, 
												  const TInt aPageResource)
	{
    TRACER_AUTO;
	CAknTextSettingItem* item(new (ELeave) CAknTextSettingItem(iOrdinal, aText));
	CleanupStack::PushL(item);
	
	const TDesC& text(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	item->SetEmptyItemTextL(text);
	item->ConstructL(iIsNumberedStyle, iOrdinal, text, iIcons, aPageResource, -1);
	item->SetSettingPageFlags(CAknTextSettingPage::EPredictiveTextEntryPermitted);
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreatePasswordItemL(TDes& aPassword,
													  const TInt aTitleResource, 
													  const TInt aPageResource)
	{
    TRACER_AUTO;
	CAknPasswordSettingItem* item(new (ELeave) CAknPasswordSettingItem(
						iOrdinal, CAknPasswordSettingItem::EAlpha, aPassword));
	CleanupStack::PushL(item);
	
	const TDesC& title(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	
	item->SetEmptyItemTextL(title);
	item->ConstructL(iIsNumberedStyle, iOrdinal, title, iIcons, aPageResource, -1);
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateIapItemL(TInt& aIapId, 
												 const TInt aTitleResource, 
												 const TInt aPageResource,
												 const TBool aAlwaysAsk)
	{
    TRACER_AUTO;
	// To avoid "Setting Item Lis 6" panic
	TInt tempIapId(aIapId);
	aIapId = 0;
	
	CMobblerAccessPointSettingItem* item(new (ELeave) 
			CMobblerAccessPointSettingItem(iOrdinal, aIapId));
	CleanupStack::PushL(item);
	
	// The same resource ID can be used for multiple enumerated text setting pages
	const TDesC& text(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	item->ConstructL(iIsNumberedStyle, iOrdinal, text, iIcons, 
				aPageResource, -1, 0, R_MOBBLER_POPUP_SETTING_TEXTS_ENUM);
	
	CArrayPtr<CAknEnumeratedText>* texts(item->EnumeratedTextArray());
	texts->ResetAndDestroy();
	CAknEnumeratedText* enumText;
	
	if (aAlwaysAsk)
		{
		// "Always ask" text
		enumText = new (ELeave) CAknEnumeratedText(0, static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ALWAYS_ASK).AllocLC());
		CleanupStack::Pop();
		CleanupStack::PushL(enumText);
		texts->AppendL(enumText);
		CleanupStack::Pop(enumText);
		}
	
	// Load list of IAPs
	//TInt firstIapId(item->LoadIapListL());
	item->LoadIapListL();
	
	// Set the real value for the item
/*	if (!aAlwaysAsk && firstIapId != KErrNotFound)
		{
		aIapId = firstIapId;
		}
	else
		{
		aIapId = tempIapId;
		}*/
	aIapId = tempIapId;
	
	// Load list of IAPs
	item->LoadL();
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateSliderItemL(TInt& aSliderValue, 
													const TInt aTitleResource, 
													const TInt aPageResource,
													const TInt aResourceSingular,
													const TInt aResourcePlural)
	{
    TRACER_AUTO;
	CMobblerSliderSettingItem* item(new (ELeave) CMobblerSliderSettingItem(
				iOrdinal, aSliderValue, aResourceSingular, aResourcePlural));
	CleanupStack::PushL(item);
	
	const TDesC& text(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	item->ConstructL(iIsNumberedStyle, iOrdinal, text, iIcons, aPageResource, -1);
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateBinaryItemL(TBool& aBinaryValue, 
													const TInt aTitleResource, 
													const TInt aPageResource,
													const TInt aFirstEnumResource,
													const TInt aSecondEnumResource)
	{
    TRACER_AUTO;
	CAknBinaryPopupSettingItem* item(new (ELeave) 
				CAknBinaryPopupSettingItem(iOrdinal, aBinaryValue));
	CleanupStack::PushL(item);
	
	const TDesC& title(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	// The same resource ID can be used for multiple binary setting pages
	item->ConstructL(iIsNumberedStyle, iOrdinal, title, iIcons, 
				aPageResource, -1, 0, R_MOBBLER_POPUP_SETTING_BINARY_TEXTS);
	
	// Load text dynamically
	CArrayPtr<CAknEnumeratedText>* texts(item->EnumeratedTextArray());
	texts->ResetAndDestroy();
	// Text 1
	const TDesC& text1(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aFirstEnumResource));
	CAknEnumeratedText* enumText(new (ELeave) CAknEnumeratedText(0, text1.AllocLC()));
	CleanupStack::Pop();
	CleanupStack::PushL(enumText);
	texts->AppendL(enumText);
	CleanupStack::Pop(enumText);
	// Text 2
	const TDesC& text2(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aSecondEnumResource));
	enumText = new (ELeave) CAknEnumeratedText(1, text2.AllocLC());
	CleanupStack::Pop();
	CleanupStack::PushL(enumText);
	texts->AppendL(enumText);
	CleanupStack::Pop(enumText);
	
	// Set the correct text visible
	item->LoadL();
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateEnumItemL(TInt& aEnumId, 
												 const TInt aTitleResource, 
												 const TInt aPageResource,
												 RArray<TInt>& aEnumResources)
	{
    TRACER_AUTO;
	// To avoid "Setting Item Lis 6" panic. If it occurs, double check settings
	// are loaded from file in the same order they're saved.
	TInt tempEnumId(aEnumId);
	aEnumId = 0;
	
	CAknEnumeratedTextPopupSettingItem* item(new (ELeave) 
						CAknEnumeratedTextPopupSettingItem(iOrdinal, aEnumId));
	CleanupStack::PushL(item);
	
	// The same resource ID can be used for multiple enumerated text setting pages
	const TDesC& title(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	item->ConstructL(iIsNumberedStyle, iOrdinal, title, iIcons, aPageResource, 
									-1, 0, R_MOBBLER_POPUP_SETTING_TEXTS_ENUM);
	
	CArrayPtr<CAknEnumeratedText>* texts(item->EnumeratedTextArray());
	texts->ResetAndDestroy();
	CAknEnumeratedText* enumText;

	for (TInt i(0); i < aEnumResources.Count(); ++i)
		{
		const TDesC& text(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aEnumResources[i]));
		enumText = new (ELeave) CAknEnumeratedText(i, text.AllocLC());
		CleanupStack::Pop();
		CleanupStack::PushL(enumText);
		texts->AppendL(enumText);
		CleanupStack::Pop(enumText);
		}
	
	// Set the real value for the item
	aEnumId = tempEnumId;
	// Tell the control to load in the value
	item->LoadL();
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateTimeItemL(TTime& aValue, 
													const TInt aTitleResource, 
													const TInt aPageResource)
	{
    TRACER_AUTO;
	CAknTimeOrDateSettingItem* item(new (ELeave) 
				CAknTimeOrDateSettingItem(iOrdinal, CAknTimeOrDateSettingItem::ETime, aValue));
	CleanupStack::PushL(item);
	
	const TDesC& title(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	
	item->SetEmptyItemTextL(title);
	item->ConstructL(iIsNumberedStyle, iOrdinal, title, iIcons, aPageResource, -1);

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateVolumeItemL(TInt& aValue, 
													const TInt aTitleResource, 
													const TInt aPageResource)
	{
    TRACER_AUTO;
	if (aValue < 1)
		{
		aValue = 1;
		}
	
	CAknVolumeSettingItem* item(new (ELeave) CAknVolumeSettingItem(iOrdinal, aValue));
	CleanupStack::PushL(item);
	
	const TDesC& title(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	
	item->SetEmptyItemTextL(title);
	item->ConstructL(iIsNumberedStyle, iOrdinal, title, iIcons, aPageResource, -1);
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateIntegerItemL(TInt& aIntegerValue, 
													const TInt aTitleResource, 
													const TInt aPageResource)
	{
    TRACER_AUTO;
	CAknIntegerEdwinSettingItem* item(new (ELeave) CAknIntegerEdwinSettingItem(
													iOrdinal, aIntegerValue));
	CleanupStack::PushL(item);
	
	const TDesC& text(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(aTitleResource));
	item->ConstructL(iIsNumberedStyle, iOrdinal, text, iIcons, aPageResource, -1);
	
	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

// End of file
