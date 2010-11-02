/*
mobblersettingitemlist.h

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

#ifndef __MOBBLERSETTINGITEMLIST_H__
#define __MOBBLERSETTINGITEMLIST_H__

#include <aknsettingitemlist.h>

class MEikCommandObserver;
class CMobblerSettingItemListSettings;

class CMobblerSettingItemList : public CAknSettingItemList
	{
public:
	CMobblerSettingItemList(CMobblerSettingItemListSettings& settings, MEikCommandObserver* aCommandObserver);
	~CMobblerSettingItemList();
	
private:
	void ChangeSelectedItemL();
	
private: // from CCoeControl
	void HandleResourceChange(TInt aType);
	
private: // from CAknSettingItemList
	void SizeChanged();
	void EditItemL(TInt aIndex, TBool aCalledFromMenu);
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

private:
	CMobblerSettingItemListSettings& iSettings;
	MEikCommandObserver* iCommandObserver;
	};
	
#endif // __MOBBLERSETTINGITEMLIST_H__
