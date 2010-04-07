/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  gw111zz
Copyright (C) 2010  Hugo van Kemenade

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

#include <brctlinterface.h>
#include <charconv.h>

#include "mobblerappui.h"
#include "mobblerbrowsercontrol.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"

CMobblerBrowserControl* CMobblerBrowserControl::NewL(const TRect& aRect, CMobblerAppUi& aMobblerAppUi, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	TRACER_AUTO;
	CMobblerBrowserControl* self(new(ELeave) CMobblerBrowserControl(aMobblerAppUi));
	CleanupStack::PushL(self);
	self->ConstructL(aRect, aCustomMessageId, aCustomMessage);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBrowserControl::CMobblerBrowserControl(CMobblerAppUi& aMobblerAppUi) :
	iAppUi(aMobblerAppUi), iBrowserLoadObserver(iAppUi.LastFmConnection())
	{
	TRACER_AUTO;
	}

void CMobblerBrowserControl::ConstructL(const TRect& aRect, TUid /*aCustomMessageId*/, const TDesC8& aCustomMessage)
	{
	TRACER_AUTO;
	CreateWindowL();
	SetRect(aRect);

	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		iBrCtlInterface = NULL;
		}

	// TODO: need to detect 3rd Ed and use
	// TRect brCtlRect(Position(), Size());   // in 3rd Ed devices
	// need a device to test on first
	TRect brCtlRect(TPoint(0, 0), Size()); // in 3rd Ed, FP1 devices

	iBrCtlInterface = ::CreateBrowserControlL(
			this,
			brCtlRect,
			TBrCtlDefs::ECapabilityDisplayScrollBar,
			TBrCtlDefs::ECommandIdBase,
			NULL,
			NULL,
			&iBrowserLoadObserver);

	_LIT8(KHtmlDataType, "text/html");
	TDataType dataType(KHtmlDataType());
	TUid uid;
	uid.iUid = KCharacterSetIdentifierUtf8;

	_LIT(KHttpDummy, "http://dummy");
	iBrCtlInterface->LoadDataL(KHttpDummy, aCustomMessage, dataType, uid);
	}

CMobblerBrowserControl::~CMobblerBrowserControl()
	{
	TRACER_AUTO;
	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		}
	}

void CMobblerBrowserControl::SizeChanged()
	{
	TRACER_AUTO;
	if (iBrCtlInterface)
		{
		iBrCtlInterface->SetRect(Rect());
		}
	}

TInt CMobblerBrowserControl::CountComponentControls() const
	{
	TRACER_AUTO;
	return 1;
	}

CCoeControl* CMobblerBrowserControl::ComponentControl(TInt aIndex) const
	{
	TRACER_AUTO;
	switch (aIndex)
		{
		case 0:
			return iBrCtlInterface;
		default:
			return NULL;
		}
	}

void CMobblerBrowserControl::HandleResourceChange(TInt aType)
	{
	TRACER_AUTO;
	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		TRect rect;
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);
		SetRect(rect);
		iAppUi.StatusPane()->MakeVisible(ETrue);
		}

	CCoeControl::HandleResourceChange(aType);
	}

void CMobblerBrowserControl::Draw(const TRect& /*aRect*/) const
	{
	TRACER_AUTO;
	CWindowGc& gc(SystemGc());
	gc.Clear(Rect());
	}

TKeyResponse CMobblerBrowserControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	TRACER_AUTO;
	// This is a hack because the CBrCtlInterface which is part of the
	// platform makes both the left and right arrow keys scroll the page
	// up which is counter-intuitive. So change right to down before passing
	// the key event to the control.
	TKeyEvent newKeyEvent(aKeyEvent);
	if (aKeyEvent.iCode == EKeyRightArrow)
		{
		newKeyEvent.iCode = EKeyDownArrow;
		}

	return iBrCtlInterface->OfferKeyEventL(newKeyEvent, aEventCode);
	}

// End of file
