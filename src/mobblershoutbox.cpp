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
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_user.png");
_LIT8(KEvent, "event");
_LIT8(KGetShouts, "getshouts");

CMobblerShoutbox::CMobblerShoutbox(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerShoutbox::ConstructL()
	{
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
	}

HBufC* CMobblerShoutbox::ShoutAtTextOwnerLC()
	{
	HBufC* shoutAtText(NULL);

	if (iText1->String().Length() == 0)
		{
		CMobblerString* name(CMobblerString::NewL(iAppUi.SettingView().Username()));
		CleanupStack::PushL(name);
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
	return ShoutAtTextLC(iList[iListBox->CurrentItemIndex()]->Title()->String8());
	}

HBufC* CMobblerShoutbox::ShoutAtTextLC(const TDesC8& aName)
	{
	const TDesC& format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHOUT_AT));
	HBufC* text(HBufC::NewLC(format.Length() + aName.Length()));
	
	CMobblerString* name(CMobblerString::NewL(aName));
	CleanupStack::PushL(name);
	text->Des().Format(format, &name->String());
	CleanupStack::PopAndDestroy(name);
	
	return text;
	}

CMobblerListControl* CMobblerShoutbox::HandleListCommandL(TInt aCommand)
	{
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
				CMobblerString* shout(CMobblerString::NewL(shoutMessage));
				CleanupStack::PushL(shout);
				switch (iType)
					{
					case EMobblerCommandUserShoutbox:
						if (aCommand == EMobblerCommandShoutUser)
							{
							iAppUi.LastFmConnection().ShoutL(KUser, iList[iListBox->CurrentItemIndex()]->Title()->String8(), shout->String8());
							}
						else if (iText1->String().Length() == 0)
							{
							CMobblerString* name(CMobblerString::NewL(iAppUi.SettingView().Username()));
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
	aCommands.AppendL(EMobblerCommandOpen);
	aCommands.AppendL(EMobblerCommandShout);
	aCommands.AppendL(EMobblerCommandShoutUser);
	aCommands.AppendL(EMobblerCommandShoutOwner);
	}

void CMobblerShoutbox::ParseL(const TDesC8& aXml)
	{
	CMobblerParser::ParseShoutboxL(aXml, *this, iList);
	}

// End of file
