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

#include "debug.h"

#include "array.inl"
#include "string.h"
#include "helpers.h"
#include "temp_storage.h"
#include "defer.h"

#include "stb_local.h"

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

static void print_varargs( FILE *file, const char *fmt, va_list args ) {
	u64 pos = mem_temp_tell();
	defer { mem_temp_rewind_to( pos ); };

	va_list args_copy;
	va_copy( args_copy, args );

	int length = stbsp_vsnprintf( NULL, 0, fmt, args );

	char *msg = cast( char *, mem_temp_alloc( cast( u64, length + 1 ) ) );
	stbsp_vsnprintf( msg, length + 1, fmt, args_copy );
	msg[length] = 0;

	fputs( msg, file );

	va_end( args_copy );
}

void print( const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	print_varargs( stdout, fmt, args );
	va_end( args );
}

void warning( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	fputs( "WARNING: ", stderr );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	print_varargs( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void error( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	fputs( "ERROR: ", stderr );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	print_varargs( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void fatal_error( const char *fmt, ... ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	print( "FATAL ERROR: " );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	va_list args;
	va_start( args, fmt );
	print_varargs( stderr, fmt, args );
	va_end( args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void dump_callstack() {
	Array<String> callstack = get_callstack( mem_get_temp_storage() );

	For ( u64, frame_index, 0, callstack.count ) {
		print( "[%" PRIu64 "]: %S\n", frame_index, callstack[frame_index] );
	}
}
