/*
mobblerlistcontrol.cpp

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

#include <gulicon.h>
#include <mobbler_strings.rsg>

#include "mobbler.hrh"
#include "mobbleralbumlist.h"
#include "mobblerappui.h"
#include "mobblerartistlist.h"
#include "mobblerbitmap.h"
#include "mobblereventlist.h"
#include "mobblerfriendlist.h"
#include "mobblerlistcontrol.h"
#include "mobblerlistitem.h"
#include "mobblerplaylistlist.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertaglist.h"
#include "mobblertracklist.h"
#include "mobblerwebservicescontrol.h"

_LIT(KDoubleLargeStyleListBoxTextFormat, "%d\t%S\t%S");
_LIT(KRecentTracksTitleFormat, "%S - %S");

const TTimeIntervalMinutes KMinutesInAnHour(60);
const TTimeIntervalHours KHoursInOneDay(24);

CMobblerListControl* CMobblerListControl::CreateListL(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl, TInt aType, const TDesC8& aText1, const TDesC8& aText2)
	{
	CMobblerListControl* self(NULL);
	
	switch (aType)
		{
		case EMobblerCommandFriends:
			self = new(ELeave) CMobblerFriendList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopArtists:
		case EMobblerCommandRecommendedArtists:
		case EMobblerCommandSimilarArtists:
			self = new(ELeave) CMobblerArtistList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandArtistTopAlbums:
			self = new(ELeave) CMobblerAlbumList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandArtistTopTracks:
		case EMobblerCommandRecentTracks:
		case EMobblerCommandSimilarTracks:
			self = new(ELeave) CMobblerTrackList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandPlaylists:
			self = new(ELeave) CMobblerPlaylistList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserEvents:
		case EMobblerCommandArtistEvents:
		case EMobblerCommandRecommendedEvents:
			self = new(ELeave) CMobblerEventList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopTags:
		case EMobblerCommandArtistTopTags:
			self = new(ELeave) CMobblerTagList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserShoutbox:
		case EMobblerCommandArtistShoutbox:
		case EMobblerCommandEventShoutbox:
			self = new(ELeave) CMobblerShoutbox(aAppUi, aWebServicesControl);
			break;
		default:
			// we should panic if we get here
			break;
		};
	
	CleanupStack::PushL(self);
	self->ConstructListL(aType, aText1, aText2);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerListControl::CMobblerListControl(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:iAppUi(aAppUi), iWebServicesControl(aWebServicesControl)
	{
	}

void CMobblerListControl::ConstructListL(TInt aType, const TDesC8& aText1, const TDesC8& aText2)
	{
	iType = aType;
	iText1 = CMobblerString::NewL(aText1);
	iText2 = CMobblerString::NewL(aText2);
	
	CreateWindowL(); // This is a window owning control
	
	InitComponentArrayL();
	
	SetRect(iAppUi.ClientRect());
	
	// Create listbox 
	iListBox = new(ELeave) CAknDoubleLargeStyleListBox();
	
	iListBox->ConstructL(this, EAknListBoxSelectionList | EAknListBoxLoopScrolling );    
	iListBox->SetContainerWindowL(*this);
	
	// Set scrollbars
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);    
	
	// Activate Listbox
	iListBox->SetRect(Rect());
	
	// Intercept scrollbar events so we know when the currently displayed items have changed
	iListBox->ScrollBarFrame()->SetScrollBarFrameObserver(this);
	
	iListBoxItems = new (ELeave) CDesCArrayFlat(4);
	iListBox->Model()->SetItemTextArray(iListBoxItems);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	
	iListBox->ItemDrawer()->ColumnData()->EnableMarqueeL(ETrue);
	
	iListBox->ActivateL();
	
	ConstructL();
	}

CMobblerListControl::~CMobblerListControl()
	{
	iAppUi.LastFMConnection().CancelTransaction(this);
	
	const TInt KListCount(iList.Count());
	for (TInt i(0); i < KListCount ; ++i)
		{
		iAppUi.LastFMConnection().CancelTransaction(iList[i]);
		}
	
	iList.ResetAndDestroy();
	delete iListBox;
	delete iListBoxItems;
	delete iDefaultImage;
	delete iText1;
	delete iText2;
	}

TInt CMobblerListControl::Count() const
	{
	return iList.Count();
	}

HBufC* CMobblerListControl::NameL()
	{
	TBuf<50> name;
	
	switch (iType)
		{
		case EMobblerCommandFriends:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_FRIENDS), &iText1->String());
			break;
		case EMobblerCommandUserTopArtists:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_ARTISTS), &iText1->String());
			break;
		case EMobblerCommandRecommendedArtists:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECOMMENDED_ARTISTS), &iText1->String());
			break;
		case EMobblerCommandRecommendedEvents:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECOMMENDED_EVENTS), &iText1->String());
			break;
		case EMobblerCommandSimilarArtists:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SIMILAR_ARTISTS), &iText1->String());
			break;
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandArtistTopAlbums:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_ALBUMS), &iText1->String());
			break;
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandArtistTopTracks:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_TRACKS), &iText1->String());
			break;
		case EMobblerCommandRecentTracks:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECENT_TRACKS), &iText1->String());
			break;
		case EMobblerCommandSimilarTracks:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SIMILAR_TRACKS), &iText2->String());
			break;
		case EMobblerCommandPlaylists:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_PLAYLISTS), &iText1->String());
			break;
		case EMobblerCommandUserEvents:
		case EMobblerCommandArtistEvents:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_EVENTS), &iText1->String());
			break;
		case EMobblerCommandUserTopTags:
		case EMobblerCommandArtistTopTags:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_TAGS), &iText1->String());
			break;
		case EMobblerCommandUserShoutbox:
		case EMobblerCommandArtistShoutbox:
		case EMobblerCommandEventShoutbox:
			name.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SHOUTBOX), &iText1->String());
			break;
		default:
			// we should panic if we get here
			break;
		};
	
	return name.AllocL();
	}

void CMobblerListControl::UpdateIconArrayL()
	{
	if (iDefaultImage->Bitmap() && iList.Count() > 0)
		{
		// only update the icons if we have loaded the default icon
		
		const TInt KListCount(iList.Count());
		
		if (!iListBox->ItemDrawer()->ColumnData()->IconArray())
			{
			iListBox->ItemDrawer()->ColumnData()->SetIconArray(new(ELeave) CArrayPtrFlat<CGulIcon>(KListCount));
			}
		else
			{
			iListBox->ItemDrawer()->ColumnData()->IconArray()->ResetAndDestroy();
			}
		
		for (TInt i(0); i < KListCount; ++i)
			{
			CGulIcon* icon = NULL;
			
			if (iList[i]->Image() &&
					iList[i]->Image()->Bitmap() && // the bitmap has loaded
					iList[i]->Image()->ScaleSatus() != CMobblerBitmap::EMobblerScalePending) // Don't display if it is still scaling
				{
				icon = CGulIcon::NewL(iList[i]->Image()->Bitmap(), iList[i]->Image()->Mask());
				}
			else
				{
				icon = CGulIcon::NewL(iDefaultImage->Bitmap(), iDefaultImage->Mask());
				}
			
			icon->SetBitmapsOwnedExternally(ETrue);
			iListBox->ItemDrawer()->ColumnData()->IconArray()->AppendL(icon);
			}
		
		iListBox->HandleItemAdditionL();
		}
	}

void CMobblerListControl::DataL(const TDesC8& aXML, TInt aError)
	{
	if (aError == KErrNone)
		{
		iState = ENormal;
		
		ParseL(aXML);
		
		const TInt KListCount(iList.Count());
		for (TInt i(0); i < KListCount; ++i)
			{
			// add the formatted text to the array
			
			switch (iType)
				{
				case EMobblerCommandUserTopTags:
				case EMobblerCommandArtistTopTags:
					{
					TInt descriptionFormatID = (iList[i]->Description()->String().Compare(_L("1")) == 0)?
												R_MOBBLER_FORMAT_TIME_USED:
												R_MOBBLER_FORMAT_TIMES_USED;
					
					const TDesC& descriptionFormat = iAppUi.ResourceReader().ResourceL(descriptionFormatID);

					HBufC* description = HBufC::NewLC(descriptionFormat.Length() + iList[i]->Description()->String().Length());
					description->Des().Format(descriptionFormat, &iList[i]->Description()->String());
					
					HBufC* format = HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + iList[i]->Title()->String().Length() + description->Length());
					format->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &iList[i]->Title()->String(), &description->Des());
					iListBoxItems->AppendL(*format);
					CleanupStack::PopAndDestroy(format);
					CleanupStack::PopAndDestroy(description);
					}
					break;
				case EMobblerCommandUserTopArtists:
				case EMobblerCommandArtistTopTracks:
					{
					TInt descriptionFormatID = (iList[i]->Description()->String().Compare(_L("1")) == 0)?
													R_MOBBLER_FORMAT_PLAY:
													R_MOBBLER_FORMAT_PLAYS;
					
					const TDesC& descriptionFormat = iAppUi.ResourceReader().ResourceL(descriptionFormatID);
										
					HBufC* description = HBufC::NewLC(descriptionFormat.Length() + iList[i]->Description()->String().Length());
					description->Des().Format(descriptionFormat, &iList[i]->Description()->String());
										
					
					HBufC* format = HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + iList[i]->Title()->String().Length() + description->Length());
					format->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &iList[i]->Title()->String(), &description->Des());
					iListBoxItems->AppendL(*format);
					CleanupStack::PopAndDestroy(format);
					CleanupStack::PopAndDestroy(description);
					}
					break;
				case EMobblerCommandRecentTracks:
					{
					HBufC* title = HBufC::NewLC(KRecentTracksTitleFormat().Length() +
													iList[i]->Title()->String().Length() +
													iList[i]->Description()->String().Length());
					
					title->Des().Format(KRecentTracksTitleFormat, &iList[i]->Title()->String(), &iList[i]->Description()->String());

					HBufC* description = HBufC::NewLC(50);
					
					TTime itemTime = iList[i]->TimeLocal();
					
					if (itemTime == Time::NullTTime())
						{
						// this means that the track is playling now
						description->Des().Copy(_L("Now listening"));
						}
					else
						{
						TTime now;
						now.HomeTime();
						
						TTimeIntervalMinutes minutesAgo(0);
						User::LeaveIfError(now.MinutesFrom(itemTime, minutesAgo));
						
						TTimeIntervalHours hoursAgo;
						User::LeaveIfError(now.HoursFrom(itemTime, hoursAgo));
						
						if (minutesAgo < KMinutesInAnHour)
							{
							description->Des().Format( (minutesAgo.Int() == 1)?
															iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TIME_AGO_MINUTE):
															iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TIME_AGO_MINUTES),
															minutesAgo.Int());
							}
						else if (hoursAgo < KHoursInOneDay)
							{
							description->Des().Format( (hoursAgo.Int() == 1)?
															iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TIME_AGO_HOUR):
															iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TIME_AGO_HOURS),
															hoursAgo.Int());				
							}
						else
							{
							TPtr yeah = description->Des();
							iList[i]->TimeLocal().FormatL(yeah, KFormatTime);
							}
						}
					
					HBufC* itemText = HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + title->Length() + description->Length());
					itemText->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &title->Des(), &description->Des());
					
					iListBoxItems->AppendL(*itemText);
					
					CleanupStack::PopAndDestroy(itemText);
					CleanupStack::PopAndDestroy(description);
					CleanupStack::PopAndDestroy(title);
					}
					break;
				default:
					{
					HBufC* itemText = HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + iList[i]->Title()->String().Length() + iList[i]->Description()->String().Length());
					itemText->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &iList[i]->Title()->String(), &iList[i]->Description()->String());
					iListBoxItems->AppendL(*itemText);
					CleanupStack::PopAndDestroy(itemText);
					}
					break;
				}
			}

	    iListBox->HandleItemAdditionL();
	
	    UpdateIconArrayL();
	    
	    RequestImagesL();
		}
	else
		{
		iState = EFailed;
		}
	
	// The state has changed so tell the web
	// services control to update the status pane
	iWebServicesControl.HandleListControlStateChangedL();
	}

void CMobblerListControl::HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType)
	{
	// There has been a scrollbar event so we now
	// may be viewing different list box items
	RequestImagesL();
	
	iListBox->HandleScrollEventL(aScrollBar, aEventType);
	}

void CMobblerListControl::RequestImagesL() const
	{
	// Request images for items that are being displayed plus two each side
	
	if (iList.Count() > 0)
		{
		// We have recieved items for the list
		
		for (TInt i(Max(iListBox->TopItemIndex() - 2, 0)); i <= Min(iListBox->BottomItemIndex() + 2, iList.Count() - 1); ++i)
			{
			if (!iList[i]->ImageRequested())
				{
				// Ihe item has not had an image requested so ask for it now
				iAppUi.LastFMConnection().RequestImageL(iList[i], iList[i]->ImageLocation());
				iList[i]->SetImageRequested(ETrue);
				}
			}
		}
	}

CMobblerListControl::TState CMobblerListControl::State() const
	{
	return iState;
	}

void CMobblerListControl::HandleLoadedL()
	{
	UpdateIconArrayL();
	}

void CMobblerListControl::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	UpdateIconArrayL();
	}

void CMobblerListControl::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	UpdateIconArrayL();
	}

TKeyResponse CMobblerListControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	RequestImagesL();
	
	if ( aKeyEvent.iCode == EKeyLeftArrow 
		|| aKeyEvent.iCode == EKeyRightArrow )
		{
		// allow the web services control to get the arrow keys
		return EKeyWasNotConsumed;
		}
	
	return iListBox->OfferKeyEventL(aKeyEvent, aEventCode);
	}


void CMobblerListControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
   	gc.Clear(Rect());
	}

CCoeControl* CMobblerListControl::ComponentControl(TInt /*aIndex*/) const
	{
	return iListBox;
	}
 
TInt CMobblerListControl::CountComponentControls() const
	{
	return 1;
	}

// End of file