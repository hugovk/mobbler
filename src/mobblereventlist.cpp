/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade

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

#include <apgcli.h>
#include <aknmessagequerydialog.h>
#include <documenthandler.h>
#include <s32file.h>
#include <sendomfragment.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"

#include "mobbler.hrh"

_LIT8(KGetEvents, "getevents");

const TUid KGoogleMapsUid = {0x2000CEA3};

CMobblerEventList::CMobblerEventList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerEventList::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultEventImage);
	
	switch (iType)
		{
		case EMobblerCommandUserEvents:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetEvents, iText1->String8(), *this);
			break;
		case EMobblerCommandArtistEvents:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetEvents, iText1->String8(), *this);
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
    TRACER_AUTO;
	delete iAttendanceHelper;
	delete iAttendanceHelperNo;
	delete iWebServicesHelper;
	}

CMobblerListControl* CMobblerEventList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandOpen:
			{
			// Show the event details in a dialog box
			_LIT(KNewLine, "\n");
			HBufC* message(HBufC::NewLC(
				iList[iListBox->CurrentItemIndex()]->Title()->String().Length() +
				KNewLine().Length() +
				iList[iListBox->CurrentItemIndex()]->Description()->String().Length()));

			message->Des().Copy(iList[iListBox->CurrentItemIndex()]->Title()->String());
			message->Des().Append(KNewLine);
			message->Des().Append(iList[iListBox->CurrentItemIndex()]->Description()->String());

			CAknMessageQueryDialog* dlg(new(ELeave) CAknMessageQueryDialog());
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(
				iAppUi.ResourceReader().ResourceL(R_MOBBLER_EVENT));
			dlg->SetMessageTextL(*message);
			dlg->RunLD();
			CleanupStack::PopAndDestroy(message);
			}
			break;
		case EMobblerCommandEventShoutbox:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, 
					EMobblerCommandEventShoutbox, 
					iList[iListBox->CurrentItemIndex()]->Title()->String8(), 
					iList[iListBox->CurrentItemIndex()]->Id());
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
		case EMobblerCommandVisitMap:
			iAppUi.GoToMapL(	iList[iListBox->CurrentItemIndex()]->Title()->String8(),
								iList[iListBox->CurrentItemIndex()]->Description()->String8(),
								iList[iListBox->CurrentItemIndex()]->Latitude(),
								iList[iListBox->CurrentItemIndex()]->Longitude());
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerEventList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandEventShoutbox);
	aCommands.AppendL(EMobblerCommandVisitWebPage);
	
	RApaLsSession lsSession;
	CleanupClosePushL(lsSession);
	User::LeaveIfError(lsSession.Connect());
	
	TApaAppInfo appInfo;

	if (lsSession.GetAppInfo(appInfo, KGoogleMapsUid) == KErrNone)
		{
		// Google Maps is installed
		
		aCommands.AppendL(EMobblerCommandVisitMap);
		}
	
	CleanupStack::PopAndDestroy(&lsSession);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandEventShare);
	
	aCommands.AppendL(EMobblerCommandAttendance);
	aCommands.AppendL(EMobblerCommandAttendanceYes);
	aCommands.AppendL(EMobblerCommandAttendanceMaybe);
	aCommands.AppendL(EMobblerCommandAttendanceNo);
	}

void CMobblerEventList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if (aObserver == iAttendanceHelperNo &&
				iType == EMobblerCommandUserEvents)
			{
			DUMPDATA(aData, _L("attendance.xml"));
			// We have removed an event from the person's Last.fm events. 
			// If the person is the user, remove it from the list.
			// Don't remove it from friends' lists.
			
			if (iText1->String().CompareF(iAppUi.SettingView().Settings().Username()) == 0)
				{
				// Parse the XML
				CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
				CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
				
				const TDesC8* statusText(domFragment->AsElement().AttrValue(KStatus));
				
				if (iListBox && statusText && (statusText->CompareF(KOk) == 0))
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

TBool CMobblerEventList::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	CMobblerParser::ParseEventsL(aXml, *this, iList);
	return ETrue;
	}

// End of file
