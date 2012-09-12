/*
mobblerdownload.cpp

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

#include <aknwaitdialog.h>
#include <apgcli.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerresourcereader.h"
#include "mobblertracer.h"

CMobblerDownload* CMobblerDownload::NewL(MMobblerDownloadObserver& aDownloadObserver)
	{
    TRACER_AUTO;
	CMobblerDownload* self(new(ELeave) CMobblerDownload(aDownloadObserver));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerDownload::CMobblerDownload(MMobblerDownloadObserver& aDownloadObserver)
	:iDownloadObserver(aDownloadObserver)
	{
    TRACER_AUTO;
	}

void CMobblerDownload::ConstructL()
	{
    TRACER_AUTO;
	iDownloadMgr.ConnectL(TUid::Uid(KMobblerAppUid), *this, EFalse);
	iDownloadMgr.DeleteAll();
	}

CMobblerDownload::~CMobblerDownload()
	{
    TRACER_AUTO;
	if (iWait)
		{
		iWait->ProcessFinishedL();
		}
	
	iDownloadMgr.Close();
	}

void CMobblerDownload::DownloadL(const TDesC8& aDownloadUrl, TUint32 aIap)
	{
    TRACER_AUTO;
	User::LeaveIfError(iDownloadMgr.SetIntAttribute(EDlMgrIap, aIap)); 
	RHttpDownload& download(iDownloadMgr.CreateDownloadL(aDownloadUrl));
	
	download.SetBoolAttribute(EDlAttrNoContentTypeCheck, ETrue);
	download.Start();
	
	// start a progress dialog
	if (iWait)
		{
		iWait->ProcessFinishedL();
		iWait = NULL;
		}
	
	iWait = new(ELeave) CAknWaitDialog((REINTERPRET_CAST(CEikDialog**, &iWait)));
	iWait->SetTextL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_DOWNLOADING));
	iWait->ExecuteLD(R_MOBBLER_DOWNLOADING_DIALOG);
	}

void CMobblerDownload::HandleDMgrEventL(RHttpDownload& aDownload, THttpDownloadEvent aEvent)
	{
    TRACER_AUTO;
	if(EHttpContentTypeReceived == aEvent.iProgressState)
		{
		// Start download again if content-type is acceptable 
		// and UiLib is not installed
		User::LeaveIfError(aDownload.Start());
		}
	
	switch (aEvent.iDownloadState)
		{
		case EHttpDlCompleted:
			{
			TFileName fileName;
			aDownload.GetStringAttribute(EDlAttrDestFilename, fileName);
			
			RApaLsSession apaLsSession;
			CleanupClosePushL(apaLsSession);
			User::LeaveIfError(apaLsSession.Connect());
			
			if (iWait)
				{
				iWait->ProcessFinishedL();
				iWait = NULL;
				}
			
			TThreadId threadId;
			User::LeaveIfError(apaLsSession.StartDocument(fileName, threadId));
			
			CleanupStack::PopAndDestroy(&apaLsSession);
			
			iDownloadObserver.HandleInstallStartedL();
			}
			break;
		case EHttpDlFailed:
			{
			if (iWait)
				{
				iWait->ProcessFinishedL();
				iWait = NULL;
				}
			
			TInt32 error(0);
			TInt32 globalError(0);
			aDownload.GetIntAttribute(EDlAttrErrorId, error);
			aDownload.GetIntAttribute(EDlAttrGlobalErrorId, globalError);
			}
			break;
		default:
			break;
		};
	}

// End of file
