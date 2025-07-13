/*
===========================================================================

Core

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#pragma once

#include "dll_export.h"
#include "core_types.h"

// TODO(DM): the only reason this exists is because we call IsDebuggerPresent() and __debugbreak() in this file
// that probably wants to be moved into a .inl in that case
#if defined( _WIN32 )
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#elif defined( __linux__ )
	#include<signal.h>
#endif

#ifdef _WIN32
#define ERROR_CODE_FORMAT "0x%X"
typedef DWORD errorCode_t;
#elif defined(__linux__)
#define ERROR_CODE_FORMAT "0x%d"
typedef s32 errorCode_t;
#else
#error Unrecognised platform!
#endif

/*
================================================================================================

	Debug functions

================================================================================================
*/

enum LogVerbosity {
	LOG_VERBOSITY_NONE = 0,
	LOG_VERBOSITY_ERROR,
	LOG_VERBOSITY_WARNING,
	LOG_VERBOSITY_INFO
};

enum ConsoleTextColor {
	CONSOLE_TEXT_COLOR_DEFAULT	= 0,
	CONSOLE_TEXT_COLOR_RED,
	CONSOLE_TEXT_COLOR_YELLOW,
	CONSOLE_TEXT_COLOR_BLUE,
	CONSOLE_TEXT_COLOR_BRIGHT_BLUE,
	CONSOLE_TEXT_COLOR_LIGHT_GRAY,
};

CORE_API void					set_console_text_color( const ConsoleTextColor color );

// DO NOT CALL THESE FUNCTIONS DIRECTLY
CORE_API void					info_internal( const char* function, const char* fmt, ... );
CORE_API void					warning_internal( const char* function, const char* fmt, ... );
CORE_API void					error_internal( const char* function, const char* fmt, ... );
CORE_API void					fatal_error_internal( const char* file, const int line, const char* prefix, const char* fmt, ... );

#define info( fmt, ... )		info_internal( __FUNCTION__, fmt, ##__VA_ARGS__ )
#define warning( fmt, ... )		warning_internal( __FUNCTION__, fmt, ##__VA_ARGS__ )
#define error( fmt, ... )		error_internal( __FUNCTION__, fmt, ##__VA_ARGS__ )

CORE_API void					set_log_verbosity( LogVerbosity verbosity );
CORE_API LogVerbosity			get_log_verbosity();
CORE_API void					dump_callstack( void );

CORE_API errorCode_t			get_last_error_code();

#ifndef fatal_error
	#define fatal_error( fmt, ... )	\
		do { \
			fatal_error_internal( __FILE__, __LINE__, "FATAL ERROR", fmt, ##__VA_ARGS__ ); \
			debug_break(); \
		} while ( 0 )
#endif

#if defined( _DEBUG )
	#if defined( _WIN32 )
		#define debug_break() \
			do { \
				if ( IsDebuggerPresent() ) { \
					__debugbreak(); \
				} \
			} while ( 0 )
	#elif defined( __linux__ )
		#define debug_break() \
			raise( SIGTRAP )
	#else
		#error Unrecognised platform.
	#endif

	// Helper macro for debugging.
	// This will trigger a breakpoint if the condition is met.
	#define debug_break_here_if( condition ) \
		do { \
			if ( (condition) ) { \
				debug_break(); \
			} \
		} while ( 0 )

	#define assert( x ) \
		do { \
			if ( !(x) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", #x ); \
				debug_break(); \
			} \
		} while ( 0 )

	#define assertf( x, fmt, ... ) \
		do { \
			if ( !(x) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", fmt, __VA_ARGS__ ); \
				debug_break(); \
			} \
		} while ( 0 )
#else
	#define debug_break()
	#define assert( x )
	#define assertf( x, fmt, ... )
#endif