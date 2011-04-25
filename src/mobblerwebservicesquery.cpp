/*
mobblerwebservicesquery.cpp

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

#include "mobblerliterals.h"
#include "mobblertracer.h"
#include "mobblerutility.h"
#include "mobblerwebservicesquery.h"

_LIT8(KMobblerApiKey, "31e4e200b2c0b8aa90d9829534ad40a6");
_LIT8(KMobblerSecretKey, "49116c5cf8d2882380cd98f19174a70a");

_LIT8(KMobblerParameterApiKey, "api_key");
_LIT8(KMobblerParameterApiSig, "api_sig");
_LIT8(KMobblerParameterMethod, "method");

CMobblerWebServicesQuery* CMobblerWebServicesQuery::NewLC(const TDesC8& aMethod)
	{
    TRACER_AUTO;
	CMobblerWebServicesQuery* self(new(ELeave) CMobblerWebServicesQuery());
	CleanupStack::PushL(self);
	self->ConstructL(aMethod);
	//CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesQuery::CMobblerWebServicesQuery()
	{
    TRACER_AUTO;
	}

void CMobblerWebServicesQuery::ConstructL(const TDesC8& aMethod)
	{
    TRACER_AUTO;
	AddFieldL(KMobblerParameterMethod, aMethod);
	AddFieldL(KMobblerParameterApiKey, KMobblerApiKey);
	}

CMobblerWebServicesQuery::~CMobblerWebServicesQuery()
	{
    TRACER_AUTO;
	const TInt KFieldCount(iFields.Count());
	for (TInt i(0); i < KFieldCount; ++i)
		{
		delete iFields[i].iParameter;
		delete iFields[i].iValue;
		}
	
	iFields.Close();
	}

void CMobblerWebServicesQuery::AddFieldL(const TDesC8& aParameter, const TDesC8& aValue)
	{
    TRACER_AUTO;
	TMobblerWebServicesQueryField field;
	field.iParameter = aParameter.AllocL();
	field.iValue = aValue.AllocL();
	TLinearOrder<TMobblerWebServicesQueryField> linearOrder(Compare);
	iFields.InsertInOrder(field, linearOrder);
	}

HBufC8* CMobblerWebServicesQuery::GetQueryAuthLC() const
	{
    TRACER_AUTO;
    TInt queryTextLength(0);
    TInt apiSigLength(0);
    
	// add all the fields
	const TInt KFieldCount(iFields.Count());
	for (TInt i(0); i < KFieldCount; ++i)
		{
		// add the fields for the normal query
		if (i > 0)
			{
			queryTextLength += KAmpersand.Length();
			}
		
		queryTextLength += iFields[i].iParameter->Length();
		queryTextLength += KEquals.Length();
		queryTextLength += iFields[i].iValue->Length();
		
		// append to the api sig
		apiSigLength += iFields[i].iParameter->Length();
		apiSigLength += iFields[i].iValue->Length();
		}
	
	// create and add the api_sig
	apiSigLength += KMobblerSecretKey().Length();
	
	queryTextLength += KAmpersand.Length();
	queryTextLength += KMobblerParameterApiSig().Length();
	queryTextLength += KEquals.Length();
	queryTextLength += 32; // apiSigHash will be 32 chars long
    
    HBufC8* queryText(HBufC8::NewLC(queryTextLength));
	HBufC8* apiSig(HBufC8::NewLC(apiSigLength));
	
	// add all the fields
	for (TInt i(0); i < KFieldCount; ++i)
		{
		// add the fields for the normal query
		if (i > 0)
			{
			queryText->Des().Append(KAmpersand);
			}
		
		queryText->Des().Append(*iFields[i].iParameter);
		queryText->Des().Append(KEquals);
		queryText->Des().Append(*iFields[i].iValue);
		
		// append to the api sig
		apiSig->Des().Append(*iFields[i].iParameter);
		apiSig->Des().Append(*iFields[i].iValue);
		}
	
	// create and add the api_sig
	apiSig->Des().Append(KMobblerSecretKey);
	HBufC8* apiSigHash(MobblerUtility::MD5LC(*apiSig));
	
	queryText->Des().Append(KAmpersand);
	queryText->Des().Append(KMobblerParameterApiSig);
	queryText->Des().Append(KEquals);
	queryText->Des().Append(*apiSigHash);
	
	CleanupStack::PopAndDestroy(2, apiSig);
	
	return queryText;
	}

HBufC8* CMobblerWebServicesQuery::GetQueryLC() const
	{
    TRACER_AUTO;
    TInt length(0);
    
	const TInt KFieldCount(iFields.Count());
	for (TInt i(0); i < KFieldCount; ++i)
		{
		// find the length
		length += KAmpersand.Length();
		length += iFields[i].iParameter->Length();
		length += KEquals.Length();
		length += iFields[i].iValue->Length();
		}
    
	HBufC8* queryText(HBufC8::NewLC(length));
	
	// add all the fields
	for (TInt i(0); i < KFieldCount; ++i)
		{
		// add the fields for the normal query
		queryText->Des().Append(KAmpersand);
		queryText->Des().Append(*iFields[i].iParameter);
		queryText->Des().Append(KEquals);
		queryText->Des().Append(*iFields[i].iValue);
		}
	
	return queryText;
	}

CHTTPFormEncoder* CMobblerWebServicesQuery::GetFormLC() const
	{
    TRACER_AUTO;
	CHTTPFormEncoder* form(CHTTPFormEncoder::NewL());
	CleanupStack::PushL(form);
	
	TInt apiSigLength(KMobblerSecretKey().Length());
	
	// add all the fields
	const TInt KFieldCount(iFields.Count());
	for (TInt i(0); i < KFieldCount; ++i)
		{
		apiSigLength += iFields[i].iParameter->Length();
		apiSigLength += iFields[i].iValue->Length();
		}
	
	HBufC8* apiSig(HBufC8::NewLC(apiSigLength));
	
	// add all the fields
	for (TInt i(0); i < KFieldCount; ++i)
		{
		// add the fields for the normal query
		form->AddFieldL(*iFields[i].iParameter, *iFields[i].iValue);
		
		// append to the api sig
		apiSig->Des().Append(*iFields[i].iParameter);
		apiSig->Des().Append(*iFields[i].iValue);
		}
	
	// create and add the api_sig
	apiSig->Des().Append(KMobblerSecretKey);
	HBufC8* apiSigHash(MobblerUtility::MD5LC(*apiSig));
	
	form->AddFieldL(KMobblerParameterApiSig, *apiSigHash);
	
	CleanupStack::PopAndDestroy(2, apiSig);
	
	return form;
	}

TInt CMobblerWebServicesQuery::Compare(const TMobblerWebServicesQueryField& aLeft, const TMobblerWebServicesQueryField& aRight)
	{
    TRACER_AUTO;
	return aLeft.iParameter->Compare(*aRight.iParameter);
	}

// End of file
