/*
mobblerdocument.cpp

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

#include "mobblerappui.h"
#include "mobblerdocument.h"
#include "mobblertracer.h"

CMobblerDocument* CMobblerDocument::NewL(CEikApplication& aApp)
	{
    TRACER_AUTO;
	CMobblerDocument* self(NewLC(aApp));
	CleanupStack::Pop(self);
	return self;
	}

CMobblerDocument* CMobblerDocument::NewLC(CEikApplication& aApp)
	{
    TRACER_AUTO;
	CMobblerDocument* self(new(ELeave) CMobblerDocument(aApp));
	CleanupStack::PushL(self);
	self->ConstructL();
	
	return self;
	}

void CMobblerDocument::ConstructL()
	{
    TRACER_AUTO;
	}

CMobblerDocument::CMobblerDocument(CEikApplication& aApp)
	:CAknDocument(aApp)
	{
    TRACER_AUTO;
	}

CMobblerDocument::~CMobblerDocument()
	{
    TRACER_AUTO;
	}

CEikAppUi* CMobblerDocument::CreateAppUiL()
	{
    TRACER_AUTO;
	return new(ELeave) CMobblerAppUi;
	}

// End of file
