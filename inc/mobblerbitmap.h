/*
mobblerbitmap.h

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

#ifndef __MOBBLERBITMAP_H__
#define __MOBBLERBITMAP_H__

#include <coecntrl.h>
#include <coedef.h>

class CBitmapScaler;
class CImageDecoder;
class CMobblerBitmap;

class MMobblerBitmapObserver
	{
public:
	virtual void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap) = 0;
	virtual void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap) = 0;
	};

class CMobblerBitmap : public CActive
	{
public:
	enum TMobblerScaleStatus
		{
		EMobblerScaleNone,
		EMobblerScalePending
		};
	
public:
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex);
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, TUid aAppUid);
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, const TDesC& aFileName, const TUid aImageType = KNullUid);
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, const TDesC8& aData, const TUid aImageType = KNullUid);
	
	void Open() const;
	void Close() const;
	
#ifdef __SYMBIAN_SIGNED__
	CFbsBitmap* Bitmap(TBool aOriginalPlease = EFalse) const;
#else
	CFbsBitmap* Bitmap() const;
#endif
	CFbsBitmap* BitmapGrayL() const;
	CFbsBitmap* Mask() const;
	
	void SetSize(TSize aSize);
	TSize SizeInPixels() const;

	void ScaleL(TSize aSize);
	TMobblerScaleStatus ScaleStatus() const;
	static TBool LongSidesEqual(TSize aLeftSize, TSize aRightSize);
	
	void SetCallbackCancelled(TBool aCallbackCancelled);
	void SetObserver(MMobblerBitmapObserver& aObserver);
	
private:
	void ConstructL(const TDesC& aFileName, const TUid aFileUid);
	void ConstructL(const TDesC8& aData, const TUid aFileUid);
	void ConstructL(TUid aAppUid);
	void ConstructL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex);
	
	CMobblerBitmap(MMobblerBitmapObserver* aObserver);
	~CMobblerBitmap();
	
private:
	void RunL();
	void DoCancel();
	
private:
	MMobblerBitmapObserver* iObserver;
	TBool iCallbackCancelled;
	
	TBool iBitmapLoaded;
	CImageDecoder* iImageDecoder;
	CFbsBitmap* iBitmap;
	mutable CFbsBitmap* iBitmapGray;
	CFbsBitmap* iMask;
	HBufC8* iData;
	
	HBufC* iMifFileName;
	TInt iMifBitmapIndex;
	TInt iMifMaskIndex;

	CBitmapScaler* iBitmapScaler;
	CFbsBitmap* iScaledBitmap;
	CFbsBitmap* iOriginalBitmap;
	TMobblerScaleStatus iScaleStatus;
	
	mutable TInt iRefCount;
	};

#endif

// End of file
