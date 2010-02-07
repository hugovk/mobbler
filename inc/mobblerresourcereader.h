/*
mobblerresourcereader.h

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

#ifndef __MOBBLERRESOURCEREADER_H__
#define __MOBBLERRESOURCEREADER_H__

class CMobblerResourceReader : public CActive
	{
private:
	class CMobblerResource : public CBase
		{
	public:
		CMobblerResource(TInt aResourceId, HBufC* aString);
		~CMobblerResource();
		
		const TDesC& String() const;
		
		static TInt Compare(const TInt* aResourceId, const CMobblerResource& aResource);
		static TInt Compare(const CMobblerResource& aLeft, const CMobblerResource& aRight);
		
	private:
		TInt iResourceId;
		HBufC* iString;
		};
	
public:
	static CMobblerResourceReader* NewL();
	~CMobblerResourceReader();
	
	const TDesC& ResourceL(TInt aResourceId);

private:
	CMobblerResourceReader();
	void ConstructL();
	
private:
	void RunL();
	void DoCancel();

private:
	HBufC* iStringNotFoundInResouce;
	TFileName iLanguageRscFile;
	
	RTimer iTimer;
	
	RResourceFile iResourceFile;
	RResourceFile iResourceFileDefault;
	
	mutable TBool iErrorDialogShown;
	mutable RPointerArray<CMobblerResource> iResources;
	
	TLinearOrder<CMobblerResource> iLinearOrder;
	};

#endif // __MOBBLERRESOURCEREADER_H__

// End of file
