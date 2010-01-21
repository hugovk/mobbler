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

#include "mobbler.hrh"
#include "mobbler_strings.rsg.h"
#include "mobbleralbumlist.h"
#include "mobblerappui.h"
#include "mobblerartistlist.h"
#include "mobblerbitmapcollection.h"
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
_LIT(KPlaylistsFormat, "%S");
_LIT(KOne, "1");

const TTimeIntervalMinutes KMinutesInAnHour(60);
const TTimeIntervalHours KHoursInOneDay(24);

CMobblerListControl* CMobblerListControl::CreateListL(CMobblerAppUi& aAppUi, 
		CMobblerWebServicesControl& aWebServicesControl, 
		TInt aType, 
		const TDesC8& aText1, 
		const TDesC8& aText2)
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
		case EMobblerCommandSearchArtist:
			self = new(ELeave) CMobblerArtistList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandArtistTopAlbums:
		case EMobblerCommandSearchAlbum:
			self = new(ELeave) CMobblerAlbumList(aAppUi, aWebServicesControl);
			break;
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandArtistTopTracks:
		case EMobblerCommandRecentTracks:
		case EMobblerCommandSimilarTracks:
		case EMobblerCommandPlaylistFetchUser:
		case EMobblerCommandPlaylistFetchAlbum:
		case EMobblerCommandSearchTrack:
		case EMobblerCommandViewScrobbleLog:
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
		case EMobblerCommandSearchTag:
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
	
	iListBoxItems = new (ELeave) CDesCArrayFlat(4);
	
	SetRect(iAppUi.ClientRect());
	
	ConstructL();
	}

void CMobblerListControl::MakeListBoxL()
	{
	delete iListBox;
	iListBox = new(ELeave) CAknDoubleLargeStyleListBox();
	
	iListBox->ConstructL(this, EAknListBoxSelectionList | EAknListBoxLoopScrolling );    
	iListBox->SetContainerWindowL(*this);
	
	// Set scrollbars
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);
	
	// Set observers
	iListBox->SetListBoxObserver(this);
	iListBox->ScrollBarFrame()->SetScrollBarFrameObserver(this);    
	
	iListBox->Model()->SetItemTextArray(iListBoxItems);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	
	iListBox->ItemDrawer()->ColumnData()->EnableMarqueeL(ETrue);
	
	// Activate Listbox
	iListBox->SetRect(Rect());
	iListBox->ActivateL();
	}

CMobblerListControl::~CMobblerListControl()
	{
	iAppUi.LastFmConnection().CancelTransaction(this);
	
	const TInt KListCount(iList.Count());
	for (TInt i(0); i < KListCount; ++i)
		{
		iAppUi.LastFmConnection().CancelTransaction(iList[i]);
		}
	
	iList.ResetAndDestroy();
	delete iListBox;
	delete iListBoxItems;
	iAppUi.BitmapCollection().Cancel(iDefaultImage);
	delete iText1;
	delete iText2;
	}

TInt CMobblerListControl::Count() const
	{
	return iList.Count();
	}

TInt CMobblerListControl::Type() const
	{
	return iType;
	}

HBufC* CMobblerListControl::NameL() const
	{
	TPtrC format(KNullDesC);
	TPtrC text(iText1->String());
	
	switch (iType)
		{
		case EMobblerCommandFriends:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_FRIENDS));
			break;
		case EMobblerCommandUserTopArtists:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_ARTISTS));
			break;
		case EMobblerCommandRecommendedArtists:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECOMMENDED_ARTISTS));
			break;
		case EMobblerCommandRecommendedEvents:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECOMMENDED_EVENTS));
			break;
		case EMobblerCommandSimilarArtists:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SIMILAR_ARTISTS));
			break;
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandArtistTopAlbums:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_ALBUMS));
			break;
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandArtistTopTracks:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_TRACKS));
			break;
		case EMobblerCommandRecentTracks:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_RECENT_TRACKS));
			break;
		case EMobblerCommandSimilarTracks:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SIMILAR_TRACKS));
			text.Set(iText2->String());
			break;
		case EMobblerCommandViewScrobbleLog:
			_LIT(KFormat, "%S");
			format.Set(KFormat);
			text.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SCROBBLE_LOG));
			break;
		case EMobblerCommandPlaylistFetchUser:
		case EMobblerCommandPlaylistFetchAlbum:
			format.Set(KPlaylistsFormat);
			break;
		case EMobblerCommandPlaylists:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_PLAYLISTS));
			break;
		case EMobblerCommandUserEvents:
		case EMobblerCommandArtistEvents:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_EVENTS));
			break;
		case EMobblerCommandUserTopTags:
		case EMobblerCommandArtistTopTags:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_TOP_TAGS));
			break;
		case EMobblerCommandUserShoutbox:
		case EMobblerCommandArtistShoutbox:
		case EMobblerCommandEventShoutbox:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SHOUTBOX));
			break;
		case EMobblerCommandSearchTrack:
		case EMobblerCommandSearchAlbum:
		case EMobblerCommandSearchArtist:
		case EMobblerCommandSearchTag:
			format.Set(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FORMAT_SEARCH));
			break;
		default:
			// we should panic if we get here
			break;
		};
	
	HBufC* returnText(HBufC::NewL(format.Length() + text.Length()));
	returnText->Des().Format(format, &text);
	return returnText;
	}

void CMobblerListControl::UpdateIconArrayL()
	{
	if (iDefaultImage && iDefaultImage->Bitmap() && iList.Count() > 0)
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
			CGulIcon* icon(NULL);
			
			if (iList[i]->Image() &&
					iList[i]->Image()->Bitmap() && // the bitmap has loaded
					iList[i]->Image()->ScaleStatus() != CMobblerBitmap::EMobblerScalePending) // Don't display if it is still scaling
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
		
		iListBox->DrawDeferred();
		}
	}

void CMobblerListControl::DataL(const TDesC8& aXml, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		iState = ENormal;
		
		ParseL(aXml);
		
		const TInt KListCount(iList.Count());
		for (TInt i(0); i < KListCount; ++i)
			{
			// add the formatted text to the array
			
			switch (iType)
				{
				case EMobblerCommandUserTopTags:
				case EMobblerCommandArtistTopTags:
				case EMobblerCommandUserTopArtists:
				case EMobblerCommandArtistTopTracks:
					{
					TInt descriptionFormatId(0);

					switch (iType)
						{
						case EMobblerCommandUserTopTags:
						case EMobblerCommandArtistTopTags:
							{
							descriptionFormatId = (iList[i]->Description()->String().Compare(KOne) == 0)?
													R_MOBBLER_FORMAT_TIME_USED:
													R_MOBBLER_FORMAT_TIMES_USED;
							break;
							}
						case EMobblerCommandUserTopArtists:
						case EMobblerCommandArtistTopTracks:
							{
							descriptionFormatId = (iList[i]->Description()->String().Compare(KOne) == 0)?
													R_MOBBLER_FORMAT_PLAY:
													R_MOBBLER_FORMAT_PLAYS;
							break;
							}
						default:
							break;
						}

					const TDesC& descriptionFormat(iAppUi.ResourceReader().ResourceL(descriptionFormatId));

					HBufC* description(HBufC::NewLC(descriptionFormat.Length() + iList[i]->Description()->String().Length()));
					description->Des().Format(descriptionFormat, &iList[i]->Description()->String());
										
					HBufC* format(HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + iList[i]->Title()->String().Length() + description->Length()));
					format->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &iList[i]->Title()->String(), description);
					iListBoxItems->AppendL(*format);
					CleanupStack::PopAndDestroy(format);
					CleanupStack::PopAndDestroy(description);
					}
					break;

				case EMobblerCommandRecentTracks:
					{
					HBufC* title(HBufC::NewLC(KRecentTracksTitleFormat().Length() +
												iList[i]->Title()->String().Length() +
												iList[i]->Description()->String().Length()));
					
					title->Des().Format(KRecentTracksTitleFormat, &iList[i]->Title()->String(), &iList[i]->Description()->String());

					HBufC* description(HBufC::NewLC(50));
					
					TTime itemTime(iList[i]->TimeLocal());
					
					if (itemTime == Time::NullTTime())
						{
						// this means that the track is playling now
						description->Des().Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_NOW_LISTENING));
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
							TPtr yeah(description->Des());
							iList[i]->TimeLocal().FormatL(yeah, KFormatTime);
							}
						}
					
					HBufC* itemText(HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + title->Length() + description->Length()));
					itemText->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, title, description);
					
					iListBoxItems->AppendL(*itemText);
					
					CleanupStack::PopAndDestroy(itemText);
					CleanupStack::PopAndDestroy(description);
					CleanupStack::PopAndDestroy(title);
					}
					break;

				case EMobblerCommandSimilarArtists:
					{
					_LIT(KPercent, "%");
					HBufC* description(HBufC::NewLC(iList[i]->Description()->String().Length() + KPercent().Length()));
					description->Des() = iList[i]->Description()->String();
					description->Des().Append(KPercent());
										
					HBufC* format(HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length() + iList[i]->Title()->String().Length() + description->Length()));
					format->Des().Format(KDoubleLargeStyleListBoxTextFormat, i, &iList[i]->Title()->String(), description);
					iListBoxItems->AppendL(*format);
					CleanupStack::PopAndDestroy(format);
					CleanupStack::PopAndDestroy(description);
					}
					break;

				default:
					{
					HBufC* itemText(HBufC::NewLC(KDoubleLargeStyleListBoxTextFormat().Length()
												+ iList[i]->Title()->String().Length()
												+ iList[i]->Description()->String().Length()));
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

void CMobblerListControl::SizeChanged()
	{
	TRAP_IGNORE(MakeListBoxL());
	TRAP_IGNORE(UpdateIconArrayL());
	}

void CMobblerListControl::HandleResourceChange(TInt aType)
	{
	TRect rect;
	
	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);
		SetRect(rect);
		}
 	
	CCoeControl::HandleResourceChange(aType);
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
		// We have received items for the list
		
		for (TInt i(Max(iListBox->TopItemIndex() - 2, 0)); i <= Min(iListBox->BottomItemIndex() + 2, iList.Count() - 1); ++i)
			{
			if (!iList[i]->ImageRequested())
				{
				// Ihe item has not had an image requested so ask for it now
				RequestImageL(i);
				iList[i]->SetImageRequested(ETrue);
				}
			}
		}
	}

void CMobblerListControl::RequestImageL(TInt aIndex) const
	{
	iAppUi.LastFmConnection().RequestImageL(iList[aIndex], iList[aIndex]->ImageLocation());
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

void CMobblerListControl::HandleListBoxEventL(CEikListBox* /*aListBox*/, TListBoxEvent aEventType)
	{
	CMobblerListControl* list(NULL);
	
	RequestImagesL();
	
	switch (aEventType)
		{
		case EEventEnterKeyPressed:
			list = HandleListCommandL(EMobblerCommandOpen);
			break;
		case EEventItemDoubleClicked:
			list = HandleListCommandL(EMobblerCommandOpen);
			break;
		default:
			break;
		}
	
	if (list)
		{
		CleanupStack::PushL(list);
		iWebServicesControl.ForwardL(list);
		CleanupStack::Pop(list);
		}
	}

TKeyResponse CMobblerListControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	RequestImagesL();
	
	switch(aKeyEvent.iCode)
		{
		case EKeyLeftArrow:
		case EKeyRightArrow:
			{
			// Forward up and down key press events to the list box
			return EKeyWasNotConsumed;
			}
		case EKeyDevice3:
			{
			CMobblerListControl* list(HandleListCommandL(EMobblerCommandOpen));
			
			if (list)
				{
				CleanupStack::PushL(list);
				iWebServicesControl.ForwardL(list);
				CleanupStack::Pop(list);
				}
			
			return EKeyWasConsumed;
			}
		default:
			break;
		}
	
	return iListBox->OfferKeyEventL(aKeyEvent, aEventCode);
	}


void CMobblerListControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc(SystemGc());
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
