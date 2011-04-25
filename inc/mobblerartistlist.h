/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade

http://code.google.com/p/mobbler

This file is part of Mobbler.

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MOBBLERARTISTLIST_H__
#define __MOBBLERARTISTLIST_H__

#include "mobblerdataobserver.h"
#include "mobblerlistcontrol.h"

class CMobblerAppUi;

class CMobblerArtistList : public CMobblerListControl, public MMobblerFlatDataObserverHelper
	{
public:
	CMobblerArtistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl);
	~CMobblerArtistList();
	
	void ConstructL();
	
	CMobblerListControl* HandleListCommandL(TInt aCommand);
	void SupportedCommandsL(RArray<TInt>& aCommands);
	TBool ParseL(const TDesC8& aXml);

private:
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError);
	
private:
	CMobblerWebServicesHelper* iWebServicesHelper;
	CMobblerFlatDataObserverHelper* iArtistBiographyObserver;
	};

#endif // __MOBBLERARTISTLIST_H__

// End of file
