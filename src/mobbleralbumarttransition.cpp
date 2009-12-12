/*
mobbleralbumarttransition.cpp

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

#include <e32math.h>
#include <hal.h>

#include "mobbleralbumarttransition.h"
#include "mobblerappui.h"
#include "mobblerbitmap.h"
#include "mobblerstatuscontrol.h"

const TTimeIntervalMicroSeconds32 KMobblerAlbumArtInterval(50000);
const TInt KTotalSlideTime(750000);

CMobblerAlbumArtTransition* CMobblerAlbumArtTransition::NewL(CMobblerStatusControl& aStatusControl)
	{
	CMobblerAlbumArtTransition* self(new(ELeave) CMobblerAlbumArtTransition(aStatusControl));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAlbumArtTransition::CMobblerAlbumArtTransition(CMobblerStatusControl& aStatusControl)
	:iStatusControl(aStatusControl)
	{
	}

void CMobblerAlbumArtTransition::ConstructL()
	{
	}

TInt CMobblerAlbumArtTransition::Clamp(TInt aValue, TInt aMin, TInt aMax) const
	{
	// Restrict value to the bounds of min and max.
	// If value is less than min, return min.
	// If value is more than max, return max.
	// Else return value.
	
	if (aValue < aMin)
		{
		return aMin;
		}
	else if (aValue > aMax)
		{
		return aMax;
		}
	else
		{
		return aValue;
		}
	}

TInt CMobblerAlbumArtTransition::Slide(TInt aTime, TInt aTotal, TInt aStart, TInt aEnd) const
	{
	TReal output;
	Math::Sin(output, ((Clamp(aTime, 0, aTotal) / static_cast<TReal>(aTotal)) * KPi) - (KPi / 2));
	return static_cast<TInt>(((output + 1) / 2) * (aEnd - aStart));
	}

TInt CMobblerAlbumArtTransition::SlideAmount(TInt aWidth) const
	{
	TInt tickPeriod;
	HAL::Get(HAL::ESystemTickPeriod, tickPeriod);
	TInt timeSoFar((iNowTickCount  - iStartTickCount) * tickPeriod);
	return Slide(timeSoFar, KTotalSlideTime, 0, aWidth);
	}

CMobblerAlbumArtTransition::~CMobblerAlbumArtTransition()
	{
	const TInt KAlbumArtCount(iAlbumArt.Count());
	for (TInt i(0); i < KAlbumArtCount; ++i)
		{
		iAlbumArt[i]->Close();
		}
	
	iAlbumArt.Reset();
	
	delete iTimer;
	}

void CMobblerAlbumArtTransition::DrawAlbumArtL(const CMobblerBitmap* aCurrentAlbumArt, 
											   const CMobblerBitmap* aNextAlbumArt, 
											   TRect aAlbumArtRect, TInt aSlideAmount)
	{
	// Try to put any new album art in the list
	
	if (iAlbumArt.Find(aCurrentAlbumArt) == KErrNotFound)
		{
		// The current album art has not been found in the list
		// add it to the list and make sure no one else deletes it until
		//  we are finished
		iAlbumArt.AppendL(aCurrentAlbumArt);
		aCurrentAlbumArt->Open();
		}
	
	if (!iTimer &&
			iAlbumArt.Count() > 1 && iAlbumArt[1]->Bitmap())
		{
		// We are not transitioning and there are at least
		// two album arts in the list and so start
		
		delete iTimer;
		iTimer = CPeriodic::NewL(CActive::EPriorityLow);
		TCallBack callBack(CallBack, this);
		iTimer->Start(KMobblerAlbumArtInterval, KMobblerAlbumArtInterval, callBack);
		
		// Record the starting tick count
		iStartTickCount = User::TickCount();
		}
	
	iNowTickCount = User::TickCount();
	
	if (!iTimer || iTimer && (SlideAmount(aAlbumArtRect.Width()) >= aAlbumArtRect.Width()))
		{
		if (iTimer)
			{
			iTimer->Cancel();
			delete iTimer;
			iTimer = NULL;
			
			iSlide = ESlideNormal;
			
			// Get rid of the last album art as we don't need it anymore
			iAlbumArt[0]->Close();
			iAlbumArt.Remove(0);
			}
		
		// The album art is not transitioning
		
		if (aNextAlbumArt && aSlideAmount > 0)
			{
			// The album art is being moved by a finger
			
			DoDrawAlbumArtL(aCurrentAlbumArt, aNextAlbumArt, aAlbumArtRect, Clamp(aSlideAmount, 0, aAlbumArtRect.Width()));
			}
		else
			{
			// It's not transitioning or being moved by a finger
			// so just display the current album art
			CMobblerBitmap::LongSidesEqual(aCurrentAlbumArt->Bitmap()->SizeInPixels(), aAlbumArtRect.Size())?
				iStatusControl.BitBltMobblerBitmapL(aCurrentAlbumArt, aAlbumArtRect.iTl, TRect(TPoint(0, 0), aCurrentAlbumArt->SizeInPixels())):
				iStatusControl.DrawMobblerBitmapL(aCurrentAlbumArt, aAlbumArtRect, aCurrentAlbumArt->SizeInPixels());
			}
		}
	else
		{
		// The album art is transitioning so draw it
		
		switch (iSlide)
			{
			case ESlideNormal:
				{
				DoDrawAlbumArtL(iAlbumArt[0], iAlbumArt[1], aAlbumArtRect, SlideAmount(aAlbumArtRect.Width()));
				}
				break;
			case ESlideLeft:
				{
				TInt slideAmount(SlideAmount(aAlbumArtRect.Width()));
				TInt actualSlideAmount(iFingerUpOffset + (slideAmount * (aAlbumArtRect.Width() - iFingerUpOffset)) / aAlbumArtRect.Width());
				DoDrawAlbumArtL(iAlbumArt[0], iAlbumArt[1], aAlbumArtRect, Clamp(actualSlideAmount, 0, aAlbumArtRect.Width()));
				}
				break;
			case ESlideRight:
				{
				TInt slideAmount(SlideAmount(aAlbumArtRect.Width()));
				TInt actualSlideAmount(iFingerUpOffset - (slideAmount * iFingerUpOffset) / aAlbumArtRect.Width());
				DoDrawAlbumArtL(aCurrentAlbumArt, aNextAlbumArt, aAlbumArtRect, Clamp(actualSlideAmount, 0, aAlbumArtRect.Width()));
				}
				break;
			}
		}
	}

/*TRect CMobblerAlbumArtTransition::ScaledRect(TRect aDestRect, TRect aSourceRect)
	{
	TRect destRect(aDestRect);
	TInt width(aSourceRect.Width());
	TInt height(aSourceRect.Height());
	
	if (width > height)
		{
		destRect.SetHeight((aDestRect.Height() * height) / width);
		}
	else if (height > width)
		{
		destRect.SetWidth((aDestRect.Width() * width) / height);
		}
	
	return destRect;
	}*/

void CMobblerAlbumArtTransition::DoDrawAlbumArtL(const CMobblerBitmap* aLeft, const CMobblerBitmap* aRight, TRect aAlbumArtRect, TInt aPosition)
	{
	// Draw the first bitmap by the slide amount
	if (CMobblerBitmap::LongSidesEqual(aLeft->Bitmap()->SizeInPixels(), aAlbumArtRect.Size()))
		{
		// The long sides are equal so the bitmap has been scaled
		
		TPoint destPoint(aAlbumArtRect.iTl);
		TRect sourceRect(TPoint(aPosition, 0), TSize(aAlbumArtRect.Width() - aPosition, aLeft->SizeInPixels().iHeight));
		
		iStatusControl.BitBltMobblerBitmapL(aLeft, destPoint, sourceRect);
		}
	else
		{
		// The bitmap has not been scaled
		
		TRect destRect(aAlbumArtRect.iTl, TSize(aAlbumArtRect.Width() - aPosition, aAlbumArtRect.Height()));
		
		TPoint sourceTopLeft((aPosition * aLeft->SizeInPixels().iWidth) / aAlbumArtRect.Width(), 0);
		TSize sourceSize((((aAlbumArtRect.Width() - aPosition) * aLeft->SizeInPixels().iWidth) / aAlbumArtRect.Width()), aLeft->SizeInPixels().iHeight);
		TRect sourceRect(sourceTopLeft, sourceSize);
		
		iStatusControl.DrawMobblerBitmapL(aLeft, destRect, sourceRect);
		}
	
	// Draw the second bitmap by the slide amount
	if (CMobblerBitmap::LongSidesEqual(aRight->Bitmap()->SizeInPixels(), aAlbumArtRect.Size()))
		{
		// The long sides are equal so the bitmap has been scaled
		
		iStatusControl.BitBltMobblerBitmapL(aRight, 
				TPoint(aAlbumArtRect.iTl.iX - aPosition + aAlbumArtRect.Width(), aAlbumArtRect.iTl.iY), 
				TRect(TPoint(0, 0), TSize(aPosition, aRight->SizeInPixels().iHeight)));
		}
	else
		{
		// The bitmap has not been scaled
		
		TRect destRect(aAlbumArtRect.iTl - TPoint(aPosition - aAlbumArtRect.Width(), 0), TSize(aPosition, aAlbumArtRect.Height()));
		TRect sourceRect(TPoint(0, 0), TSize(((aPosition * aRight->SizeInPixels().iWidth) / aAlbumArtRect.Width()), aRight->SizeInPixels().iHeight));
		
		iStatusControl.DrawMobblerBitmapL(aRight, destRect, sourceRect);
		}
	}

void CMobblerAlbumArtTransition::FingerUpL(TInt aPosition, TSlide aSlide)
	{
	iFingerUpOffset = aPosition;
	iSlide = aSlide;
	
	// Start the callbacks
	delete iTimer;
	iTimer = CPeriodic::NewL(CActive::EPriorityLow);
	TCallBack callBack(CallBack, this);
	iTimer->Start(KMobblerAlbumArtInterval, KMobblerAlbumArtInterval, callBack);
	
	// Record the starting tick count
	iStartTickCount = User::TickCount();
	}

TInt CMobblerAlbumArtTransition::CallBack(TAny* /*aRef*/)
	{
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawNow();
	return KErrNone;
	}

// End of file
