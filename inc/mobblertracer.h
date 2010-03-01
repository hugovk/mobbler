/*
mobblertracer.h

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2006  Marko Kivijärvi

Originally from:
http://wiki.forum.nokia.com/index.php/Trace_Function_Enter,_Exit_and_Leave

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

#ifndef __MOBBLERTRACER_H__
#define __MOBBLERTRACER_H__

#include <e32base.h>

// Define tracer logging method
// 0    = Logging off
// 1    = Log to RDebug
// 2    = Log to file (RFileLogger)
#define TRACER_LOG_METHOD 0

// ============================================================================

// Logging off, define empty macros and skip all the rest
#if (TRACER_LOG_METHOD == 0) || !defined(_DEBUG)

    #define TRACER(func)
    #define TRACER_RET(func,format)
    #define TRACER_AUTO
    #define TRACER_AUTO_RET

#else                       // Logging on

    // Macro to print function entry, exit and leave.
    // Example: TRACER("CMyClass::MyFunction");
    #define TRACER(func) TTracer function_tracer( _S(func), _S("") );

    // Macro to print function return value in addition to entry, exit
    // and leave conditions Second parameter is a formatting string used
    // to print the return value Example to print an integer return value:
    // TRACER_RET("CMyclass::MyFunction", "%d");
    #define TRACER_RET(func,format) TTracer func_tracer( _S(func), _S(format) );

    // Macro to print function entry, exit and leave.
    // Gets the function name automatically
    // Example: TRACER_AUTO;
    // Substract 1 from MaxLength() because PtrZ() adds the zero terminator
    #define TRACER_AUTO \
        TPtrC8 __ptr8((const TUint8*)__PRETTY_FUNCTION__); \
        TBuf<150> __buf; \
        __buf.Copy( __ptr8.Left( __buf.MaxLength() - 1 ) ); \
        TTracer function_tracer( __buf.PtrZ(), _S("") )

    // Macro to print function entry, exit and leave.
    // Gets the function name automatically
    // Example: TRACER_AUTO_RET("%d");
    // Substract 1 from MaxLength() because PtrZ() adds the zero terminator
    #define TRACER_AUTO_RET(format) \
        TPtrC8 __ptr8((const TUint8*)__PRETTY_FUNCTION__); \
        TBuf<150> __buf; \
        __buf.Copy( __ptr8.Left( __buf.MaxLength() - 1 ) ); \
        TTracer function_tracer( __buf.PtrZ(), _S(format) )

    #if TRACER_LOG_METHOD == 1      // Print to RDebug

        #include <e32debug.h>
        #define TRACER_PRINT(a)         RDebug::Print(a,&iFunc);
        #define TRACER_PRINT_RET(a,b)   RDebug::Print(a,&iFunc,b);

    #elif TRACER_LOG_METHOD == 2    // Print to file

        #include <flogger.h>
        _LIT( KLogDir,  "mobbler" );     // Log directory: C:\logs\mobbler
        _LIT( KLogFile, "mobbler.txt" ); // Log file: c:\logs\mobbler\mobbler.txt
        #define TRACER_PRINT(a)         RFileLogger::WriteFormat(KLogDir, \
                            KLogFile,EFileLoggingModeAppend,a,&iFunc);
        #define TRACER_PRINT_RET(a,b)   RFileLogger::WriteFormat(KLogDir, \
                            KLogFile,EFileLoggingModeAppend,a,&iFunc,b);

    #endif

    _LIT( KLogEnter,    "%S: ENTER" );
    _LIT( KLogExit,     "%S: EXIT" );
    _LIT( KLogLeave,    "%S: LEAVE!" );
    _LIT( KLogExitRet,  "%S: EXIT, Returning " );

    /**
     * Simple tracer class that logs function enter, exit, or leave
     */
    class TTracer
        {
    public:

        /**
         * inline constructor to write log of entering a function
         */
        TTracer( const TText* aFunc, const TText* aRetFormat )
            : iFunc( aFunc )
            , iRetFormat( aRetFormat )
            {
            TRACER_PRINT( KLogEnter );
            }

        /**
         * inline destructor to write log of exiting a function
         * normally or with a leave
         */
        ~TTracer()
            {
            if ( std::uncaught_exception() ) // Leave is an exception
                {
                // The function exited with a leave
                TRACER_PRINT( KLogLeave );
                }
            else
                {
                // The function exited normally
                if ( iRetFormat.Length() == 0 )
                    {
                    TRACER_PRINT( KLogExit );
                    }
                else
                    {
                    // Log the return value
                    #ifdef __WINS__
                        TInt32 retVal = 0;

                        // The assembly bit. This needs to be reimplemented
                        // for every target.
                        _asm( mov retVal, ebx );

                        TBuf<100> format( KLogExitRet );
                        format.Append( iRetFormat );
                        TRACER_PRINT_RET( format, retVal );
                    #else
                        TRACER_PRINT( KLogExit );
                    #endif
                    }
                }
            }

    private:

        /**
         * Pointer descriptor to function signature that is to be logged.
         */
        TPtrC iFunc;

        /**
         * Formatting string used to print the function return value
         */
        TPtrC iRetFormat;

        };

#endif // TRACER_LOG_METHOD == 0

#endif // __MOBBLERTRACER_H__

// End of file
