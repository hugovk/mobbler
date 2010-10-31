/*
mobblerdestinations.h

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

#ifndef __MOBBLERDESTINATIONS_H__
#define __MOBBLERDESTINATIONS_H__

#include <e32base.h>
#include <comms-infras/cs_mobility_apiext.h>
#include <cmmanager.h>

#include <mobbler\mobblerdestinationsinterface.h>

class CMobblerDestinations : public CMobblerDestinationsInterface, public MMobilityProtocolResp
	{
public:
	static CMobblerDestinations* NewL();
	~CMobblerDestinations();
	
	TInt LoadDestinationListL(CArrayPtr<CAknEnumeratedText>& aDestinations);
	void RegisterMobilityL(RConnection& aConnection, MMobblerDestinationsInterfaceObserver* aObserver);
	
private: // from MMobilityProtocolResp
	void PreferredCarrierAvailable(TAccessPointInfo aOldAPInfo, TAccessPointInfo aNewAPInfo, TBool aIsUpgrade, TBool aIsSeamless);
	void NewCarrierActive(TAccessPointInfo aNewAPInfo, TBool aIsSeamless);
	void Error(TInt aError);
	
private:
	CMobblerDestinations();
	void ConstructL();

private:
	RCmManager iCmManager;
	
	CActiveCommsMobilityApiExt* iMobility;
	MMobblerDestinationsInterfaceObserver* iMobilityObserver;
	};

#endif // __MOBBLERDESTINATIONS_H__

// End of file
