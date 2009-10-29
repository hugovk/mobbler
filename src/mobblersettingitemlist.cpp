/*
mobblersettingitemlist.cpp

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

#include <aknview.h>
#include <aknviewappui.h>

#include "mobblerappui.h"
#include "mobblersettingitemlist.h"

CMobblerSettingItemList::CMobblerSettingItemList(CMobblerSettingItemListSettings& aSettings, MEikCommandObserver* aCommandObserver)
	:iSettings(aSettings), iCommandObserver(aCommandObserver)
	{
	}


CMobblerSettingItemList::~CMobblerSettingItemList()
	{
	}

void CMobblerSettingItemList::EditItemL(TInt aIndex, TBool aCalledFromMenu)
	{
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
	(*SettingItemArray())[aIndex]->StoreL();
	}

void CMobblerSettingItemList::HandleResourceChange(TInt aType)
	{
	CAknSettingItemList::HandleResourceChange(aType);
	SetRect(iAvkonViewAppUi->View(TUid::Uid(KMobblerSettingsViewUid))->ClientRect());
	}

TKeyResponse CMobblerSettingItemList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	if ( aKeyEvent.iCode == EKeyLeftArrow 
		|| aKeyEvent.iCode == EKeyRightArrow )
		{
		// allow the tab control to get the arrow keys
		return EKeyWasNotConsumed;
		}
	
	return CAknSettingItemList::OfferKeyEventL(aKeyEvent, aType);
	}

void CMobblerSettingItemList::SizeChanged()
	{
	}

// End of file
