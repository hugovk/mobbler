/*
mobblerwebservicescontrol.cpp

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

#include <aknnavide.h>
#include <aknnavilabel.h>

#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerlistcontrol.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerwebservicescontrol.h"

#include "mobbler.hrh"

CMobblerWebServicesControl* CMobblerWebServicesControl::NewL(CMobblerAppUi& aAppUi, const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
    TRACER_AUTO;
	CMobblerWebServicesControl* self(new(ELeave) CMobblerWebServicesControl(aAppUi));
	CleanupStack::PushL(self);
	self->ConstructL(aRect, aCustomMessageId, aCustomMessage);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesControl::CMobblerWebServicesControl(CMobblerAppUi& aAppUi)
	:iAppUi(aAppUi)
	{
    TRACER_AUTO;
	}

void CMobblerWebServicesControl::ConstructL(const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
    TRACER_AUTO;
	CreateWindowL(); // This is a window owning control
	InitComponentArrayL();
	SetRect(aRect);
	
	CMobblerListControl* control(NULL);
	
	switch (aCustomMessageId.iUid)
		{
		case EMobblerCommandSimilarArtists:
		case EMobblerCommandSimilarTracks:
			// These can only be called if there is a current track playing
			if (iAppUi.CurrentTrack())
				{
				control = CMobblerListControl::CreateListL(iAppUi, *this, 
						aCustomMessageId.iUid, 
						iAppUi.CurrentTrack()->Artist().String8(), 
						iAppUi.CurrentTrack()->Title().String8());
				}
			break;
		default:
			control = CMobblerListControl::CreateListL(iAppUi, *this, aCustomMessageId.iUid, aCustomMessage, KNullDesC8);
			break;
		};
	
	iControls.AppendL(control);
	control->SetMopParent(&iAppUi);
	control->ActivateL();
	iAppUi.AddToStackL(control);
	
	iNaviContainer = static_cast<CAknNavigationControlContainer*>(iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi)));
	iNaviLabelDecorator = iNaviContainer->CreateNavigationLabelL();
	iNaviContainer->PushL(*iNaviLabelDecorator);
	
	iAppUi.LastFmConnection().AddStateChangeObserverL(this);
	
	ChangePaneTextL();
	}

CMobblerWebServicesControl::~CMobblerWebServicesControl()
	{
    TRACER_AUTO;
	iAppUi.LastFmConnection().RemoveStateChangeObserver(this);
	
	// remove the top control from the stack
	iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
	iControls.ResetAndDestroy();
	
	iNaviContainer->Pop(iNaviLabelDecorator);
	delete iNaviLabelDecorator;
	}

CMobblerListControl* CMobblerWebServicesControl::TopControl()
	{
    TRACER_AUTO;
	return iControls[iControls.Count() - 1];
	}

void CMobblerWebServicesControl::HandleListCommandL(TInt aCommand)
	{
    TRACER_AUTO;
	CMobblerListControl* list(iControls[iControls.Count() - 1]->HandleListCommandL(aCommand));
	
	if (list)
		{
		// remove the old top control from the stack
		iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
		
		// add and activate the new list 
		iControls.AppendL(list);
		list->SetMopParent(&iAppUi);
		list->ActivateL();
		iAppUi.AddToStackL(list);
		
		DrawDeferred();
		
		ChangePaneTextL();
		}
	}

void CMobblerWebServicesControl::HandleConnectionStateChangedL()
	{
    TRACER_AUTO;
	ChangePaneTextL();
	}

void CMobblerWebServicesControl::HandleListControlStateChangedL()
	{
    TRACER_AUTO;
	ChangePaneTextL();
	}

void CMobblerWebServicesControl::ChangePaneTextL()
	{
    TRACER_AUTO;
	TBuf<KMaxMobblerTextSize> text;
	
	text.AppendFill('>', Max(0, iControls.Count() - 1));
	
	CMobblerListControl::TState state(iControls.Count() > 0 ? TopControl()->State() : CMobblerListControl::ELoading);
	
	switch (state)
		{
		case CMobblerListControl::ELoading:
			switch (iAppUi.LastFmConnection().State())
				{
				case CMobblerLastFmConnection::EConnecting:
					text.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_CONNECTING));
					break;
				case CMobblerLastFmConnection::EHandshaking:
					text.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_HANDSHAKING));
					break;
				case CMobblerLastFmConnection::ENone:
					text.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_LOADING));
					break;
				}
			break;
		case CMobblerListControl::ENormal:
			if (iControls.Count() > 0)
				{
				HBufC* name(TopControl()->NameL());
				text.Append(*name);
				delete name;
				}
			break;
		case CMobblerListControl::EFailed:
			text.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_FAILED));
			break;
		default:
			break;
		}
	
	if (iNaviLabelDecorator)
		{
		static_cast<CAknNaviLabel*>(iNaviLabelDecorator->DecoratedControl())->SetTextL(text);
		iNaviContainer->Pop();
		iNaviContainer->PushL(*iNaviLabelDecorator);	
		}
	}

void CMobblerWebServicesControl::ForwardL(CMobblerListControl* aListControl)
	{
    TRACER_AUTO;
	// remove the top control
	iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);

	// add the new top control to the stack
	iControls.AppendL(aListControl);
	aListControl->SetMopParent(&iAppUi);
	aListControl->ActivateL();
	iAppUi.AddToStackL(aListControl);
	
	ChangePaneTextL();

	DrawDeferred();
	}

void CMobblerWebServicesControl::BackL()
	{
    TRACER_AUTO;
	if (iControls.Count() == 1)
		{
		// switch back to the staus view
		iAppUi.ActivateLocalViewL(TUid::Uid(KMobblerStatusViewUid)); // switch back to the status view		
		}
	else
		{
		// remove and delete the top control
		iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
		delete iControls[iControls.Count() - 1];
		iControls.Remove(iControls.Count() - 1);

		// add the new top control to the stack
		iAppUi.AddToStackL(iControls[iControls.Count() - 1]);
		
		ChangePaneTextL();
		}
	
	DrawDeferred();
	}

TKeyResponse CMobblerWebServicesControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode /*aEventCode*/)
	{
    TRACER_AUTO;
	TKeyResponse response(EKeyWasNotConsumed);
	RArray<TInt> commands;
	CleanupClosePushL(commands);
			
	switch (aKeyEvent.iCode)
		{
		case EKeyLeftArrow:
			// the user pressed left so try to go up a control
			BackL();
			response = EKeyWasConsumed;
			break;
		case EKeyRightArrow:
			TopControl()->SupportedCommandsL(commands);
			
			if (commands.Find(EMobblerCommandOpen) != KErrNotFound)
				{
				HandleListCommandL(EMobblerCommandOpen);
				}
			
			response = EKeyWasConsumed;
			break;
		default:
			break;
		}
	
	DrawDeferred();
	
	CleanupStack::PopAndDestroy(&commands);
	
	return response;
	}


void CMobblerWebServicesControl::Draw(const TRect& /*aRect*/) const
	{
    TRACER_AUTO;
	CWindowGc& gc(SystemGc());
   	gc.Clear(Rect());
	}

CCoeControl* CMobblerWebServicesControl::ComponentControl(TInt /*aIndex*/) const
	{
    TRACER_AUTO;
	return iControls[iControls.Count() - 1];
	}
 
TInt CMobblerWebServicesControl::CountComponentControls() const
	{
    TRACER_AUTO;
	return 1;
	}

// End of file
