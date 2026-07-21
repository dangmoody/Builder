/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

#include "int_types.h"

struct linearAllocator_t;
struct string_t;
template<class T> struct array_t;

#ifdef _WIN32
	#define DebugBreak	__debugbreak
#elif defined(__linux__)
	#define DebugBreak	__builtin_trap
#else
	#error Unrecognised platform!
#endif

#define DebugBreakHereIf( condition ) \
	do { \
		if ( ( condition ) ) { \
			DebugBreak(); \
		} \
	} while ( 0 )

#if defined( _WIN32 )
	#define ERROR_CODE_FORMAT "0x%X"
#elif defined( __linux__ )
	#define ERROR_CODE_FORMAT "%d"
#endif

#ifdef _DEBUG
	#define Assert( condition ) \
		do { \
			if ( !(condition) ) { \
				AssertInternal( __FILE__, __LINE__, #condition ); \
				DebugBreak(); \
			} \
		} while ( 0 )
#else
	#define Assert( condition )
#endif

enum ConsoleTextColor {
	CONSOLE_TEXT_COLOR_DEFAULT	= 0,
	CONSOLE_TEXT_COLOR_RED,
	CONSOLE_TEXT_COLOR_YELLOW,
	CONSOLE_TEXT_COLOR_BLUE,
	CONSOLE_TEXT_COLOR_BRIGHT_BLUE,
	CONSOLE_TEXT_COLOR_LIGHT_GRAY,
};

s32				GetLastErrorCode();

void			SetConsoleTextColor( const ConsoleTextColor color );

// Prints the format string to the console.
// %S is a special override that allows Core Strings to be printed (pass by value).
void			print( const char *fmt, ... );

// Prints "WARNING: " followed by the specified format string to the console.
void			warning( const char *fmt, ... );

// Prints "ERROR: " followed by the specified format string to the console.
void			error( const char *fmt, ... );

// Prints "FATAL ERROR: " followed by the specified format string to the console, then crashes.
void			FatalError( const char *fmt, ... );

// do not call this one directly
// call Assert() instead
void			AssertInternal( const char *file, const int line, const char *fmt, ... );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
