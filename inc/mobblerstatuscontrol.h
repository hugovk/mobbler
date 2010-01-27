/*
mobblerstatuscontrol.h

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

#ifndef __MOBBLERSTATUSCONTROL_H__
#define __MOBBLERSTATUSCONTROL_H__

#include "mobblerbitmap.h" 
#include "mobblermusiclistener.h"
#include "mobblerradioplayer.h" 

class CAknNavigationControlContainer;
class CAknNavigationDecorator;
class CAknsBasicBackgroundControlContext;
class CBitmapScaler;
class CMobblerAlbumArtTransition;
class CMobblerAppUi;
class CMobblerBitmap;
class CMobblerMarquee;
class CMobblerTimeout;
class CMobblerTouchFeedbackInterface;
class CMobblerTrack;

class CMobblerStatusControl : public CCoeControl,
								public MMobblerBitmapObserver,
								public MMobblerRadioStateChangeObserver,
								public MMobblerConnectionStateObserver,
								public MMobblerMusicAppListenerObserver
	{
public:
	friend class CMobblerAlbumArtTransition;
	
public:
	static CMobblerStatusControl* NewL(const TRect& aRect, const CMobblerAppUi& aAppUi);
	~CMobblerStatusControl();
	
	void VolumeChanged();
	TBool IsFifthEdition() const { return (iMobblerFeedback != NULL); }
	
private:
	void ConstructL(const TRect& aRect);
	CMobblerStatusControl(const CMobblerAppUi& aAppUi);
	
	void LoadGraphicsL();
	
	void SetPositions();
	
	// double buffering
	void CreateBackBufferL();
	void ReleaseBackBuffer();
	
	// callback from CMobblerBitmap when graphic is loaded
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
	// Drawing helper methods
	void DrawMobblerBitmapL(const CMobblerBitmap* aMobblerBitmap, const TRect& aRect, const TRect& aSourceRect, TBool aGray = EFalse) const;
	void BitBltMobblerBitmapL(const CMobblerBitmap* aMobblerBitmap, const TPoint& aPoint, const TRect& aSourceRect, TBool aGray = EFalse) const;
	void DrawText(const TDesC& aText, const TRect& aRect, const TRgb& aPenColor, CGraphicsContext::TTextAlign aTextAlign, TInt aOffset) const;
	void DrawRect(const TRect& aRect, const TRgb& aPenColor, const TRgb& aBrushColor) const;
	
	// formatting text
	static void FormatTime(TDes& aString, TTimeIntervalSeconds aSeconds, TTimeIntervalSeconds aTotalSeconds = 0);
	void DoChangePaneTextL();
	
private: // from CCoeControl
	void HandleResourceChange(TInt aType);
	TTypeUid::Ptr MopSupplyObject(TTypeUid aId);
	void SizeChanged();
	void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	
private: // state change observers
	void HandleConnectionStateChangedL();
	void HandleRadioStateChangedL();
	void HandleMusicAppChangeL();
	
private:
	const CMobblerAppUi& iAppUi;
	CAknsBasicBackgroundControlContext* iBgContext;
	
	CAknNavigationControlContainer *iNaviContainer;
	CAknNavigationDecorator* iNaviLabelDecorator;
	
	// Graphics
	CMobblerBitmap* iMobblerBitmapBan;
	CMobblerBitmap* iMobblerBitmapMore;
	CMobblerBitmap* iMobblerBitmapLove;
	CMobblerBitmap* iMobblerBitmapPlay;
	CMobblerBitmap* iMobblerBitmapNext;
	CMobblerBitmap* iMobblerBitmapStop;
	CMobblerBitmap* iMobblerBitmapLastFm;
	CMobblerBitmap* iMobblerBitmapSpeakerLow;
	CMobblerBitmap* iMobblerBitmapSpeakerHigh;
	CMobblerBitmap* iMobblerBitmapScrobble;
	CMobblerBitmap* iMobblerBitmapTrackIcon;
	CMobblerBitmap* iMobblerBitmapAlarmIcon;
	CMobblerBitmap* iMobblerBitmapHarddiskIcon;
	CMobblerBitmap* iMobblerBitmapOnTour;
	CMobblerBitmap* iMobblerBitmapMusicAppIcon;
	CMobblerBitmap* iMobblerBitmapAppIcon;
	
	// for double buffering
	CFbsBitmap*						iBackBuffer;
	CFbsBitmapDevice*				iBackBufferDevice;
	CFbsBitGc*						iBackBufferContext;
	TSize							iBackBufferSize;

	// text displayed
	mutable TBuf<KMaxMobblerTextSize> iScrobbledQueued;
	mutable TBuf<KMaxMobblerTextSize> iTitleText;
	mutable TBuf<KMaxMobblerTextSize> iAlbumText;
	mutable TBuf<KMaxMobblerTextSize> iArtistText;
	mutable TBuf<KMaxMobblerTextSize> iPlaybackLeftText;
	mutable TBuf<KMaxMobblerTextSize> iPlaybackGoneText;
	
	// graphics and text positions
	TRect iRectAlbumArt;
	TBool iShowAlbumArtFullscreen;
	
	TPoint iPointLastFm;
	TPoint iPointMore;
	TPoint iPointLove;
	TPoint iPointBan;
	TPoint iPointSkip;
	TPoint iPointPlayStop;
	TPoint iPointOnTour;
	TSize iControlSize;
	
	TRect iRectTitleText;
	TRect iRectArtistText;
	TRect iRectAlbumText;
	TRect iRectScrobbledQueuedText;
	TRect iRectProgressBar;
	
	CFbsFont* iMobblerFont;
	
	CMobblerTimeout* iMobblerVolumeTimeout;
	
	CMobblerMarquee* iTitleMarquee;
	CMobblerMarquee* iArtistMarquee;
	CMobblerMarquee* iAlbumMarquee;
	
	TPointerEvent iLastPointerEvent;
	
	CMobblerTouchFeedbackInterface* iMobblerFeedback;
	TUid iDtorIdKey;
	
	CMobblerAlbumArtTransition* iAlbumArtTransition;
	
	TPoint iFingerDownPosition;
	TPoint iFingerNowPosition;
	};

#endif // __MOBBLERSTATUSCONTROL_H__

// End of file
