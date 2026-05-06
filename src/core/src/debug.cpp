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

#include <core_array.inl>
#include <core_helpers.h>
#include <temp_storage.h>

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

void warning( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "WARNING: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void error( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "ERROR: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void fatal_error( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "FATAL ERROR: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void dump_callstack() {
	Array<const char *> callstack = get_callstack( g_temp_storage );

	For ( u64, i, 0, callstack.count ) {
		printf( "[%" PRIu64 "]: %s\n", i, callstack[i] );
	}
}
