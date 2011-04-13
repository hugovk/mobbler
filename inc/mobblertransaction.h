/*
mobblertransaction.h

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

#ifndef __MOBBLERTRANSACTION_H__
#define __MOBBLERTRANSACTION_H__

#include <e32base.h>
#include <http/mhttptransactioncallback.h> 

#include "mobblerdataobserver.h"

class CHTTPFormEncoder;
class CMobblerLastFmConnection;
class CMobblerWebServicesQuery;

class CMobblerTransaction : public CBase, public MHTTPTransactionCallback
	{
public:
	static CMobblerTransaction* NewL(CMobblerLastFmConnection& aConnection, CUri8* aURI);
	static CMobblerTransaction* NewL(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication, CUri8* aURI, CHTTPFormEncoder* aForm);
	static CMobblerTransaction* NewL(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication, CUri8* aURI, CMobblerWebServicesQuery* aQuery);
	
	static CMobblerTransaction* NewL(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication);
	
	~CMobblerTransaction();
	
	TBool RequiresAuthentication() const;
	
	void SetTwitter();
	void AddTwitterOAuthStringL(const TDesC8& aString);
	
	void SubmitL();
	void Cancel();
	
	void SetFlatDataObserver(MMobblerFlatDataObserver* aFlatDataObserver);
	MMobblerFlatDataObserver* FlatDataObserver();
	
	RHTTPTransaction& Transaction();
	
private: // from MHTTPTransactionCallback
	void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent);
	TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	
private:
	CMobblerTransaction(CMobblerLastFmConnection& aConnection, TBool aRequiresAuthentication);
	void ConstructL(CUri8* aURI);
	void ConstructL(CUri8* aURI, CHTTPFormEncoder* aForm);
	void ConstructL(CUri8* aURI, CMobblerWebServicesQuery* aQuery);
	
private:
	CMobblerLastFmConnection& iConnection;
	MMobblerFlatDataObserver* iFlatDataObserver;
	
	CMobblerWebServicesQuery* iQuery;
	
	RHTTPTransaction iTransaction;
	CUri8* iURI;
	CHTTPFormEncoder* iForm;
	
	CBufBase* iBuffer;
	
	TBool iRequiresAuthentication;
	
	TBool iTwitter;
	CDesC8Array* iTwitterOAuthStrings;
	};

#endif // __MOBBLERTRANSACTION_H__

// End of file
