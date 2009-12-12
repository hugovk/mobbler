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

#include <apgcli.h>  
#include <documenthandler.h>
#include <s32file.h>
#include <sendomfragment.h>

#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblerwebserviceshelper.h"

#include "mobbler.hrh"

_LIT8(KGetEvents, "getevents");

const TUid KGoogleMapsUid = {0x2000CEA3};

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
	delete iAttendanceHelper;
	delete iAttendanceHelperNo;
	delete iWebServicesHelper;
	delete iFoursquareHelper;
	}

CMobblerListControl* CMobblerEventList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
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
			iAppUi.GoToMapL(iList[iListBox->CurrentItemIndex()]->Title()->String8(),
								iList[iListBox->CurrentItemIndex()]->Latitude(),
								iList[iListBox->CurrentItemIndex()]->Longitude());
			break;
		case EMobblerCommandFoursquare:
			delete iFoursquareHelper;
			iFoursquareHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().FoursquareL(	iList[iListBox->CurrentItemIndex()]->Longitude(),
													iList[iListBox->CurrentItemIndex()]->Latitude(),
													*iFoursquareHelper);
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
	aCommands.AppendL(EMobblerCommandVisitWebPage);
	
	RApaLsSession lsSession;
	CleanupClosePushL(lsSession);
	User::LeaveIfError(lsSession.Connect());
	
	TApaAppInfo appInfo;

	if (lsSession.GetAppInfo(appInfo, KGoogleMapsUid) == KErrNone)
		{
		// Google Maps is installed
		
		aCommands.AppendL(EMobblerCommandMaps);
		aCommands.AppendL(EMobblerCommandVisitMap);
		//aCommands.AppendL(EMobblerCommandFoursquare);
		}
	
	CleanupStack::PopAndDestroy(&lsSession);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandEventShare);
	
	aCommands.AppendL(EMobblerCommandAttendance);
	aCommands.AppendL(EMobblerCommandAttendanceYes);
	aCommands.AppendL(EMobblerCommandAttendanceMaybe);
	aCommands.AppendL(EMobblerCommandAttendanceNo);
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
				
				const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));
				
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
		else if (aObserver == iFoursquareHelper)
			{
			_LIT(KMapKmlFilename, "c:\\mobblermap.kml");
			
			_LIT8(KMapKmlStartFormat,		"<kml xmlns=\"http://earth.google.com/kml/2.0\">\r\n");	
			
			_LIT8(KMapKmlPlacemarkFormat,	"\t<Placemark>\r\n"
											"\t\t<name>%S</name>\r\n"
											"\t\t<description>%S said \"%S\"</description>\r\n" 
											"\t\t<Point>\r\n"
											"\t\t\t<coordinates>%S,%S</coordinates>\r\n"
											"\t\t</Point>\r\n"
											"\t</Placemark>\r\n");
			
			_LIT8(KMapKmlEndFormat,			"</kml>\r\n");
			_LIT8(KElementFirstName, 		"firstname");
			_LIT8(KElementGeoLat, 			"geolat");
			_LIT8(KElementGeoLong, 			"geolong");
			_LIT8(KElementGroup, 			"group");
			_LIT8(KElementText, 			"text");

			RFileWriteStream file;
			CleanupClosePushL(file);
			file.Replace(CCoeEnv::Static()->FsSession(), KMapKmlFilename, EFileWrite);
			
			file.WriteL(KMapKmlStartFormat);

			// Create the XML reader and DOM fragement and associate them with each other
			CSenXmlReader* xmlReader(CSenXmlReader::NewL());
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment(CSenDomFragment::NewL());
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			xmlReader->ParseL(aData);
			
			RPointerArray<CSenElement>& tips(domFragment->AsElement().Element(KElementGroup)->ElementsL());
			
			const TInt KTipCount(tips.Count());
			for (TInt i(0); i < KTipCount; ++i)
				{
				TPtrC8 title(tips[i]->Element(KElementVenue)->Element(KElementName)->Content());
				TPtrC8 firstname(tips[i]->Element(KUser)->Element(KElementFirstName)->Content());
				TPtrC8 description(tips[i]->Element(KElementText)->Content());
				
				TPtrC8 longitude(tips[i]->Element(KElementVenue)->Element(KElementGeoLong)->Content());
				TPtrC8 latitude(tips[i]->Element(KElementVenue)->Element(KElementGeoLat)->Content());
				
				// Add this tip to the KML file
				HBufC8* placemark(HBufC8::NewLC(KMapKmlPlacemarkFormat().Length() + title.Length() + firstname.Length() + description.Length() + longitude.Length() + latitude.Length()));
				placemark->Des().Format(KMapKmlPlacemarkFormat, &title, &firstname, &description, &longitude, &latitude);
				file.WriteL(*placemark);
				CleanupStack::PopAndDestroy(placemark);
				}
			
			CleanupStack::PopAndDestroy(2);
			
			file.WriteL(KMapKmlEndFormat);
			CleanupStack::PopAndDestroy(&file);
			
			CDocumentHandler* docHandler(CDocumentHandler::NewL(CEikonEnv::Static()->Process()));
			CleanupStack::PushL(docHandler);
			TDataType emptyDataType = TDataType();
			docHandler->OpenFileEmbeddedL(KMapKmlFilename, emptyDataType);
			CleanupStack::PopAndDestroy(docHandler);
			}
		}
	}

void CMobblerEventList::ParseL(const TDesC8& aXml)
	{
	CMobblerParser::ParseEventsL(aXml, *this, iList);
	}

// End of file
