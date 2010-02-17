/*
mobblerbrowsercontrolspecialloadobserver.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2010 Michael Coffey

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

#include "mobblerbrowsercontrolspecialloadobserver.h"

#include "mobblerlastfmconnection.h"

TBrowserLoadObserver::TBrowserLoadObserver(CMobblerLastFmConnection& aLastFmConnection)
	: iLastFmConnection(aLastFmConnection)
	{
	}

/*
 * Make the browser control reuse the original connection.
 * aConnectionPtr needs to be a RConnection*
 * aSockSvrHandle needs to be a RSocketServ::Handle()*
 */
void TBrowserLoadObserver::NetworkConnectionNeededL(TInt* aConnectionPtr,
	                                              TInt* aSockSvrHandle,
	                                              TBool* aNewConn,
	                                              TApBearerType* aBearerType)
	{
	*aNewConn = EFalse;
	*aBearerType = EApBearerTypeAllBearers;

	*aConnectionPtr = reinterpret_cast<TInt>(&iLastFmConnection.Connection());
	*aSockSvrHandle = iLastFmConnection.SocketServ().Handle();

	__ASSERT_DEBUG(aConnectionPtr, User::Invariant());
	__ASSERT_DEBUG(aSockSvrHandle, User::Invariant());
	}

TBool TBrowserLoadObserver::HandleRequestL(RArray<TUint>* /*aTypeArray*/, CDesCArrayFlat* /*aDesArray*/)
	{
	return EFalse;
	}

TBool TBrowserLoadObserver::HandleDownloadL(RArray<TUint>* /*aTypeArray*/, CDesCArrayFlat* /*aDesArray*/)
	{
	return EFalse;
	}



