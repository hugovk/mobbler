/*
mobblerdownloadmanager.cpp

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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

#include <eikenv.h>
#include <apgcli.h>
#include <aknwaitdialog.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobblerappui.h"
#include "mobblerresourcereader.h"

TUid KMobblerAppUid = {0xA0007648};

CMobblerDownload* CMobblerDownload::NewL(MMobblerDownloadObserver& aDownloadObserver)
	{
	CMobblerDownload* self = new(ELeave) CMobblerDownload(aDownloadObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerDownload::CMobblerDownload(MMobblerDownloadObserver& aDownloadObserver)
	:iDownloadObserver(aDownloadObserver)
	{
	}

void CMobblerDownload::ConstructL()
	{
	iDownloadMgr.ConnectL(KMobblerAppUid, *this, EFalse);
    iDownloadMgr.DeleteAll();
	}

CMobblerDownload::~CMobblerDownload()
	{
	if (iWait)
		{
		iWait->ProcessFinishedL();
		}
	    		
	iDownloadMgr.Close();
	}

void CMobblerDownload::DownloadL(const TDesC8& aDownloadUrl, TUint32 aIap)
	{
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
    		
	        TInt32 error = 0;
	        TInt32 globalError = 0;
	        aDownload.GetIntAttribute(EDlAttrErrorId, error);
	        aDownload.GetIntAttribute(EDlAttrGlobalErrorId, globalError);
	        }
    		break;
    	default:
    		break;
    	};
	}
