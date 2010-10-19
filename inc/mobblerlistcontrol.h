/*
mobblerlistcontrol.h

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

#ifndef __MOBBLERLISTCONTROL_H__
#define __MOBBLERLISTCONTROL_H__

#include <aknlists.h>

#include "mobblerbitmap.h"
#include "mobblerdataobserver.h"

class CMobblerAppUi;
class CMobblerListItem;
class CMobblerString;
class CMobblerWebServicesControl;

class CMobblerListControl : public CCoeControl,
							public MMobblerBitmapObserver,
							public MMobblerFlatDataObserver,
							public MEikScrollBarObserver,
							public MEikListBoxObserver
	{
public:
	enum TState
		{
		ELoading,
		ENormal,
		EFailed
		};
	
public:
	static CMobblerListControl* CreateListL(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl, TInt aType, const TDesC8& aText1, const TDesC8& aText2);
	
	~CMobblerListControl();
	
	void HandleLoadedL();
	TState State() const;
	
	TInt Count() const;
	
	TInt Type() const;
	HBufC* NameL() const;
	
	void SetWebServicesControl();
	
public: // interface for derived lists
	virtual CMobblerListControl* HandleListCommandL(TInt aCommand) = 0;
	virtual void SupportedCommandsL(RArray<TInt>& aCommands) = 0;
	virtual TBool ParseL(const TDesC8& aData) = 0;
	
protected:
	CMobblerListControl(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl);
	
	void ConstructListL(TInt aType, const TDesC8& aText1, const TDesC8& aText2);
	virtual void ConstructL() = 0;
	
	void MakeListBoxL();
	
	void UpdateIconArrayL();
	
	virtual void RequestImageL(TInt aIndex) const;
	
private: // from CCoeControl
	void Draw(const TRect& aRect) const;
	CCoeControl* ComponentControl(TInt /*aIndex*/) const;
	TInt CountComponentControls() const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode);
	void HandleResourceChange(TInt aType);
	void SizeChanged();
	
private: // from MMobblerBitmapObserver
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
	void RequestImagesL() const;
	
protected:
	void DataL(const TDesC8& aData, TInt aTransactionError);
	
private:
	void HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType);
	void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType);
	
	
	
protected:
	CMobblerAppUi& iAppUi;
	CMobblerWebServicesControl& iWebServicesControl;
	
	CAknDoubleLargeStyleListBox* iListBox;
	CDesCArrayFlat* iListBoxItems;
	
	RPointerArray<CMobblerListItem> iList;
	
	CMobblerBitmap* iDefaultImage;
	
	TState iState;
	
	TInt iType;
	CMobblerString* iText1;
	CMobblerString* iText2;
	};

#endif // __MOBBLERLISTCONTROL_H__

// End of file
