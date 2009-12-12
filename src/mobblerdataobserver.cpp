/*
mobblerdataobserver.cpp

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

#include <aknwaitdialog.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerdataobserver.h"
#include "mobblerresourcereader.h"

CMobblerFlatDataObserverHelper* CMobblerFlatDataObserverHelper::NewL(
									CMobblerLastFmConnection& aConnection,
									MMobblerFlatDataObserverHelper& aObserver,
									TBool aShowWaitDialog)
	{
	CMobblerFlatDataObserverHelper* self(new(ELeave) CMobblerFlatDataObserverHelper(aConnection, aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aShowWaitDialog);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerFlatDataObserverHelper::CMobblerFlatDataObserverHelper(CMobblerLastFmConnection& aConnection, MMobblerFlatDataObserverHelper& aObserver)
	:iConnection(aConnection), iObserver(aObserver)
	{
	}

void CMobblerFlatDataObserverHelper::ConstructL(TBool aShowWaitDialog)
	{
	if (aShowWaitDialog)
		{
		iWaitDialog = new (ELeave) CAknWaitDialog((REINTERPRET_CAST(CEikDialog**, &iWaitDialog)), ETrue);
		iWaitDialog->SetCallback(this);
		iWaitDialog->SetTextL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_PLEASE_WAIT));
		iWaitDialog->ExecuteLD(R_MOBBLER_WAIT_DIALOG);
		}
	}

CMobblerFlatDataObserverHelper::~CMobblerFlatDataObserverHelper()
	{
	if(iWaitDialog)
		{
		iWaitDialog->ProcessFinishedL(); 
		}
	}

void CMobblerFlatDataObserverHelper::DialogDismissedL(TInt aButtonId)
	{
	iWaitDialog = NULL;
	
	if (aButtonId == EAknSoftkeyCancel)
		{
		iConnection.CancelTransaction(this);
		}
	}

void CMobblerFlatDataObserverHelper::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (iWaitDialog)
		{
		iWaitDialog->ProcessFinishedL(); 
		iWaitDialog = NULL;
		}
	
	iObserver.DataL(this, aData, aTransactionError);
	}

// End of file
