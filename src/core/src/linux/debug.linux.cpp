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

#ifdef __linux__

#include <debug.h>

#include <core_types.h>
#include <string_helpers.h>
#include <typecast.inl>

#include <stdio.h>	// printf, vprintf
#include <stdlib.h>	// malloc, free
#include <malloc.h>	// alloca
#include <errno.h>  // linux specific error code handling
#include <string.h> // linux specific error code conversion to string

#include <stdarg.h>

/*
================================================================================================

	Linux debug

================================================================================================
*/

void set_console_text_color( const ConsoleTextColor color ) {
	const char* color_linux = NULL;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		color_linux = "\033[0m"; break;
		case CONSOLE_TEXT_COLOR_RED:			color_linux = "\033[0;31m"; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			color_linux = "\033[0;32m"; break;
		case CONSOLE_TEXT_COLOR_BLUE:			color_linux = "\033[1;34m"; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	color_linux = "\033[1;94m"; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		color_linux = "\033[1;37m"; break;
	}

	assert( color_linux != NULL );

	printf( "%s", color_linux );
}

#ifdef _DEBUG
void dump_callstack( void ) {
	printf(
		"Callstack:\n"
		"TODO(DM): this\n"
	);
}
#endif // _DEBUG

static void assert_dialog_internal( const char* file, const int line, const char* prefix, const char* msg ) {
	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	printf( "%s: %s line %d: ", prefix, file, line );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	printf( "%s\n", msg );

#ifdef _DEBUG
	dump_callstack();
#endif

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );

#ifdef _DEBUG
	//_CrtDbgReport( _CRT_ASSERT, file, line, NULL, msg );
#else
	// TODO (MY) - maybe consider revisiting
	// MessageBox( NULL, msg, "ASSERTION ERROR", MB_OK );
#endif
    printf( "FATAL ERROR!\n" );    // DM!!! whats the linux equivalent of MessageBox()?
}

errorCode_t get_last_error_code() {
	errorCode_t captured_error = errno;

	if ( captured_error != 0 ) {
		printf( "Linux error: %s\n", strerror( captured_error ) );
	}

	return captured_error;
}

#endif // __linux__
