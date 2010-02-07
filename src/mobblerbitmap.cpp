/*
mobblerbitmap.cpp

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

#include <aknsutils.h>
#include <BitmapTransforms.h>
#include <imageconversion.h>

#include "mobblerbitmap.h"

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex)
	{
	CMobblerBitmap* self(new(ELeave) CMobblerBitmap(&aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aMifFileName, aBitmapIndex, iMaskIndex);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, TUid aAppUid)
	{
	CMobblerBitmap* self(new(ELeave) CMobblerBitmap(&aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aAppUid);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, const TDesC& aFileName, const TUid aFileUid)
	{
	CMobblerBitmap* self(new(ELeave) CMobblerBitmap(&aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aFileName, aFileUid);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, const TDesC8& aData, const TUid aFileUid)
	{
	CMobblerBitmap* self(new(ELeave) CMobblerBitmap(&aObserver));
	CleanupStack::PushL(self);
	self->ConstructL(aData, aFileUid);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap::CMobblerBitmap(MMobblerBitmapObserver* aObserver)
	:CActive(CActive::EPriorityStandard), iObserver(aObserver), iRefCount(1)
	{
	CActiveScheduler::Add(this);
	}

CMobblerBitmap::~CMobblerBitmap()
	{
	Cancel();
	
	if (iScaledBitmap)
		{
		iScaledBitmap->Reset();
		}

	delete iBitmap;
	delete iBitmapGray;
	delete iScaledBitmap;
	delete iOriginalBitmap;
	delete iBitmapScaler;
	delete iMask;
	delete iImageDecoder;
	delete iData;
	delete iMifFileName;
	}

void CMobblerBitmap::Open() const
	{
	++iRefCount;
	}

void CMobblerBitmap::Close() const
	{
	if (this && --iRefCount == 0)
		{
		delete this;
		}
	}

void CMobblerBitmap::SetCallbackCancelled(TBool aCallbackCancelled)
	{
	if (aCallbackCancelled)
		{
		iObserver = NULL;
		}
	}

void CMobblerBitmap::SetObserver(MMobblerBitmapObserver& aObserver)
	{
	iObserver = &aObserver;
	}

#ifdef __SYMBIAN_SIGNED__
CFbsBitmap* CMobblerBitmap::Bitmap(TBool aOriginalPlease) const
#else
CFbsBitmap* CMobblerBitmap::Bitmap() const
#endif
	{
	CFbsBitmap* bitmap(NULL);
	
	if (iBitmapLoaded)
		{
#ifdef __SYMBIAN_SIGNED__
		aOriginalPlease && iOriginalBitmap ?
			bitmap = iOriginalBitmap:
			bitmap = iBitmap;
#else
		bitmap = iBitmap;
#endif
		}
	
	return bitmap;
	}

CFbsBitmap* CMobblerBitmap::BitmapGrayL() const
	{
	CFbsBitmap* bitmap(NULL);
	
	if (iBitmapLoaded)
		{
		if (!iBitmapGray)
			{
			iBitmapGray = new (ELeave) CFbsBitmap();
			TSize picSize(iBitmap->SizeInPixels());
			iBitmapGray->Create(picSize, EGray256);
			CFbsBitmap* src(const_cast<CFbsBitmap*>(iBitmap));
			CFbsBitmapDevice* device(CFbsBitmapDevice::NewL(iBitmapGray));
			CleanupStack::PushL(device);
			CFbsBitGc* gc(NULL);
			User::LeaveIfError(device->CreateContext(gc));
			CleanupStack::PushL(gc);
			gc->BitBlt(TPoint(0, 0), src);
			CleanupStack::PopAndDestroy(2);
			}
		
		bitmap = iBitmapGray;
		}
	
	return bitmap;
	}

CFbsBitmap* CMobblerBitmap::Mask() const
	{
	CFbsBitmap* mask(NULL);
	
	if (iBitmapLoaded)
		{
		mask = iMask;
		}
	
	return mask;
	}

void CMobblerBitmap::SetSize(TSize aSize)
	{
	if (this)
		{
		AknIconUtils::SetSize(iBitmap, aSize);
		delete iBitmapGray;
		iBitmapGray = NULL;
		}
	}

TSize CMobblerBitmap::SizeInPixels() const
	{
	TSize returnSize(0, 0);
	
	if (this && iBitmap)
		{
		returnSize = iBitmap->SizeInPixels();
		}
	
	return returnSize;
	}
	
void CMobblerBitmap::ConstructL(const TDesC& aFileName, const TUid aFileUid)
	{
	TFileName fileName;
	if (aFileName[0] == '\\')
		{
		// Find the drive the Mobbler is installed on so that we know where the graphic file is
		TParse parse;
		TFileName appFullName(RProcess().FileName());
		parse.Set(appFullName, NULL, NULL);
		fileName.Copy(parse.Drive());
		fileName.Append(aFileName);
		}
	else
		{
		fileName.Copy(aFileName);
		}

	iImageDecoder = CImageDecoder::FileNewL(CCoeEnv::Static()->FsSession(), fileName, CImageDecoder::EOptionNone, aFileUid, TUid::Null(), TUid::Null());
	const TFrameInfo& info(iImageDecoder->FrameInfo());
	iBitmap = new(ELeave) CFbsBitmap();

	iBitmap->Create(info.iOverallSizeInPixels, info.iFrameDisplayMode);
	
	TRequestStatus* status(&iStatus);
	if (info.iFlags & TFrameInfo::ETransparencyPossible)
		{
		iMask = new(ELeave) CFbsBitmap();
		iMask->Create(info.iOverallSizeInPixels, EGray256);
		iImageDecoder->Convert(status, *iBitmap, *iMask);
		}
	else
		{
		iImageDecoder->Convert(status, *iBitmap);
		}

	SetActive();
	}

void CMobblerBitmap::ConstructL(const TDesC8& aData, const TUid aFileUid)
	{
	iData = aData.AllocL();
	
	iImageDecoder = CImageDecoder::DataNewL(CCoeEnv::Static()->FsSession(), *iData, CImageDecoder::EOptionAlwaysThread, aFileUid, TUid::Null(), TUid::Null());
	const TFrameInfo& info(iImageDecoder->FrameInfo());
	iBitmap = new(ELeave) CFbsBitmap();
	iBitmap->Create(info.iOverallSizeInPixels, info.iFrameDisplayMode);
	
	TRequestStatus* status(&iStatus);
	if (info.iFlags & TFrameInfo::ETransparencyPossible)
		{
		iMask = new(ELeave) CFbsBitmap();
		iMask->Create(info.iOverallSizeInPixels, EGray256);
		iImageDecoder->Convert(status, *iBitmap, *iMask);
		}
	else
		{
		iImageDecoder->Convert(status, *iBitmap);
		}

	SetActive();
	}

void CMobblerBitmap::ConstructL(TUid aAppUid)
	{
	CFbsBitmap* bitmap(NULL);
	CFbsBitmap* mask(NULL);
	AknsUtils::CreateAppIconLC(AknsUtils::SkinInstance(), aAppUid,  EAknsAppIconTypeContext, bitmap, mask);
	CleanupStack::Pop(2);
	iBitmap = bitmap;
	iMask = mask;
	iBitmapLoaded = ETrue;
	}
	
void CMobblerBitmap::ConstructL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt aMaskIndex)
	{
	// Find which drive the mif file is on
	iMifFileName = HBufC::NewL(aMifFileName.Length() + 2);
	TParse parse;
	TFileName appFullName(RProcess().FileName());
	parse.Set(appFullName, NULL, NULL);
	iMifFileName->Des().Copy(parse.Drive());
	iMifFileName->Des().Append(aMifFileName);
	
	iMifBitmapIndex = aBitmapIndex;
	iMifMaskIndex = aMaskIndex;
	
	// asked to be called back to do the conversion
	TRequestStatus* status(&iStatus);
	User::RequestComplete(status, KErrNone);
	SetActive();
	}

void CMobblerBitmap::RunL()
	{
	if (!iBitmapScaler)
		{
		delete iImageDecoder;
		iImageDecoder = NULL;
	
		if (iStatus.Int() == KErrNone)
			{
			if (iMifFileName)
				{
				AknIconUtils::CreateIconL(iBitmap, iMask, *iMifFileName, iMifBitmapIndex, iMifMaskIndex);
				delete iMifFileName;
				iMifFileName = NULL;
				}
		
			iBitmapLoaded = ETrue;
			if (iObserver)
				{
				iObserver->BitmapLoadedL(this);
				}
			}
		}
	else
		{
		delete iBitmapScaler;
		iBitmapScaler = NULL;
		
		if (iStatus.Int() == KErrNone)
			{
			if (!iOriginalBitmap)
				{
				// Save a handle to the original bitmap
				iOriginalBitmap = iBitmap;
				}
			
			iBitmap = iScaledBitmap;
			iScaledBitmap = NULL;
			iScaleStatus = EMobblerScaleNone;

			if (iObserver)
				{
				iObserver->BitmapResizedL(this);
				}
			}
		}
	}

void CMobblerBitmap::DoCancel()
	{
	if (iImageDecoder)
		{
		iImageDecoder->Cancel();
		}
	
	if (iBitmapScaler)
		{
		iBitmapScaler->Cancel();
		}
	}

void CMobblerBitmap::ScaleL(TSize aSize)
	{
	if (iBitmapLoaded && iScaleStatus != EMobblerScalePending
			&& !LongSidesEqual(iBitmap->SizeInPixels(), aSize))
		{
		// Delete and stop any previous attempt to scale and create a new scaler
		Cancel();
		delete iBitmapScaler;
		iBitmapScaler = NULL;
		iBitmapScaler = CBitmapScaler::NewL();
		iBitmapScaler->SetQualityAlgorithm(CBitmapScaler::EMaximumQuality);
		
		// Create the new bitmap at the correct size and start scaling
		iScaledBitmap = new(ELeave) CFbsBitmap();
		iScaledBitmap->Create(aSize, EColor16M);
		
		if (iOriginalBitmap)
			{
			iBitmapScaler->Scale(&iStatus, *iOriginalBitmap, *iScaledBitmap, ETrue);
			}
		else
			{
			iBitmapScaler->Scale(&iStatus, *iBitmap, *iScaledBitmap, ETrue);
			}
		SetActive();
		
		iScaleStatus = EMobblerScalePending;
		}
	}

CMobblerBitmap::TMobblerScaleStatus CMobblerBitmap::ScaleStatus() const
	{
	return iScaleStatus;
	}

TBool CMobblerBitmap::LongSidesEqual(TSize aLeftSize, TSize aRightSize)
	{
	TInt width(aLeftSize.iWidth);
	TInt height(aLeftSize.iHeight);
	
	if ((width >= height && width == aRightSize.iWidth)
			||
		(height >= width && height == aRightSize.iHeight))
		{
		return ETrue;
		}

	return EFalse;
	}
	
// End of file
