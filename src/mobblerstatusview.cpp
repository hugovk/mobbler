/*
mobblerstatusview.cpp

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

#include <akncontext.h>
#include <akntitle.h>
#include <audioequalizerutility.h>
#include <barsread.h>
#include <eikmenub.h>
#include <mdaaudiooutputstream.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerradioplayer.h"
#include "mobblerrichtextcontrol.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstatusview.h"

CMobblerStatusView* CMobblerStatusView::NewL()
	{
	CMobblerStatusView* self = new(ELeave) CMobblerStatusView;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerStatusView::CMobblerStatusView()
	{
	}


CMobblerStatusView::~CMobblerStatusView()
	{
	delete iMobblerStatusControl;
	iMobblerStatusControl = NULL;
	}

void CMobblerStatusView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_STATUS_VIEW);
	SetupStatusPaneL();
	}

void CMobblerStatusView::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/) 
	{
	}

void CMobblerStatusView::SetMenuItemTextL(CEikMenuPane* aMenuPane,
										  TInt aResourceId, TInt aCommandId)
	{
	HBufC* menuText = static_cast<CMobblerAppUi*>(AppUi())->AllocReadLC(aResourceId);
	aMenuPane->SetItemTextL(aCommandId, *menuText);
	CleanupStack::PopAndDestroy(menuText);
	}

void CMobblerStatusView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
	{
	// First load the menu text so as not to confuse any dimming logic
	if (aResourceId == R_MOBBLER_STATUS_MENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_START,		EMobblerCommandRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RESUME_RADIO,		EMobblerCommandResumeRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_GO_ONLINE,		EMobblerCommandOnline);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_GO_OFFLINE,		EMobblerCommandOffline);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EQUALIZER,		EMobblerCommandEqualizer);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOOLS_SUBMENU,	EMobblerCommandTools);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SETTINGS,			EMobblerCommandEditSettings);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ABOUT,			EMobblerCommandAbout);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXIT,				EAknSoftkeyExit);
		}
	else if(aResourceId == R_RADIO_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_ARTIST,				EMobblerCommandRadioArtist);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_TAG,				EMobblerCommandRadioTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_USER,				EMobblerCommandRadioUser);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_PERSONAL,			EMobblerCommandRadioPersonal);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_RECOMMENDATIONS,	EMobblerCommandRadioRecommendations);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_LOVED,				EMobblerCommandRadioLoved);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_NEIGHBOURHOOD,		EMobblerCommandRadioNeighbourhood);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_PLAYLIST,			EMobblerCommandRadioMyPlaylist);
		}
	else if(aResourceId == R_MOBBLER_EQUALIZER_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EQUALIZER_PROFILE_DEFAULT, EMobblerCommandEqualizerDefault);
		}
	else if(aResourceId == R_MOBBLER_TOOLS_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_CHECK_FOR_UPDATES,	EMobblerCommandCheckForUpdates);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SLEEP_TIMER,			EMobblerCommandSleepTimer);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXPORT_QUEUE_TO_LOG,	EMobblerCommandExportQueueToLogFile);
		}

	// Now the menu text is set, dimming logic is next
	if (aResourceId == R_MOBBLER_STATUS_MENU_PANE)
		{
		if (static_cast<CMobblerAppUi*>(AppUi())->Mode() == CMobblerLastFMConnection::EOnline ||
				static_cast<CMobblerAppUi*>(AppUi())->State() == CMobblerLastFMConnection::EConnecting ||
				static_cast<CMobblerAppUi*>(AppUi())->State() == CMobblerLastFMConnection::EHandshaking)
			{
			aMenuPane->SetItemDimmed(EMobblerCommandOnline, ETrue);
			}
		else
			{
			aMenuPane->SetItemDimmed(EMobblerCommandOffline, ETrue);
			}
		
		CMdaAudioOutputStream* tempStream = CMdaAudioOutputStream::NewL(*this);
		CAudioEqualizerUtility* tempEqualizer = NULL;
#ifndef __WINS__
		// Emulator seems to crap out even when TRAP_IGNORE is used,
		// it doesn't support the equalizer anyway
		TRAP_IGNORE(tempEqualizer = CAudioEqualizerUtility::NewL(*tempStream));
#endif
		if (!tempEqualizer)
			{
			aMenuPane->SetItemDimmed(EMobblerCommandEqualizer, ETrue);
			}
			
		tempStream->Stop();
		delete tempEqualizer;
		delete tempStream;
		
		aMenuPane->SetItemDimmed(EMobblerCommandResumeRadio, 
					!static_cast<CMobblerAppUi*>(AppUi())->RadioResumable());
		}
	else if (aResourceId == R_MOBBLER_EQUALIZER_SUBMENU_PANE)
		{
		CMdaAudioOutputStream* tempStream = CMdaAudioOutputStream::NewL(*this);
		CleanupStack::PushL(tempStream);
		CAudioEqualizerUtility* tempEqualizer = NULL;
#ifndef __WINS__
		// Emulator seems to crap out even when TRAP_IGNORE is used,
		// it doesn't support the equalizer anyway
		TRAP_IGNORE(tempEqualizer = CAudioEqualizerUtility::NewL(*tempStream));
#endif
		if (tempEqualizer)
			{
			CleanupStack::PushL(tempEqualizer);
			TInt count = tempEqualizer->Presets().Count();
			for (TInt i(0); i < count; ++i)
				{
				CEikMenuPaneItem::SData item;
				item.iCascadeId = 0;
				item.iCommandId = EMobblerCommandEqualizerDefault + i + 1;
				if (i == count - 1)
					{
					item.iFlags = EEikMenuItemRadioEnd;
					}
				else
					{
					item.iFlags = EEikMenuItemRadioMiddle;
					}
				item.iText = tempEqualizer->Presets()[i].iPresetName;
				aMenuPane->AddMenuItemL(item);
				}
			TInt equalizerIndex = static_cast<CMobblerAppUi*>(AppUi())->RadioPlayer()->EqualizerIndex();
			aMenuPane->SetItemButtonState(EMobblerCommandEqualizerDefault + equalizerIndex + 1, EEikMenuItemSymbolOn);
			tempStream->Stop();
			CleanupStack::PopAndDestroy(tempEqualizer);
			}
		CleanupStack::PopAndDestroy(tempStream);
		}
	}

TUid CMobblerStatusView::Id() const
	{
	return TUid::Uid(0xA0007CA8);
	}

void CMobblerStatusView::HandleCommandL(TInt aCommand)
	{
	// let the app ui handle the event
	AppUi()->HandleCommandL(aCommand);
	}

void CMobblerStatusView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	if (!iMobblerStatusControl)
		{
		iMobblerStatusControl = CMobblerStatusControl::NewL(ClientRect(), *static_cast<CMobblerAppUi*>(AppUi()));
		iMobblerStatusControl->SetMopParent(AppUi());
		}
	iMobblerStatusControl->ActivateL();
	AppUi()->AddToStackL(*this, iMobblerStatusControl);

	// Change the Back softkey to Hide
	TInt pos = Cba()->PositionById(EAknSoftkeyBack);
	Cba()->RemoveCommandFromStack(pos, EAknSoftkeyBack);
	HBufC* HideText = static_cast<CMobblerAppUi*>(AppUi())->AllocReadLC(R_MOBBLER_SOFTKEY_HIDE);
	Cba()->SetCommandL(pos, EAknSoftkeyBack, *HideText);
	CleanupStack::PopAndDestroy(HideText);
	}

void CMobblerStatusView::DoDeactivate()
	{
	if (iMobblerStatusControl)
		{
		AppUi()->RemoveFromStack(iMobblerStatusControl);
		//delete iMobblerStatusControl;
		//iMobblerStatusControl = NULL;
		}
	}

void CMobblerStatusView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	TRAP_IGNORE(SetupStatusPaneL()); 
	}

void CMobblerStatusView::SetupStatusPaneL()
	{
	TUid contextPaneUid = TUid::Uid( EEikStatusPaneUidContext );
	CEikStatusPaneBase::TPaneCapabilities subPaneContext = StatusPane()->PaneCapabilities(contextPaneUid);
	
	if ( subPaneContext.IsPresent() && subPaneContext.IsAppOwned() )
		{
		CAknContextPane* context = static_cast<CAknContextPane*>(StatusPane()->ControlL(contextPaneUid ));
		context->SetPictureToDefaultL();
		}
	
	// setup the title pane
	TUid titlePaneUid = TUid::Uid( EEikStatusPaneUidTitle );
	CEikStatusPaneBase::TPaneCapabilities subPaneTitle = StatusPane()->PaneCapabilities(titlePaneUid);
	
	if ( subPaneTitle.IsPresent() && subPaneTitle.IsAppOwned() )
		{
		CAknTitlePane* title = static_cast<CAknTitlePane*>(StatusPane()->ControlL(titlePaneUid));
		TResourceReader reader;
		iEikonEnv->CreateResourceReaderLC(reader, R_MOBBLER_TITLE_RESOURCE);
		title->SetFromResourceL(reader);
		CleanupStack::PopAndDestroy();
		}		
	}

void CMobblerStatusView::DrawDeferred() const
	{
	if (iMobblerStatusControl)
		{
		iMobblerStatusControl->DrawDeferred();
		}
	}

CMobblerRichTextControl& CMobblerStatusView::ArtistInfoControlL()
	{
	if (!iArtistInfoControl)
		{
		iArtistInfoControl = CMobblerRichTextControl::NewL(ClientRect());
		iArtistInfoControl->SetMopParent(AppUi());
		}
	
	iArtistInfoControl->ActivateL();
	AppUi()->AddToStackL(*this, iArtistInfoControl);
	
	return *iArtistInfoControl;
	}

// End of file
