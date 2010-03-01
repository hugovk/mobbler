/*
mobblerbrowserview.cpp

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

#include "mobbler.rsg.h"
#include "mobblerappui.h"
#include "mobblerbrowsercontrol.h"
#include "mobblerbrowserview.h"
#include "mobblerresourcereader.h"
#include "mobblertracer.h"

CMobblerBrowserView* CMobblerBrowserView::NewL()
	{
    TRACER_AUTO;
	CMobblerBrowserView* self(new (ELeave) CMobblerBrowserView);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBrowserView::CMobblerBrowserView()
	{
    TRACER_AUTO;
	}

CMobblerBrowserView::~CMobblerBrowserView()
	{
    TRACER_AUTO;
	delete iBrowserControl;
	}

void CMobblerBrowserView::ConstructL()
	{
    TRACER_AUTO;
	BaseConstructL(R_MOBBLER_BROWSER_VIEW);
	}

void CMobblerBrowserView::SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, TInt aCommandId)
	{
    TRACER_AUTO;
	HBufC* menuText(static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(aResourceId).AllocLC());

	const TInt KTextLimit(CEikMenuPaneItem::SData::ENominalTextLength);
	if (menuText->Length() > KTextLimit)
		{
		TBuf<KTextLimit> newText(menuText->Left(KTextLimit));
		CleanupStack::PopAndDestroy(menuText);
		menuText = newText.AllocLC();
		}

	aMenuPane->SetItemTextL(aCommandId, *menuText);
	CleanupStack::PopAndDestroy(menuText);
	}

void CMobblerBrowserView::DynInitMenuPaneL(TInt /*aResourceId*/, CEikMenuPane* /*aMenuPane*/)
	{
    TRACER_AUTO;
	}

TUid CMobblerBrowserView::Id() const
	{
    TRACER_AUTO;
	return TUid::Uid(KMobblerBrowserViewUid);
	}

void CMobblerBrowserView::HandleCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	switch (aCommand)
		{
		case EAknSoftkeyBack:
			// Change view layout back to normal (with title pane etc.)
			if(StatusPane()->CurrentLayoutResId() != R_AVKON_STATUS_PANE_LAYOUT_USUAL)
				{
				StatusPane()->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_USUAL);
				}
			AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid)); // switch back to the status view
			break;
		case EAknSoftkeyExit:
			AppUi()->HandleCommandL(aCommand);
			break;
		default:
			break;
		}
	}

void CMobblerBrowserView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
    TRACER_AUTO;
	if (!iBrowserControl)
		{
		// Make the app fullscreen but show the softkeys
		if (StatusPane()->CurrentLayoutResId() != R_AVKON_STATUS_PANE_LAYOUT_EMPTY)
			{
			StatusPane()->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_EMPTY);
			}
		iBrowserControl = CMobblerBrowserControl::NewL(	AppUi()->ClientRect(),
														*static_cast<CMobblerAppUi*>(AppUi()),
														aCustomMessageId,
														aCustomMessage);
		iBrowserControl->SetMopParent(AppUi());

		iBrowserControl->ActivateL();
		AppUi()->AddToStackL(*this, iBrowserControl);
		}
	}

void CMobblerBrowserView::DoDeactivate()
	{
    TRACER_AUTO;
	if (iBrowserControl)
		{
		AppUi()->RemoveFromStack(iBrowserControl);
		delete iBrowserControl;
		iBrowserControl = NULL;
		}
	}

void CMobblerBrowserView::HandleStatusPaneSizeChange()
	{
    TRACER_AUTO;
	CAknView::HandleStatusPaneSizeChange();
	}

// End of file
