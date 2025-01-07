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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

/*
================================================================================================

	Debug functions

================================================================================================
*/

// logging
void							warning( const char* fmt, ... );
void							error( const char* fmt, ... );

void							dump_callstack( void );

// DO NOT CALL THESE FUNCTIONS DIRECTLY
void							fatal_error_internal( const char* file, const int line, const char* prefix, const char* fmt, ... );


#ifndef fatal_error
	#define fatal_error( fmt, ... )	\
		do { \
			fatal_error_internal( __FILE__, __LINE__, "FATAL ERROR", fmt, __VA_ARGS__ ); \
			debug_break(); \
		} while ( 0 )
#endif

#ifdef _DEBUG
	#ifdef _WIN32
		#define debug_break() \
			do { \
				if ( IsDebuggerPresent() ) { \
					__debugbreak(); \
				} \
			} while ( 0 )
	#else
		#error Unrecognised platform.
	#endif

	// helper macro for debugging
	// will trigger a breakpoint if the condition is met
	#define debug_break_here_if( condition ) \
		do { \
			if ( (condition) ) { \
				debug_break(); \
			} \
		} while ( 0 )

	#define assert( x ) \
		do { \
			if ( !( x ) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", #x ); \
				debug_break(); \
			} \
		} while ( 0 )

	#define assertf( x, fmt, ... ) \
		do { \
			if ( !( x ) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", fmt, __VA_ARGS__ ); \
				debug_break(); \
			} \
		} while ( 0 )
#else
	#define debug_break()
	#define assert( x )
	#define assertf( x, fmt, ... )
#endif