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

/*
================================================================================================

	Platform-agnostic debug

================================================================================================
*/

//Note(Tom): Question for Dan: is this going to be a problem across lib divides like you found with core context?
static LogVerbosity g_current_verbosity = LOG_VERBOSITY_INFO;

static void log( LogVerbosity required_verbosity, ConsoleTextColor prefix_color, ConsoleTextColor message_color, const char* prefix, const char* function, const char* fmt, va_list args ) {
	if ( g_current_verbosity < required_verbosity ) {
		return;
	}

	set_console_text_color( prefix_color );

#ifdef LOG_SHOW_FUNCTIONS
	printf( "%s(%s):  ", prefix, function );
#else
	unused( function );

	printf( "%s:  ", prefix );
#endif

	set_console_text_color( message_color );

	vprintf( fmt, args );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );
}

void info_internal(const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_INFO, CONSOLE_TEXT_COLOR_BRIGHT_BLUE, CONSOLE_TEXT_COLOR_LIGHT_GRAY, "INFO", function, fmt, args );
	va_end( args );
}

void warning_internal( const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_WARNING, CONSOLE_TEXT_COLOR_RED, CONSOLE_TEXT_COLOR_YELLOW, "WARNING", function, fmt, args );
	va_end( args );
}

void error_internal( const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_ERROR, CONSOLE_TEXT_COLOR_RED, CONSOLE_TEXT_COLOR_YELLOW, "ERROR", function, fmt, args );
	va_end( args );
}

void set_log_verbosity( LogVerbosity verbosity ) {
	g_current_verbosity = verbosity;
}

LogVerbosity get_log_verbosity() {
	return g_current_verbosity;
}

void fatal_error_internal( const char* file, const int line, const char* prefix, const char* fmt, ... ) {
#if defined( _WIN32 ) && defined( _DEBUG )
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
#endif

	va_list args;
	va_start( args, fmt );

	// build error msg
	int len = string_vsnprintf( NULL, 0, fmt, args );
	len++;	// + 1 for null terminator

	// u64 total_length = trunc_cast( u64, len );

	char* error_msg = cast( char*, alloca( len ) );
	string_vsnprintf( error_msg, len, fmt, args );
	error_msg[len - 1] = 0;

	assert_dialog_internal( file, line, prefix, error_msg );

	va_end( args );
}