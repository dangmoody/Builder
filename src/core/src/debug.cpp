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

#include <debug.h>

#include <stdio.h>
#include <stdarg.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#endif

void warning( const char* fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "WARNING: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
}

void error( const char* fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "ERROR: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
}

void fatal_error( const char* fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "FATAL ERROR: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
