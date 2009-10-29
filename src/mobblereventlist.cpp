/*
mobblereventlist.cpp

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

#include <sendomfragment.h>

#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblerwebserviceshelper.h"

#include "mobbler.hrh"

CMobblerEventList::CMobblerEventList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerEventList::ConstructL()
	{
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultEventImage);
	
	switch (iType)
		{
		case EMobblerCommandUserEvents:
			iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("getevents"), iText1->String8(), *this);
			break;
		case EMobblerCommandArtistEvents:
			iAppUi.LastFmConnection().WebServicesCallL(_L8("artist"), _L8("getevents"), iText1->String8(), *this);
			break;
		case EMobblerCommandRecommendedEvents:
			iAppUi.LastFmConnection().RecommendedEventsL(*this);
			break;
		default:
			break;
		}
	}

CMobblerEventList::~CMobblerEventList()
	{
	delete iAttendanceHelper;
	delete iAttendanceHelperNo;
	delete iWebServicesHelper;
	}

CMobblerListControl* CMobblerEventList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandEventShoutbox:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandEventShoutbox, iList[iListBox->CurrentItemIndex()]->Title()->String8(), iList[iListBox->CurrentItemIndex()]->Id());
			break;
		case EMobblerCommandEventShare:
			delete iWebServicesHelper;
			iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
			iWebServicesHelper->EventShareL(iList[iListBox->CurrentItemIndex()]->Id());
			break;
		case EMobblerCommandAttendanceYes:
			delete iAttendanceHelper;
			iAttendanceHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().EventAttendL(iList[iListBox->CurrentItemIndex()]->Id(), CMobblerLastFmConnection::EAttending, *iAttendanceHelper);
			break;
		case EMobblerCommandAttendanceMaybe:
			delete iAttendanceHelper;
			iAttendanceHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().EventAttendL(iList[iListBox->CurrentItemIndex()]->Id(), CMobblerLastFmConnection::EMaybe, *iAttendanceHelper);
			break;
		case EMobblerCommandAttendanceNo:
			delete iAttendanceHelperNo;
			iAttendanceHelperNo = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().EventAttendL(iList[iListBox->CurrentItemIndex()]->Id(), CMobblerLastFmConnection::ENotAttending, *iAttendanceHelperNo);
			break;
		case EMobblerCommandEventWebPage:
			iAppUi.GoToLastFmL(aCommand, iList[iListBox->CurrentItemIndex()]->Id());
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerEventList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandEventShoutbox);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandEventShare);
	
	aCommands.AppendL(EMobblerCommandAttendance);
	aCommands.AppendL(EMobblerCommandAttendanceYes);
	aCommands.AppendL(EMobblerCommandAttendanceMaybe);
	aCommands.AppendL(EMobblerCommandAttendanceNo);
	
	aCommands.AppendL(EMobblerCommandVisitWebPage);
	}

void CMobblerEventList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if (aObserver == iAttendanceHelperNo &&
				iType == EMobblerCommandUserEvents)
			{
			// We have removed an event from the person's Last.fm events. 
			// If the person is the user, remove it from the list.
			// Don't remove it from friends' lists.
			
			if (iText1->String().CompareF(iAppUi.SettingView().Username()) == 0)
				{
				// Create the XML reader and DOM fragement and associate them with each other
				CSenXmlReader* xmlReader(CSenXmlReader::NewL());
				CleanupStack::PushL(xmlReader);
				CSenDomFragment* domFragment(CSenDomFragment::NewL());
				CleanupStack::PushL(domFragment);
				xmlReader->SetContentHandler(*domFragment);
				domFragment->SetReader(*xmlReader);
				
				xmlReader->ParseL(aData);
				
				const TDesC8* statusText(domFragment->AsElement().AttrValue(_L8("status")));
				
				if (iListBox && statusText && (statusText->CompareF(_L8("ok")) == 0))
					{
					// Get the list box items model
					MDesCArray* listArray(iListBox->Model()->ItemTextArray());
					CDesCArray* itemArray(static_cast<CDesCArray*>(listArray));
					
					 // Number of items in the list
					const TInt KCount(itemArray->Count());
					
					// Validate index then delete
					const TInt KIndex(iListBox->CurrentItemIndex());
					if (KIndex >= 0 && KIndex < KCount)
						{
						itemArray->Delete(KIndex, 1);
						AknListBoxUtils::HandleItemRemovalAndPositionHighlightL(
							iListBox, KIndex, ETrue);
						iListBox->DrawNow();
						}
					}
				else
					{
					// There was an error!
					}
				
				CleanupStack::PopAndDestroy(2);
				}
			}
		}
	}

void CMobblerEventList::ParseL(const TDesC8& aXml)
	{
	CMobblerParser::ParseEventsL(aXml, *this, iList);
	}

// End of file
