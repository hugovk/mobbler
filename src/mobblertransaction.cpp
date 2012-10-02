/*
mobblertransaction.cpp

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

#include <chttpformencoder.h> 
#include <http/rhttpheaders.h>
#include <httpstringconstants.h>
#include <imcvcodc.h> 

#include "mobblertracer.h"
#include "mobblertransaction.h"
#include "mobblerwebservicesquery.h"

// The granularity of the buffer that responses from Last.fm are read into
const TInt KBufferGranularity(256);

_LIT8(KSk, "sk");
_LIT8(KFormEncoding, "application/x-www-form-urlencoded");

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFmConnection& aConnection, CUri8* aURI)
	{
    TRACER_AUTO;
	CMobblerTransaction* self(new(ELeave) CMobblerTransaction(aConnection, EFalse));
	CleanupStack::PushL(self);
	self->ConstructL(aURI);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication, CUri8* aURI, CHTTPFormEncoder* aForm)
	{
    TRACER_AUTO;
	CMobblerTransaction* self(new(ELeave) CMobblerTransaction(aConnection, aRequiresAuthentication));
	CleanupStack::PushL(self);
	self->ConstructL(aURI, aForm);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFmConnection& aConnection, 
											   TBool aRequiresAuthentication, 
											   CUri8* aURI, 
											   CMobblerWebServicesQuery* aQuery)
	{
    TRACER_AUTO;
	CMobblerTransaction* self(new(ELeave) CMobblerTransaction(aConnection, aRequiresAuthentication));
	CleanupStack::PushL(self);
	self->ConstructL(aURI, aQuery);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication)
	{
    TRACER_AUTO;
	CMobblerTransaction* self(new(ELeave) CMobblerTransaction(aConnection, aRequiresAuthentication));
	//CleanupStack::PushL(self);
	//self->ConstructL();
	//CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction::CMobblerTransaction(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication)
	:iConnection(aConnection), iRequiresAuthentication(aRequiresAuthentication)
	{
    TRACER_AUTO;
	}

void CMobblerTransaction::ConstructL(CUri8* aUri)
	{
    TRACER_AUTO;
	iURI = aUri;
	}

void CMobblerTransaction::ConstructL(CUri8* aUri, CHTTPFormEncoder* aForm)
	{
    TRACER_AUTO;
	iForm = aForm;
	iURI = aUri;
	}

void CMobblerTransaction::ConstructL(CUri8* aUri, CMobblerWebServicesQuery* aQuery)
	{
    TRACER_AUTO;
	iQuery = aQuery;
	iURI = aUri;
	}

RHTTPTransaction& CMobblerTransaction::Transaction()
	{
    TRACER_AUTO;
	return iTransaction;
	}

void CMobblerTransaction::SubmitL()
	{
    TRACER_AUTO;
	delete iBuffer;
	iBuffer = CBufFlat::NewL(KBufferGranularity);
	
	if (iURI)
		{
		if (iQuery)
			{
            if (iConnection.iWebServicesSessionKey)
                {
                // we were passed a query so add the session key
                iQuery->AddFieldL(KSk, *iConnection.iWebServicesSessionKey);
                }
			
			iForm = iQuery->GetFormLC();
			CleanupStack::Pop(iForm);
			delete iQuery;
			iQuery = NULL;
			}
		
		if (iForm)
			{
			// get the post string
			RStringF string;
			RStringPool stringPool(iConnection.iHTTPSession.StringPool());
			string = stringPool.StringF(HTTP::EPOST, RHTTPSession::GetTable());
			
			iTransaction = iConnection.iHTTPSession.OpenTransactionL(iURI->Uri(), *this, string);
			iTransaction.Request().SetBody(*iForm);
			
			// get the header
			RStringF contentType(stringPool.OpenFStringL(KFormEncoding));
			THTTPHdrVal accVal(contentType);
			iTransaction.Request().GetHeaderCollection().SetFieldL(stringPool.StringF(HTTP::EContentType, RHTTPSession::GetTable()), accVal);
			contentType.Close();
			}
		else
			{
			iTransaction = iConnection.iHTTPSession.OpenTransactionL(iURI->Uri(), *this);
			}
		}
	
	iTransaction.SubmitL();
	}

CMobblerTransaction::~CMobblerTransaction()
	{
    TRACER_AUTO;
	iTransaction.Close();
	delete iBuffer;
	delete iForm;
	delete iURI;
	delete iQuery;
	}

void CMobblerTransaction::Cancel()
	{
    TRACER_AUTO;
	iTransaction.Cancel();
	}

void CMobblerTransaction::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{
    TRACER_AUTO;
	// it must be a transaction event
	TPtrC8 nextDataPartPtr;
	
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseHeaders:
			{
			RHTTPHeaders headers(aTransaction.Response().GetHeaderCollection());
			 
			THTTPHdrVal locationValue;
			if( headers.GetField(iConnection.iHTTPSession.StringPool().StringF(HTTP::ELocation, RHTTPSession::GetTable()), 0, locationValue) == KErrNone )
				{
				// This is a redirect so ask for the new location
				
				const TDesC8& urides(locationValue.StrF().DesC());
				TUriParser8 uri;
				uri.Parse(urides);
				aTransaction.Cancel();
				iTransaction.Request().SetURIL(uri);
				iTransaction.SubmitL();
				}
			}
			break;
		case THTTPEvent::EGotResponseBodyData:
			aTransaction.Response().Body()->GetNextDataPart(nextDataPartPtr);
			iBuffer->InsertL(iBuffer->Size(), nextDataPartPtr);
			aTransaction.Response().Body()->ReleaseData();
			break;
		case THTTPEvent::ESucceeded:
			{
			HBufC8* response(iBuffer->Ptr(0).AllocLC());
			iConnection.TransactionResponseL(this, *response);
			CleanupStack::PopAndDestroy(response);
			}
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			iConnection.TransactionCompleteL(this);
			break;
		case THTTPEvent::EFailed:
			{
			HBufC8* response(iBuffer->Ptr(0).AllocLC());
			iConnection.TransactionFailedL(this, *response, iTransaction.Response().StatusText().DesC(), iTransaction.Response().StatusCode());
			CleanupStack::PopAndDestroy(response);
			}
			break;
		default:
			break;
		}
	}

TInt CMobblerTransaction::MHFRunError(TInt aError, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
    TRACER_AUTO;
	_LIT8(KMHFRunError, "MHFRunError");
	iConnection.TransactionFailedL(this, KNullDesC8, KMHFRunError, aError);
	return KErrNone;
	}

void CMobblerTransaction::SetFlatDataObserver(MMobblerFlatDataObserver* aFlatDataObserver)
	{
    TRACER_AUTO;
	iFlatDataObserver = aFlatDataObserver;
	}

MMobblerFlatDataObserver* CMobblerTransaction::FlatDataObserver()
	{
    TRACER_AUTO;
	return iFlatDataObserver;
	}

TBool CMobblerTransaction::RequiresAuthentication() const
	{
    TRACER_AUTO;
	return iRequiresAuthentication;
	}

// End of file
