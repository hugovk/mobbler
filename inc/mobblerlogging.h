/*
mobblerlogging.h

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

#ifndef __MOBBLERLOGGING_H__
#define __MOBBLERLOGGING_H__

#include <e32base.h>

class CMobblerString;

class CMobblerLogging : public CBase
	{
public:

#ifdef _DEBUG

#define DUMPDATA(a, b) CMobblerLogging::DumpDataL(a, b)
#define LOG(a) CMobblerLogging::LogL(a)
#define LOG2(a, b) CMobblerLogging::LogL(a, b)
#define LOGTEXT(a) CMobblerLogging::LogL(_L8(a))
#define LOGTEXT2(a, b) CMobblerLogging::LogL(_L8(a), _L8(b))

	static void DumpDataL(const TDesC8& aData, const TDesC& aLogFile);
	static void LogL(const TInt aFirstNumber, const TInt aSecondNumber);
	static void LogL(const TInt aNumber);
	static void LogL(const TDesC& aText);
	static void LogL(const TDesC8& aText, const TInt aNumber);
	static void LogL(const TDesC8& aFirstText, const TDesC8& aSecondText);
	static void LogL(const TDesC8& aFirstText, const TDesC& aSecondText);
	static void LogL(const CMobblerString* aMobblerString);
	static void LogL(const TDesC8& aText);

#else // _DEBUG

#define DUMPDATA(a, b)
#define LOG(a)
#define LOG2(a, b)
#define LOGTEXT(a)
#define LOGTEXT2(a, b)

#endif // _DEBUG
	};

#endif // __MOBBLERLOGGING_H__

// End of file
