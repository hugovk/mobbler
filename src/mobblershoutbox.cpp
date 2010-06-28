/*
mobblershoutbox.cpp

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

#include <aknmessagequerydialog.h>
#include <aknquerydialog.h>
#include <sendomfragment.h>
#include <senxmlutils.h> 

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblershoutbox.h"
#include "mobblereventlist.h"
#include "mobblerplaylistlist.h"
#include "mobblerparser.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblerutility.h"

_LIT8(KGetShouts, "getshouts");

CMobblerShoutbox::CMobblerShoutbox(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
    TRACER_AUTO;
	}

void CMobblerShoutbox::ConstructL()
	{
    TRACER_AUTO;
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultUserImage);
	
	switch (iType)
		{
		case EMobblerCommandUserShoutbox:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetShouts, iText1->String8(), *this);
			break;
		case EMobblerCommandEventShoutbox:
			iAppUi.LastFmConnection().WebServicesCallL(KEvent, KGetShouts, iText2->String8(), *this);
			break;
		case EMobblerCommandArtistShoutbox:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetShouts, iText1->String8(), *this);
			break;
		default:
			break;
		}
	}

CMobblerShoutbox::~CMobblerShoutbox()
	{
    TRACER_AUTO;
	iHelpers.ResetAndDestroy();
	}

HBufC* CMobblerShoutbox::ShoutAtTextOwnerLC()
	{
    TRACER_AUTO;
	HBufC* shoutAtText(NULL);

	if (iText1->String().Length() == 0)
		{
		CMobblerString* name(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
		shoutAtText = ShoutAtTextLC(name->String8());
		CleanupStack::Pop(shoutAtText);
		CleanupStack::PopAndDestroy(name);
		CleanupStack::PushL(shoutAtText);
		}
	else
		{
		shoutAtText = ShoutAtTextLC(iText1->String8());
		}
	
	return shoutAtText;
	}

HBufC* CMobblerShoutbox::ShoutAtTextUserLC()
	{
    TRACER_AUTO;
	return ShoutAtTextLC(iList[iListBox->CurrentItemIndex()]->Title()->String8());
	}

HBufC* CMobblerShoutbox::ShoutAtTextLC(const TDesC8& aName)
	{
    TRACER_AUTO;
	const TDesC& format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHOUT_AT));
	HBufC* text(HBufC::NewLC(format.Length() + aName.Length()));
	
	CMobblerString* name(CMobblerString::NewLC(aName));
	text->Des().Format(format, &name->String());
	CleanupStack::PopAndDestroy(name);
	
	return text;
	}

CMobblerListControl* CMobblerShoutbox::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(NULL);
	
	CAknTextQueryDialog* shoutDialog(NULL);
	TBuf<KMobblerMaxQueryDialogLength> shoutMessage;
	
	HBufC* dialogPromptText(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandOpen:
			{
			// Show the shout in a dialog box
			CAknMessageQueryDialog* dlg(new(ELeave) CAknMessageQueryDialog());
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(iList[iListBox->CurrentItemIndex()]->Title()->String());
			dlg->SetMessageTextL(iList[iListBox->CurrentItemIndex()]->Description()->String());
			dlg->RunLD();
			}
			break;
		case EMobblerCommandShoutUser:
		case EMobblerCommandShoutOwner:
			
			if (aCommand == EMobblerCommandShoutUser)
				{
				dialogPromptText = ShoutAtTextUserLC();
				}
			else
				{
				dialogPromptText = ShoutAtTextOwnerLC();
				}
			
			shoutDialog = new(ELeave) CAknTextQueryDialog(shoutMessage);
			shoutDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			shoutDialog->SetPromptL(*dialogPromptText);
			shoutDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (shoutDialog->RunLD())
				{
				CMobblerString* shout(CMobblerString::NewLC(shoutMessage));
				switch (iType)
					{
					case EMobblerCommandUserShoutbox:
						if (aCommand == EMobblerCommandShoutUser)
							{
							iAppUi.LastFmConnection().ShoutL(KUser, iList[iListBox->CurrentItemIndex()]->Title()->String8(), shout->String8());
							}
						else if (iText1->String().Length() == 0)
							{
							CMobblerString* name(CMobblerString::NewL(iAppUi.SettingView().Settings().Username()));
							iAppUi.LastFmConnection().ShoutL(KUser, name->String8(), shout->String8());
							}
						else
							{
							iAppUi.LastFmConnection().ShoutL(KUser, iText1->String8(), shout->String8());
							}
						break;
					case EMobblerCommandEventShoutbox:
						iAppUi.LastFmConnection().ShoutL(KEvent, iText2->String8(), shout->String8());
						break;
					case EMobblerCommandArtistShoutbox:
						iAppUi.LastFmConnection().ShoutL(KArtist, iList[iListBox->CurrentItemIndex()]->Title()->String8(), shout->String8());
						break;
					default:
						break;
					}
				
				CleanupStack::PopAndDestroy(shout);
				}
			
			CleanupStack::PopAndDestroy(dialogPromptText);
			
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerShoutbox::SupportedCommandsL(RArray<TInt>& aCommands)
	{
    TRACER_AUTO;
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandShout);
	aCommands.AppendL(EMobblerCommandShoutUser);
	aCommands.AppendL(EMobblerCommandShoutOwner);
	}

void CMobblerShoutbox::ParseL(const TDesC8& aXml)
	{
    TRACER_AUTO;
	CMobblerParser::ParseShoutboxL(aXml, *this, iList);
	}

void CMobblerShoutbox::RequestImageL(TInt aIndex) const
	{
    TRACER_AUTO;
	// do user.getInfo for this user
	// when we receive that we can fetch the actual image
	CMobblerFlatDataObserverHelper* helper(CMobblerFlatDataObserverHelper::NewL(const_cast<CMobblerShoutbox*>(this)->iAppUi.LastFmConnection(), *const_cast<CMobblerShoutbox*>(this), EFalse));
	CleanupStack::PushL(helper);
	iHelpers.AppendL(helper);
	CleanupStack::Pop(helper);
	
	iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetInfo, iList[aIndex]->Title()->String8(), *helper);
	}

void CMobblerShoutbox::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		DUMPDATA(aData, _L("usergetinfo.xml"));
		
		// Parse the XML
		CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
		CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
		
		TPtrC8 name(domFragment->AsElement().Element(KUser)->Element(KName)->Content());
		TPtrC8 imageLocation(domFragment->AsElement().Element(KUser)->Element(KImage)->Content());
		
		// find the user in the list and add it
		const TInt KCount(iList.Count());
		for (TInt i(0); i < KCount; ++i)
			{
			if (name.CompareF(iList[i]->Title()->String8()) == 0)
				{
				if (iList[i]->ImageLocation().CompareF(KNullDesC8) == 0)
					{
					iList[i]->SetImageLocationL(imageLocation);
					iAppUi.LastFmConnection().RequestImageL(iList[i], imageLocation);
					break;
					}
				}
			}
		
		CleanupStack::PopAndDestroy(2, xmlReader);
		}
	}

// End of file
