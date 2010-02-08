/*
mobblersettinglistslider.cpp

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

#include <AknSlider.h>

#include "mobblerappui.h"
#include "mobbler_strings.rsg.h"
#include "mobblerresourcereader.h"
#include "mobblerslidersettingitem.h"

CMobblerSliderSettingItem::CMobblerSliderSettingItem(TInt aIdentifier,
													 TInt& aSliderValue,
													 TInt aResourceId)
	: CAknSliderSettingItem(aIdentifier, aSliderValue),
	  iResourceIdSingular(aResourceId),
	  iResourceIdPlural(aResourceId)
  	{
  	}

CMobblerSliderSettingItem::CMobblerSliderSettingItem(TInt aIdentifier,
													 TInt& aSliderValue,
													 TInt aResourceIdSingular,
													 TInt aResourceIdPlural)
	: CAknSliderSettingItem(aIdentifier, aSliderValue),
	  iResourceIdSingular(aResourceIdSingular),
	  iResourceIdPlural(aResourceIdPlural)
	{
	}

void CMobblerSliderSettingItem::CreateAndExecuteSettingPageL()
	{
	CAknSettingPage* dlg(CreateSettingPageL());

	SetSettingPage(dlg);
	SettingPage()->SetSettingPageObserver(this);

	// This has to be called so that slider setting page will work correctly in
	// all environments (WINS UDEB, GCCE UREL, etc)
	SettingPage()->SetSettingTextL(SettingName());

	SettingPage()->ExecuteLD(CAknSettingPage::EUpdateWhenChanged);

	SetSettingPage(0);
	}

CFbsBitmap* CMobblerSliderSettingItem::CreateBitmapL()
	{
	return NULL;
	}

const TDesC& CMobblerSliderSettingItem::SettingTextL()
	{
	iText.Format(InternalSliderValue() == 1?
			static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(iResourceIdSingular):
			static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(iResourceIdPlural),
			InternalSliderValue());
	
	return iText;
	}

// End of file
