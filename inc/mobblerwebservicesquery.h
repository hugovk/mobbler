/*
mobblerwebservicesquery.h

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

#ifndef __MOBBLERWEBSERVICESQUERY_H__
#define __MOBBLERWEBSERVICESQUERY_H__

#include <e32base.h>

class TMobblerWebServicesQueryField
	{
public:
	HBufC8* iParameter;
	HBufC8* iValue;
	};

class CMobblerWebServicesQuery : public CBase
	{
public:
	static CMobblerWebServicesQuery* NewLC(const TDesC8& aMethod);
	~CMobblerWebServicesQuery();
	
	void AddFieldL(const TDesC8& aParameter, const TDesC8& aValue);
	
	HBufC8* GetQueryLC() const;
	HBufC8* GetQueryAuthLC() const;
	CHTTPFormEncoder* GetFormLC() const;
	
private:
	CMobblerWebServicesQuery();
	void ConstructL(const TDesC8& aMethod);
	
	static TInt Compare(const TMobblerWebServicesQueryField& aLeft, const TMobblerWebServicesQueryField& aRight);
	
private:
	RArray<TMobblerWebServicesQueryField> iFields;
	};

#endif // __MOBBLERWEBSERVICESQUERY_H__

// End of file
