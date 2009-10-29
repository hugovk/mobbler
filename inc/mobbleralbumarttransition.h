/*
mobbleralbumarttransition.h

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

#ifndef __MOBBLERALBUMARTTRANSITION_H__
#define __MOBBLERALBUMARTTRANSITION_H__

#include <e32base.h>

class CFbsBitmap;
class CGulIcon;
class CMobblerBitmap;
class CMobblerStatusControl;
class CPeriodic;

class CMobblerAlbumArtTransition : public CBase
	{
public:
	enum TSlide
		{
		ESlideNormal,
		ESlideLeft,
		ESlideRight
		};

public:
	static CMobblerAlbumArtTransition* NewL(CMobblerStatusControl& aStatusControl);
	~CMobblerAlbumArtTransition();
	
	void DrawAlbumArtL(const CMobblerBitmap* aAlbumArt, const CMobblerBitmap* aNextAlbumArt, TRect aAlbumArtSize, TInt aSlideAmount);
	
	void FingerUpL(TInt aPosition, TSlide aSlide);
	
private:
	CMobblerAlbumArtTransition(CMobblerStatusControl& aStatusControl);
	void ConstructL();
	
	void DoDrawAlbumArtL(const CMobblerBitmap* aLeft, const CMobblerBitmap* aRight, TRect aAlbumArtRect, TInt aPosition);
	
	TInt SlideAmount(TInt aWidth) const;
	TInt Clamp(TInt aValue, TInt aMin, TInt aMax) const;
	TInt Slide(TInt aTime, TInt aTotal, TInt aStart, TInt aEnd) const;
	
private:
	static TInt CallBack(TAny* aRef);
	
	static TRect ScaledRectL(TRect aDestRect, TRect aSourceRect);
	
private:
	CMobblerStatusControl& iStatusControl;
	
	RPointerArray<CMobblerBitmap> iAlbumArt;
	
	CPeriodic* iTimer;
	TUint iStartTickCount;
	TUint iNowTickCount;
	
	TInt iFingerUpOffset;
	TSlide iSlide;
	};

#endif // __MOBBLERALBUMARTTRANSITION_H__

// End of file
