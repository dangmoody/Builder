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

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

#include "dll_export.h"

// DM!!! write your own assert macro again, but better!
#include <assert.h>

#ifdef _WIN32
	#define debug_break			__debugbreak
#elif defined(__linux__)
	#define debug_break			abort( SIGTRAP )
#else
	#error Unrecognised platform!
#endif

enum ConsoleTextColor {
	CONSOLE_TEXT_COLOR_DEFAULT	= 0,
	CONSOLE_TEXT_COLOR_RED,
	CONSOLE_TEXT_COLOR_YELLOW,
	CONSOLE_TEXT_COLOR_BLUE,
	CONSOLE_TEXT_COLOR_BRIGHT_BLUE,
	CONSOLE_TEXT_COLOR_LIGHT_GRAY,
};

CORE_API s32		get_last_error_code();

CORE_API void		set_console_text_color( const ConsoleTextColor color );

CORE_API void		warning( const char* fmt, ... );
CORE_API void		error( const char* fmt, ... );
CORE_API void		fatal_error( const char* fmt, ... );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
