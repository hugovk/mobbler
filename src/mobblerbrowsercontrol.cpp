/*
mobblerbrowsercontrol.cpp

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

#include <brctlinterface.h>
#include <charconv.h>

#include "mobblerappui.h"
#include "mobblerbrowsercontrol.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

/*
 * Format string specifiers are:
 *
 * %S - artist name
 * %S - image URL
 * %d - image width
 * %S - tags
 * %S - similar artists
 * %S - biography text
 */
_LIT8(KArtistInfoHtmlTemplate, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\
	<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\
	<html xmlns=\"http://www.w3.org/1999/xhtml\">\
\
	<head>\
		<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\
		<title></title>\
		<style type=\"text/css\">\
			body, html {\
				margin: 0px 0px 2px 0px;\
				padding: 0px;\
				font-family: Arial, Helvetica, sans-serif;\
				font-size: medium;\
				color: #333333;\
				background: url(data:image/gif;base64,\
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)\
no-repeat fixed 5%% 20%%; \
				\
			}\
\
			#nav_bar {\
				width: 100%%;\
				text-align: center;\
				background-color: #D51007;\
				color: #ffffff;\
				padding: 1px;\
			}\
\
			#info_bar {\
				width: 100%%;\
				margin: 2px;\
			}\
\
			#scrobbled_tracks {\
				float: left;\
				width: 35%%;\
			}\
\
			#artist_image {\
				float:right;\
				width: auto;\
				margin: 2px;\
			}\
\
			#body_text {\
				width: 100%%;\
				float: left;\
				margin: 2px;\
			}\
\
		</style>\
	</head>\
	<body>\
		<div id=\"nav_bar\"><h3>%S</h3></div>\
		<div id=\"info_bar\">\
		<div id=\"artist_image\"><img src=\"%S\" width=\"%d\"  /></div>\
		<a name=\"#tags\"><p><strong>Tags: </strong>%S</p></a>\
		<a name=\"#similar_artists\"><p><strong>Similar Artists: </strong>%S</p></a>\
		</div>\
		<div id=\"body_text\">\
		<p>%S</p>\
\
		</div><!-- body_text -->\
	</body>\
</html>");

_LIT8(KHtmlDataType, "text/html");

CMobblerBrowserControl* CMobblerBrowserControl::NewL(const TRect& aRect, CMobblerAppUi& aMobblerAppUi, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	CMobblerBrowserControl* self(new(ELeave) CMobblerBrowserControl(aMobblerAppUi));
	CleanupStack::PushL(self);
	self->ConstructL(aRect, aCustomMessageId, aCustomMessage);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBrowserControl::CMobblerBrowserControl(CMobblerAppUi& aMobblerAppUi) :
	iAppUi(aMobblerAppUi), iBrowserLoadObserver(iAppUi.LastFmConnection())
	{
	}

void CMobblerBrowserControl::ConstructL(const TRect& aRect, TUid /*aCustomMessageId*/, const TDesC8& aCustomMessage)
	{
	CreateWindowL();
	SetRect(aRect);

	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		iBrCtlInterface = NULL;
		}

	// TODO: need to detect 3rd Ed and use
	// TRect brCtlRect(Position(), Size());   // in 3rd Ed devices
	// need a device to test on first
	TRect brCtlRect(TPoint(0, 0), Size()); // in 3rd Ed, FP1 devices

	iBrCtlInterface = ::CreateBrowserControlL(
			this,
			brCtlRect,
			TBrCtlDefs::ECapabilityDisplayScrollBar,
			TBrCtlDefs::ECommandIdBase,
			NULL,
			NULL,
			&iBrowserLoadObserver);

	_LIT(KHttpDummy, "http://dummy");
	HBufC8* tagsText(NULL);
	HBufC8* similarArtistsText(NULL);
	HBufC8* imageUrl(NULL);
	HBufC8* artistInfo(NULL);

	CMobblerParser::ParseArtistInfoL(aCustomMessage, artistInfo, imageUrl, tagsText, similarArtistsText);

	CleanupStack::PushL(tagsText);
	CleanupStack::PushL(imageUrl);
	CleanupStack::PushL(artistInfo);
	CleanupStack::PushL(similarArtistsText);

	// Decide how big the artist picture should be taking into account the width
	// of the application
	TRect applicationRect(iAppUi.ApplicationRect());
	TInt artistImageWidth((TInt)((TReal)applicationRect.Width() * 0.40));

	HBufC8* artistInfoHtml(HBufC8::NewLC(KArtistInfoHtmlTemplate().Length() +
			iAppUi.CurrentTrack()->Artist().String8().Length() +
			tagsText->Length() +
			similarArtistsText->Length() +
			imageUrl->Length() +
			3 +
			artistInfo->Length()));

	TPtr8 artistHtmlPtr(artistInfoHtml->Des());
	artistHtmlPtr.AppendFormat(KArtistInfoHtmlTemplate,
			&(iAppUi.CurrentTrack()->Artist().String8()),
			imageUrl,
			artistImageWidth,
			tagsText,
			similarArtistsText,
			artistInfo);
	DUMPDATA(artistHtmlPtr, _L("artistbio.txt"));

	TDataType dataType(KHtmlDataType());
	TUid uid;
	uid.iUid = KCharacterSetIdentifierUtf8;


	iBrCtlInterface->LoadDataL(KHttpDummy, artistHtmlPtr, dataType, uid);

	CleanupStack::PopAndDestroy(5, tagsText);
	}

CMobblerBrowserControl::~CMobblerBrowserControl()
	{
	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		}
	}

void CMobblerBrowserControl::SizeChanged()
	{
	if (iBrCtlInterface)
		{
		iBrCtlInterface->SetRect(Rect());
		}
	}

TInt CMobblerBrowserControl::CountComponentControls() const
	{
	return 1;
	}

CCoeControl* CMobblerBrowserControl::ComponentControl(TInt aIndex) const
	{
	switch (aIndex)
		{
		case 0:
			return iBrCtlInterface;
		default:
			return NULL;
		}
	}

void CMobblerBrowserControl::HandleResourceChange(TInt aType)
	{
	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		TRect rect;
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);
		SetRect(rect);
		iAppUi.StatusPane()->MakeVisible(ETrue);
		}

	CCoeControl::HandleResourceChange(aType);
	}

void CMobblerBrowserControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc(SystemGc());
	gc.Clear(Rect());
	}

TKeyResponse CMobblerBrowserControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	// This is a hack because the CBrCtlInterface which is part of the
	// platform makes both the left and right arrow keys scroll the page
	// up which is counter-intuitive. So change right to down before passing
	// the key event to the control.
	TKeyEvent newKeyEvent(aKeyEvent);
	if (aKeyEvent.iCode == EKeyRightArrow)
		{
		newKeyEvent.iCode = EKeyDownArrow;
		}

	return iBrCtlInterface->OfferKeyEventL(newKeyEvent, aEventCode);
	}

// End of file
