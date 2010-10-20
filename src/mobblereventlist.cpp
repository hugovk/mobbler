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
#include "mobblerparser.h"
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
	delete iFoursquareHelper;
	}

CMobblerListControl* CMobblerEventList::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
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
    TRACER_AUTO;
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

void CMobblerEventList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		if (aObserver == iAttendanceHelperNo &&
				iType == EMobblerCommandUserEvents)
			{
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
		else if (aObserver == iFoursquareHelper)
			{
			CCoeEnv::Static()->FsSession().MkDirAll(KMapKmlFilename);
			
			_LIT8(KMapKmlStartFormat,		"<kml xmlns=\"http://earth.google.com/kml/2.0\">\r\n");	
			
			_LIT8(KMapKmlPlacemarkFormat,	"\t<Placemark>\r\n"
											"\t\t<name>%S</name>\r\n"
											"\t\t<description>%S said \"%S\"</description>\r\n" 
											"\t\t<Point>\r\n"
											"\t\t\t<coordinates>%S,%S</coordinates>\r\n"
											"\t\t</Point>\r\n"
											"\t</Placemark>\r\n");
			
			_LIT8(KMapKmlEndFormat,			"</kml>\r\n");
			_LIT8(KFirstName, 				"firstname");
			_LIT8(KGeoLat, 					"geolat");
			_LIT8(KGeoLong, 				"geolong");
			_LIT8(KGroup, 					"group");
			_LIT8(KText, 					"text");

			RFileWriteStream file;
			CleanupClosePushL(file);
			file.Replace(CCoeEnv::Static()->FsSession(), KMapKmlFilename, EFileWrite);
			
			file.WriteL(KMapKmlStartFormat);
			
			// Parse the XML
			CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
			CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
			
			RPointerArray<CSenElement>& tips(domFragment->AsElement().Element(KGroup)->ElementsL());
			
			const TInt KTipCount(tips.Count());
			for (TInt i(0); i < KTipCount; ++i)
				{
				TPtrC8 title(tips[i]->Element(KVenue)->Element(KName)->Content());
				TPtrC8 firstname(tips[i]->Element(KUser)->Element(KFirstName)->Content());
				TPtrC8 description(tips[i]->Element(KText)->Content());
				
				TPtrC8 longitude(tips[i]->Element(KVenue)->Element(KGeoLong)->Content());
				TPtrC8 latitude(tips[i]->Element(KVenue)->Element(KGeoLat)->Content());
				
				// Add this tip to the KML file
				HBufC8* placemark(HBufC8::NewLC(KMapKmlPlacemarkFormat().Length() + title.Length() + firstname.Length() + description.Length() + longitude.Length() + latitude.Length()));
				placemark->Des().Format(KMapKmlPlacemarkFormat, &title, &firstname, &description, &longitude, &latitude);
				file.WriteL(*placemark);
				CleanupStack::PopAndDestroy(placemark);
				}
			
			CleanupStack::PopAndDestroy(2);
			
			file.WriteL(KMapKmlEndFormat);
			CleanupStack::PopAndDestroy(&file);
			
			iAppUi.LaunchFileL(KMapKmlFilename);
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
