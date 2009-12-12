/*
mobblercontacts.h

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

#ifndef __MOBBLERCONTACTS_H__
#define __MOBBLERCONTACTS_H__

#include <cntviewbase.h>

class CContactFilteredView;

class CMobblerContacts : public CBase, public MContactViewObserver
	{
public:
	static CMobblerContacts* NewLC();
	TInt Count() const;
	TPtrC GetNameAt(TInt aIndex) const; 
	CDesCArray* GetEmailsAtLC(TInt aIndex) const;
	HBufC8* GetPhotoAtL(TInt aIndex) const;
	~CMobblerContacts();
	
private:
	CMobblerContacts();
	void ConstructL(); 
	virtual void HandleContactViewEvent(const CContactViewBase& aView, const TContactViewEvent& aEvent);
	void BuildListL();
	
private:
	enum
		{
		EFirstName,
		ELastName,
		ECompanyName,
		EEmail
		};

private:
	CContactDatabase* iDb;
	CContactNamedRemoteView* iRemoteView;
	CContactFilteredView* iFilteredView;
	CDesCArray* iNameList;
	TInt iNumViews;
	TBool iListBuilt;
	};

#endif  // __MOBBLERCONTACTS_H__

// End of file
