/*
mobblertracklist.h

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

#ifndef __MOBBLERTRACKLIST_H__
#define __MOBBLERTRACKLIST_H__

#include "mobblerdataobserver.h"
#include "mobblerlistcontrol.h"

class CMobblerAppUi;
class CMobblerWebServicesHelper;

class CMobblerTrackList : public CMobblerListControl, public MMobblerFlatDataObserverHelper
	{
public:
	CMobblerTrackList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl);
	~CMobblerTrackList();
	
	void ConstructL();
	
	CMobblerListControl* HandleListCommandL(TInt aCommand);
	void SupportedCommandsL(RArray<TInt>& aCommands);
	void ParseL(const TDesC8& aXML);
	
private:
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);
	
	static TInt ViewScrobbleLogCallBackL(TAny* aPtr);
	
	void GetArtistAndTitleName(TPtrC8& aArtist, TPtrC8& aTitle);
	
private:
	CMobblerWebServicesHelper* iWebServicesHelper;
	CMobblerFlatDataObserverHelper* iAlbumInfoObserver;
	CMobblerFlatDataObserverHelper* iLoveObserver;
	CMobblerFlatDataObserverHelper* iLyricsObserver;
	
	// callback for viewing scrobble log
	CAsyncCallBack* iAsyncCallBack;
	TCallBack iCallBack;
	};

#endif // __MOBBLERTRACKLIST_H__

// End of file
