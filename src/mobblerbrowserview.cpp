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

CMobblerBrowserView* CMobblerBrowserView::NewL()
	{
	CMobblerBrowserView* self(new (ELeave) CMobblerBrowserView);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBrowserView::CMobblerBrowserView()
	{
	}

CMobblerBrowserView::~CMobblerBrowserView()
	{
	delete iBrowserControl;
	}

void CMobblerBrowserView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_BROWSER_VIEW);
	}

void CMobblerBrowserView::SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, TInt aCommandId)
	{
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
	}

TUid CMobblerBrowserView::Id() const
	{
	return TUid::Uid(KMobblerBrowserViewUid);
	}

void CMobblerBrowserView::HandleCommandL(TInt aCommand)
	{
	//iWebServicesControl->HandleListCommandL(aCommand);

	switch (aCommand)
		{
		case EAknSoftkeyBack:
			AppUi()->ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid)); // switch back to the status view
			break;
		case EAknSoftkeyExit:
			AppUi()->HandleCommandL(aCommand);
			break;
		default:
			break;
		}
	}

void CMobblerBrowserView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	if (!iBrowserControl)
		{
		iBrowserControl = CMobblerBrowserControl::NewL(AppUi()->ClientRect(), *static_cast<CMobblerAppUi*>(AppUi()));
		iBrowserControl->SetMopParent(AppUi());

		iBrowserControl->ActivateL();
		AppUi()->AddToStackL(*this, iBrowserControl);
		}
	}

void CMobblerBrowserView::DoDeactivate()
	{
	if (iBrowserControl)
		{
		AppUi()->RemoveFromStack(iBrowserControl);
		delete iBrowserControl;
		iBrowserControl = NULL;
		}
	}

void CMobblerBrowserView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	}

// End of file
