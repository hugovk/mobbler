/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010, 2012  Hugo van Kemenade

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

#include <akninfopopupnotecontroller.h>
#include <AknLists.h>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <aknsutils.h>
#include <bautils.h>
#include <sendomfragment.h>
#include <senxmlutils.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblercontacts.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerwebserviceshelper.h"

CMobblerWebServicesHelper* CMobblerWebServicesHelper::NewL(CMobblerAppUi& aAppUi)
	{
    TRACER_AUTO;
	CMobblerWebServicesHelper* self(new(ELeave) CMobblerWebServicesHelper(aAppUi));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesHelper::CMobblerWebServicesHelper(CMobblerAppUi& aAppUi)
	:iAppUi(aAppUi)
	{
    TRACER_AUTO;
	}

void CMobblerWebServicesHelper::ConstructL()
	{
    TRACER_AUTO;
	}

CMobblerWebServicesHelper::~CMobblerWebServicesHelper()
	{
    TRACER_AUTO;
	delete iTagAddHelper;
	delete iTagRemoveHelper;
	
	delete iTrackTagRemoveTagsHelper;
	delete iAlbumTagRemoveTagsHelper;
	delete iArtistTagRemoveTagsHelper;
	
	delete iEventId;
	delete iFriendFetchObserverHelperTrackShare;
	delete iFriendFetchObserverHelperArtistShare;
	delete iFriendFetchObserverHelperEventShare;
	delete iShareObserverHelper;
	delete iPlaylistAddObserverHelper;
	delete iPlaylistFetchObserverHelper;
	
	if (iTrack)
		{
		iTrack->Release();
		}
	}

CMobblerWebServicesHelper::TShareWith CMobblerWebServicesHelper::ShareSourceL()
	{
    TRACER_AUTO;
	TShareWith shareSource(EShareCancelled);
	
	// parse and bring up an add to playlist popup menu
	CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
	CleanupStack::PushL(list);
	
	CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
	CleanupStack::PushL(popup);
	
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	
	popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHARE));
	
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
	CleanupStack::PushL(items);
	
	items->AppendL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHARE_LASTFM));
	//items->AppendL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHARE_CONTACTS));
	
	CleanupStack::Pop(items);
	
	list->Model()->SetItemTextArray(items);
	list->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	CleanupStack::Pop(popup);
	
	if (popup->ExecuteLD())
		{
		switch (list->CurrentItemIndex())
			{
			case 0:
				shareSource = EShareWithFriends;
				break;
			case 2:
				shareSource = EShareWithContacts;
				break;
			}
		}
	
	CleanupStack::PopAndDestroy(list); //list
	
	return shareSource;
	}

void CMobblerWebServicesHelper::TrackShareL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	TShareWith shareSource(ShareSourceL());
	
	if (shareSource == EShareWithFriends)
		{
		CMobblerString* username(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
		
		delete iFriendFetchObserverHelperTrackShare;
		iFriendFetchObserverHelperTrackShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
		iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, username->String8(), *iFriendFetchObserverHelperTrackShare);
		
		CleanupStack::PopAndDestroy(username);
		}
	else if (shareSource == EShareWithContacts)
		{
		HBufC* contact(DisplayContactListL());
		
		if (contact)
			{
			CleanupStack::PushL(contact);
			CMobblerString* contactString(CMobblerString::NewLC(*contact));
			DoShareL(EMobblerCommandTrackShare, contactString->String8());
			CleanupStack::PopAndDestroy(2, contact);
			}
		}
	}

void CMobblerWebServicesHelper::AlbumShareL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	TShareWith shareSource(ShareSourceL());
	
	if (shareSource == EShareWithFriends)
		{
		CMobblerString* username(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
		
		delete iFriendFetchObserverHelperAlbumShare;
		iFriendFetchObserverHelperAlbumShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
		iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, username->String8(), *iFriendFetchObserverHelperAlbumShare);
		
		CleanupStack::PopAndDestroy(username);
		}
	else if (shareSource == EShareWithContacts)
		{
		HBufC* contact(DisplayContactListL());
		
		if (contact)
			{
			CleanupStack::PushL(contact);
			CMobblerString* contactString(CMobblerString::NewLC(*contact));
			DoShareL(EMobblerCommandAlbumShare, contactString->String8());
			CleanupStack::PopAndDestroy(2, contact);
			}
		}
	}

void CMobblerWebServicesHelper::ArtistShareL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	TShareWith shareSource(ShareSourceL());
	
	if (shareSource == EShareWithFriends)
		{
		CMobblerString* username(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
		
		delete iFriendFetchObserverHelperArtistShare;
		iFriendFetchObserverHelperArtistShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
		iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, username->String8(), *iFriendFetchObserverHelperArtistShare);
		
		CleanupStack::PopAndDestroy(username);
		}
	else if (shareSource == EShareWithContacts)
		{
		HBufC* contact(DisplayContactListL());
		
		if (contact)
			{
			CleanupStack::PushL(contact);
			CMobblerString* contactString(CMobblerString::NewLC(*contact));
			DoShareL(EMobblerCommandArtistShare, contactString->String8());
			CleanupStack::PopAndDestroy(2, contact);
			}
		}
	}

void CMobblerWebServicesHelper::PlaylistAddL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	CMobblerString* username(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
	
	delete iPlaylistFetchObserverHelper;
	iPlaylistFetchObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetPlaylists, username->String8(), *iPlaylistFetchObserverHelper);
	
	CleanupStack::PopAndDestroy(username);
	}

void CMobblerWebServicesHelper::EventShareL(const TDesC8& aEventId)
	{
    TRACER_AUTO;
	iEventId = aEventId.AllocL();
	
	CMobblerString* username(CMobblerString::NewLC(iAppUi.SettingView().Settings().Username()));
	
	delete iFriendFetchObserverHelperEventShare;
	iFriendFetchObserverHelperEventShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetFriends, username->String8(), *iFriendFetchObserverHelperEventShare);
	
	CleanupStack::PopAndDestroy(username);
	}

void CMobblerWebServicesHelper::AddTagL(CMobblerTrack& aTrack, TInt aCommand)
	{
    TRACER_AUTO;
	TBuf<KMobblerMaxQueryDialogLength> tag;
	
	TInt resourceId(R_MOBBLER_TRACK_ADD_TAG_PROMPT);
	switch (aCommand)
		{
//		case EMobblerCommandTrackAddTag:
//			resourceId = R_MOBBLER_TRACK_ADD_TAG_PROMPT;
//			break;
		case EMobblerCommandAlbumAddTag:
			resourceId = R_MOBBLER_ALBUM_ADD_TAG_PROMPT;
			break;
		case EMobblerCommandArtistAddTag:
			resourceId = R_MOBBLER_ARTIST_ADD_TAG_PROMPT;
			break;
		default:
			break;
		}

	CAknTextQueryDialog* tagDialog(new(ELeave) CAknTextQueryDialog(tag));
	tagDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
	tagDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(resourceId));
	tagDialog->SetPredictiveTextInputPermitted(ETrue);
	
	if (tagDialog->RunLD())
		{
		CMobblerString* tagString(CMobblerString::NewLC(tag));
		
		delete iTagAddHelper;
		iTagAddHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
		switch (aCommand)
			{
			case EMobblerCommandTrackAddTag:
				iAppUi.LastFmConnection().QueryLastFmL(aCommand, aTrack.Artist().String8(), KNullDesC8, aTrack.Title().String8(),  tagString->String8(), *iTagAddHelper);
				break;
			case EMobblerCommandAlbumAddTag:
				iAppUi.LastFmConnection().QueryLastFmL(aCommand, aTrack.Artist().String8(), aTrack.Album().String8(), KNullDesC8, tagString->String8(), *iTagAddHelper);
				break;
			case EMobblerCommandArtistAddTag:
				iAppUi.LastFmConnection().QueryLastFmL(aCommand, aTrack.Artist().String8(), KNullDesC8, KNullDesC8, tagString->String8(), *iTagAddHelper);
				break;
			default:
				break;
			}
		
		CleanupStack::PopAndDestroy(tagString);
		}
	}

void CMobblerWebServicesHelper::TrackRemoveTagL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	// fetch the user's tags for this track
	delete iTrackTagRemoveTagsHelper;
	iTrackTagRemoveTagsHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandTrackGetTags, aTrack.Artist().String8(), KNullDesC8, aTrack.Title().String8(), KNullDesC8, *iTrackTagRemoveTagsHelper);
	}

void CMobblerWebServicesHelper::AlbumRemoveTagL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	// fetch the user's tags for this track
	delete iAlbumTagRemoveTagsHelper;
	iAlbumTagRemoveTagsHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandAlbumGetTags, aTrack.Artist().String8(), aTrack.Album().String8(), KNullDesC8, KNullDesC8, *iAlbumTagRemoveTagsHelper);
	}

void CMobblerWebServicesHelper::ArtistRemoveTagL(CMobblerTrack& aTrack)
	{
    TRACER_AUTO;
	iTrack = &aTrack;
	iTrack->Open();
	
	// fetch the user's tags for this track
	delete iArtistTagRemoveTagsHelper;
	iArtistTagRemoveTagsHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandArtistGetTags, aTrack.Artist().String8(), KNullDesC8, KNullDesC8, KNullDesC8, *iArtistTagRemoveTagsHelper);
	}

HBufC* CMobblerWebServicesHelper::DisplayEmailListL(const CDesCArray& aEmails)
	{
    TRACER_AUTO;
	CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
	CleanupStack::PushL(list);
	
	CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
	CleanupStack::PushL(popup);
	
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	
	popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
	
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
	CleanupStack::PushL(items);
	
	const TInt KEmailCount(aEmails.Count());
	for (TInt i(0); i < KEmailCount; ++i)
		{
		items->AppendL(aEmails[i]);
		}
	
	CleanupStack::Pop(items);
	
	list->Model()->SetItemTextArray(items);
	list->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	CleanupStack::Pop(popup);
	
	HBufC* email(NULL);
	
	if (popup->ExecuteLD())
		{
		email = aEmails[list->CurrentItemIndex()].AllocL();
		}
	
	CleanupStack::PopAndDestroy(list);
	
	return email;
	}

void CMobblerWebServicesHelper::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	CActiveScheduler::Stop();
	}

void CMobblerWebServicesHelper::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	
	}

HBufC* CMobblerWebServicesHelper::DisplayContactListL()
	{
    TRACER_AUTO;
	CMobblerContacts* contacts(CMobblerContacts::NewLC());
	
	CAknDoubleLargeGraphicPopupMenuStyleListBox* list(new(ELeave) CAknDoubleLargeGraphicPopupMenuStyleListBox);
	CleanupStack::PushL(list);
	
	CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
	CleanupStack::PushL(popup);
	
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	
	popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
	
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	const TInt KContactCount(contacts->Count());
	
	CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(KContactCount));
	CleanupStack::PushL(items);
	
	list->ItemDrawer()->ColumnData()->SetIconArray(new(ELeave) CArrayPtrFlat<CGulIcon>(KContactCount));
	
	CMobblerBitmap* defaultUser(iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultUserImage));
	CActiveScheduler::Start();
	CGulIcon* defaultUserIcon = CGulIcon::NewL(defaultUser->Bitmap(), defaultUser->Mask());
	defaultUserIcon->SetBitmapsOwnedExternally(ETrue);
	list->ItemDrawer()->ColumnData()->IconArray()->AppendL(defaultUserIcon);
	
	TInt photoNumber(1);
	
	_LIT(KEllipsisText, "...");
	_LIT(KFormat, "%d\t%S\t%S");
	for (TInt i(0); i < KContactCount; ++i)
		{
		HBufC8* photo(contacts->GetPhotoAtL(i));
		TPtrC name(contacts->GetNameAt(i));
		CDesCArray* emails(contacts->GetEmailsAtLC(list->CurrentItemIndex()));
		TPtrC firstEmail(KNullDesC);
		
		if (emails->Count() == 1)
			{
			firstEmail.Set((*emails)[0]);
			}
		else
			{
			firstEmail.Set(KEllipsisText);
			}
		
		if (photo)
			{
			CleanupStack::PushL(photo);
			CMobblerBitmap* bitmap(CMobblerBitmap::NewL(*this, *photo));
			CActiveScheduler::Start();
			CGulIcon* icon(CGulIcon::NewL(bitmap->Bitmap(), bitmap->Mask()));
			icon->SetBitmapsOwnedExternally(ETrue);
			list->ItemDrawer()->ColumnData()->IconArray()->AppendL(icon);
			//bitmap->Close();
			CleanupStack::PopAndDestroy(photo);
			
			TBuf<1024> formatted;
			formatted.Format(KFormat, photoNumber++, &name, &firstEmail);
			items->AppendL(formatted);
			}
		else
			{
			TBuf<1024> formatted;
			formatted.Format(KFormat, 0, &name, &firstEmail);
			items->AppendL(formatted);
			}
		
		CleanupStack::PopAndDestroy(emails);
		}
	
	CleanupStack::Pop(items);
	
	list->Model()->SetItemTextArray(items);
	list->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	CleanupStack::Pop(popup);
	
	HBufC* email(NULL);
	
	if (popup->ExecuteLD())
		{
		CDesCArray* emails(contacts->GetEmailsAtLC(list->CurrentItemIndex()));
		
		if (emails->Count() == 1)
			{
			email = (*emails)[0].AllocL();
			}
		else
			{
			// they have multiple emails so ask which one
			email = DisplayEmailListL(*emails);
			}
		
		CleanupStack::PopAndDestroy(emails);
		}
	
	CleanupStack::PopAndDestroy(list);
	CleanupStack::PopAndDestroy(contacts);
	
	return email;
	}

void CMobblerWebServicesHelper::DoShareL(TInt aCommand, const TDesC8& aRecipient)
	{
    TRACER_AUTO;
	TBuf<KMobblerMaxQueryDialogLength> message;
	
	CAknTextQueryDialog* shoutDialog(new(ELeave) CAknTextQueryDialog(message));
	shoutDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
	shoutDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_MESSAGE_PROMPT));
	shoutDialog->SetPredictiveTextInputPermitted(ETrue);
	
	if (shoutDialog->RunLD())
		{
		CMobblerString* messageString(CMobblerString::NewLC(message));
		
		if (aCommand == EMobblerCommandTrackShare)
			{
			delete iShareObserverHelper;
			iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().ShareL(EMobblerCommandTrackShare, aRecipient, iTrack->Artist().String8(), iTrack->Album().String8(), iTrack->Title().String8(), KNullDesC8, messageString->String8(), *iShareObserverHelper);
			}
		else if (aCommand == EMobblerCommandAlbumShare)
			{
			delete iShareObserverHelper;
			iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().ShareL(EMobblerCommandAlbumShare, aRecipient, iTrack->Artist().String8(), iTrack->Album().String8(), KNullDesC8, KNullDesC8, messageString->String8(), *iShareObserverHelper);
			}
		else if (aCommand == EMobblerCommandArtistShare)
			{
			delete iShareObserverHelper;
			iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().ShareL(EMobblerCommandArtistShare, aRecipient, iTrack->Artist().String8(), iTrack->Album().String8(), KNullDesC8, KNullDesC8, messageString->String8(), *iShareObserverHelper);
			}
		else
			{
			// This must be sharing an event
			delete iShareObserverHelper;
			iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().ShareL(EMobblerCommandEventShare, aRecipient, KNullDesC8, KNullDesC8, KNullDesC8, *iEventId, messageString->String8(), *iShareObserverHelper);
			}
		
		CleanupStack::PopAndDestroy(messageString);
		}
	}

void CMobblerWebServicesHelper::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
    TBool releaseTrack(ETrue);
    
    if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
        {
        // Parse the XML
        CSenXmlReader* xmlReader(NULL);
        CSenDomFragment* domFragment(NULL);
        
        // Don't parse the response for the following calls as we don't need to
        xmlReader = CSenXmlReader::NewLC();
        domFragment = MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData);
        
        if (aObserver == iShareObserverHelper ||
                aObserver == iPlaylistAddObserverHelper)
            {
            if (domFragment->AsElement().AttrValue(KStatus)->Compare(KOk) == 0)
                {
                // Everything worked
                CAknConfirmationNote* note(new (ELeave) CAknConfirmationNote(EFalse));
                note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_DONE));
                }
            else
                {
                // There was an error so display it
                CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
                CMobblerString* string(CMobblerString::NewLC(domFragment->AsElement().Element(KError)->Content()));
                note->ExecuteLD(string->String());
                CleanupStack::Pop(string);
                }
            }
        else if (aObserver == iFriendFetchObserverHelperTrackShare ||
                    aObserver == iFriendFetchObserverHelperAlbumShare ||
                    aObserver == iFriendFetchObserverHelperArtistShare ||
                    aObserver == iFriendFetchObserverHelperEventShare)
            {
            // Parse and bring up a share with friends popup menu
            
            CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
            CleanupStack::PushL(list);
            
            CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
            CleanupStack::PushL(popup);
            
            list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
            
            popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
            
            list->CreateScrollBarFrameL(ETrue);
            list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
            
            CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
            CleanupStack::PushL(items);
            
            RPointerArray<CSenElement>& users(domFragment->AsElement().Element(KFriends)->ElementsL());
            
            const TInt KUserCount(users.Count());
            for (TInt i(0); i < KUserCount; ++i)
                {
                CMobblerString* user(CMobblerString::NewLC(users[i]->Element(KName)->Content()));
                items->AppendL(user->String());
                CleanupStack::PopAndDestroy(user);
                }
            
            CleanupStack::Pop(items);
            
            list->Model()->SetItemTextArray(items);
            list->Model()->SetOwnershipType(ELbmOwnsItemArray);
            
            CleanupStack::Pop(popup);
            
            if (popup->ExecuteLD())
                {
                CMobblerString* recipient(CMobblerString::NewLC((*items)[list->CurrentItemIndex()]));
                
                if (aObserver == iFriendFetchObserverHelperTrackShare)
                    {
                    DoShareL(EMobblerCommandTrackShare, recipient->String8());
                    }
                else if (aObserver == iFriendFetchObserverHelperAlbumShare)
                    {
                    DoShareL(EMobblerCommandAlbumShare, recipient->String8());
                    }
                else if (aObserver == iFriendFetchObserverHelperArtistShare)
                    {
                    DoShareL(EMobblerCommandArtistShare, recipient->String8());
                    }
                else
                    {
                    // This must be sharing an event
                    DoShareL(EMobblerCommandEventShare, recipient->String8());
                    }
                
                CleanupStack::PopAndDestroy(recipient);
                }
            
            CleanupStack::PopAndDestroy(list); //list
            }
        else if (aObserver == iPlaylistFetchObserverHelper)
            {
            // parse and bring up an add to playlist popup menu
            CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
            CleanupStack::PushL(list);
            
            CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
            CleanupStack::PushL(popup);
            
            list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
            
            popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_ADD_TRACK));
            
            list->CreateScrollBarFrameL(ETrue);
            list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
            
            CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
            CleanupStack::PushL(items);
            
            RPointerArray<CSenElement>& playlists(domFragment->AsElement().Element(KPlaylists)->ElementsL());
            
            const TInt KPlaylistCount(playlists.Count());
            for (TInt i(0); i < KPlaylistCount; ++i)
                {
                CMobblerString* playlist(CMobblerString::NewLC(playlists[i]->Element(KTitle)->Content()));
                items->AppendL(playlist->String());
                CleanupStack::PopAndDestroy(playlist);
                }
            
            CleanupStack::Pop(items);
            
            list->Model()->SetItemTextArray(items);
            list->Model()->SetOwnershipType(ELbmOwnsItemArray);
            
            CleanupStack::Pop(popup);
            
            if (popup->ExecuteLD())
                {
                if (list->CurrentItemIndex() >= 0)
                    {
                    delete iPlaylistAddObserverHelper;
                    iPlaylistAddObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
                    iAppUi.LastFmConnection().PlaylistAddTrackL(playlists[list->CurrentItemIndex()]->Element(KId)->Content(), iTrack->Artist().String8(), iTrack->Title().String8(), *iPlaylistAddObserverHelper);
                    }
                }
            
            CleanupStack::PopAndDestroy(list); //list
            }
        else if (aObserver == iTrackTagRemoveTagsHelper
                || aObserver == iAlbumTagRemoveTagsHelper
                || aObserver == iArtistTagRemoveTagsHelper)
            {
            RPointerArray<CSenElement>& tags(domFragment->AsElement().Element(KTags)->ElementsL());
            
            const TInt KTagCount(tags.Count());
            
            if (KTagCount > 0)
                {
                CDesCArray* textArray(new(ELeave) CDesCArrayFlat(KTagCount));
                CleanupStack::PushL(textArray);
                
                for (TInt i(0) ; i < KTagCount ; ++i)
                    {
                    CMobblerString* tagName(CMobblerString::NewLC(tags[i]->Element(KName)->Content()));
                    textArray->AppendL(tagName->String());
                    CleanupStack::PopAndDestroy(tagName);
                    }
                
                TInt index;
                CAknListQueryDialog* tagRemoveDialog = new(ELeave) CAknListQueryDialog(&index);
                tagRemoveDialog->PrepareLC(R_MOBBLER_TAG_REMOVE_QUERY);
                tagRemoveDialog->SetItemTextArray(textArray); 
                tagRemoveDialog->SetOwnershipType(ELbmDoesNotOwnItemArray); 
             
                if (tagRemoveDialog->RunLD())
                    {
                    // remove the tag!
                    CMobblerString* tagName(CMobblerString::NewLC((*textArray)[index]));
                    
                    delete iTagRemoveHelper;
                    iTagRemoveHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
                    
                    if (aObserver == iTrackTagRemoveTagsHelper)
                        {
                        iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandTrackRemoveTag, iTrack->Artist().String8(), KNullDesC8, iTrack->Title().String8(), tagName->String8(), *iTagRemoveHelper);
                        }
                    else if (aObserver == iAlbumTagRemoveTagsHelper)
                        {
                        iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandAlbumRemoveTag, iTrack->Artist().String8(), iTrack->Album().String8(), KNullDesC8, tagName->String8(), *iTagRemoveHelper);
                        }
                    else if (aObserver == iArtistTagRemoveTagsHelper)
                        {
                        iAppUi.LastFmConnection().QueryLastFmL(EMobblerCommandArtistRemoveTag, iTrack->Artist().String8(), KNullDesC8, KNullDesC8, tagName->String8(), *iTagRemoveHelper);
                        }
                    
                    CleanupStack::PopAndDestroy(tagName);
                    }
                
                CleanupStack::Pop(textArray);
                }
            else
                {
                // TODO: display an error!
                }
            }
        }
    else
        {
        CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
        note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ERROR));
        }
	
	if (releaseTrack && iTrack)
		{
		iTrack->Release();
		iTrack = NULL;
		}
	
	delete iEventId;
	iEventId = NULL;
	}

// End of file
