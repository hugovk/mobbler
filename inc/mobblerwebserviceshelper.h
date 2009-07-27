/*
mobblerwebserviceshelper.h

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

#ifndef __MOBBLERWEBSERVICESHELPER_H__
#define __MOBBLERWEBSERVICESHELPER_H__

#include <e32base.h>

#include "mobblerdataobserver.h"

class CMobblerAppUi;
class CMobblerLastFMConnection;
class CMobblerTrack;

class CMobblerWebServicesHelper : public CBase,	public MMobblerFlatDataObserverHelper
	{
public:
	static CMobblerWebServicesHelper* NewL(CMobblerAppUi& aAppUi);
	~CMobblerWebServicesHelper();
	
	void TrackShareL(CMobblerTrack& aTrack);
	void ArtistShareL(CMobblerTrack& aTrack);
	void PlaylistAddL(CMobblerTrack& aTrack);
	
	void EventShareL(const TDesC8& aEventId);
	
private:
	void ConstructL();
	CMobblerWebServicesHelper(CMobblerAppUi& aAppUi);

private:
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFMConnection::TError aError);
	
private:
	CMobblerAppUi& iAppUi;
	
	CMobblerTrack* iTrack;
	HBufC8* iEventId;
	
	CMobblerFlatDataObserverHelper* iFriendFetchObserverHelperTrackShare;
	CMobblerFlatDataObserverHelper* iFriendFetchObserverHelperArtistShare;
	CMobblerFlatDataObserverHelper* iFriendFetchObserverHelperEventShare;
	CMobblerFlatDataObserverHelper* iShareObserverHelper;
	CMobblerFlatDataObserverHelper* iPlaylistAddObserverHelper;
	CMobblerFlatDataObserverHelper* iPlaylistFetchObserverHelper;
	};

#endif // __MOBBLERWEBSERVICESHELPER_H__

// End of file
