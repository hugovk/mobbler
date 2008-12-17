/*
mobbleraccesspointsettingitem.cpp

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

#include <commdb.h>

#include "mobbleraccesspointsettingitem.h"

CMobblerAccessPointSettingItem::CMobblerAccessPointSettingItem(TInt aIdentifier, TInt& aValue)
		:CAknEnumeratedTextPopupSettingItem(aIdentifier, aValue), iValue(aValue)
		{
		}

void CMobblerAccessPointSettingItem::LoadL()
	{
	CArrayPtr<CAknEnumeratedText>* texts = this->EnumeratedTextArray();
	TInt selectedIndex = this->IndexFromValue(iValue);
	if (selectedIndex < 0)  // no match found.
		{
		if (texts->Count() > 0)  
			{
			// choose the first item as default one.
			CAknEnumeratedText* item = texts->At(0);
			// reset external value to the default one.
			this->SetExternalValue(item->EnumerationValue());
			}
		}

	CAknEnumeratedTextPopupSettingItem::LoadL();
	}


void CMobblerAccessPointSettingItem::LoadIapListL()
		{
		// Add all the access point to the list
		CCommsDatabase* commDb = CCommsDatabase::NewL(EDatabaseTypeIAP);
		CleanupStack::PushL(commDb);


		// open IAP table
		CCommsDbTableView* commView = commDb->OpenIAPTableViewMatchingBearerSetLC(ECommDbBearerGPRS | ECommDbBearerWLAN, ECommDbConnectionDirectionOutgoing);
		
		// search all IAPs
		for (TInt error = commView->GotoFirstRecord();
						error == KErrNone;
						error = commView->GotoNextRecord())
				{
				TBuf<KCommsDbSvrMaxColumnNameLength> iapName;
				TUint32 iapId;
				
				commView->ReadTextL(TPtrC(COMMDB_NAME), iapName);
				commView->ReadUintL(TPtrC(COMMDB_ID), iapId);

				HBufC* text = iapName.AllocLC();
				CAknEnumeratedText* enumText = new(ELeave) CAknEnumeratedText(iapId, text);
				CleanupStack::Pop(text);
				CleanupStack::PushL(enumText);
				EnumeratedTextArray()->AppendL(enumText);
				CleanupStack::Pop(enumText);
				}
		
		CleanupStack::PopAndDestroy(commView);
		CleanupStack::PopAndDestroy(commDb);
		}
 


void CMobblerAccessPointSettingItem::CreateAndExecuteSettingPageL()
	{
	CAknSettingPage* dlg = CreateSettingPageL();

	SetSettingPage(dlg);
	SettingPage()->SetSettingPageObserver(this);

	SettingPage()->SetSettingTextL(SettingName());

	SettingPage()->ExecuteLD(CAknSettingPage::EUpdateWhenChanged);
	SetSettingPage(0);
	}


void CMobblerAccessPointSettingItem::CompleteConstructionL()
	{
	CAknEnumeratedTextPopupSettingItem::CompleteConstructionL();
	LoadIapListL();
	}

// End of file
