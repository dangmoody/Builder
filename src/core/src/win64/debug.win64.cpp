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

#ifdef _WIN64

#include <debug.h>
#include <core_types.h>
//#include <string_helpers.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include <stdio.h>	// printf, vprintf
#include <stdlib.h>	// malloc, free
#include <malloc.h>	// alloca

#include <string_helpers.h>

/*
================================================================================================

	Debug

================================================================================================
*/

enum ConsoleColor{
	CONSOLE_COLOR_DEFAULT		= 0x07,
	CONSOLE_COLOR_RED			= 0x0C,
	CONSOLE_COLOR_YELLOW		= 0x0E,
	CONSOLE_COLOR_BLUE			= 0x01,
	CONSOLE_COLOR_BRIGHT_BLUE	= 0x09,
	CONSOLE_COLOR_LIGHT_GRAY	= 0x07
};

//Note(Tom): Question for Dan: is this going to be a problem across lib divides like you found with core context?
static LogVerbosity g_current_verbosity = LOG_VERBOSITY_INFO;

static void log( LogVerbosity required_verbosity, ConsoleColor prefix_color, ConsoleColor message_color, const char* prefix, const char* function, const char* fmt, va_list args ) {
	if ( g_current_verbosity < required_verbosity ) {
		return;
	}

	unused( function );

	HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );

	SetConsoleTextAttribute( handle, prefix_color );

#ifdef LOG_SHOW_FUNCTIONS
	printf( "\n%s(%s):  ", prefix, function );
#else
	printf( "\n%s:  ", prefix );
#endif

	SetConsoleTextAttribute( handle, message_color );

	vprintf( fmt, args );

	SetConsoleTextAttribute( handle, CONSOLE_COLOR_DEFAULT );
}

void info_internal(const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_INFO, CONSOLE_COLOR_BRIGHT_BLUE, CONSOLE_COLOR_LIGHT_GRAY, "INFO", function, fmt, args );
	va_end( args );
}

void warning_internal( const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_WARNING, CONSOLE_COLOR_RED, CONSOLE_COLOR_YELLOW, "WARNING",function, fmt, args );
	va_end( args );
}

void error_internal( const char* function, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	log( LOG_VERBOSITY_ERROR, CONSOLE_COLOR_RED, CONSOLE_COLOR_YELLOW, "ERROR", fmt, function, args );
	va_end( args );
}

void set_log_verbosity( LogVerbosity verbosity ) {
	g_current_verbosity = verbosity;
}

LogVerbosity get_log_verbosity() {
	return g_current_verbosity;
}

#ifdef _DEBUG
// DM!!! this is broken...somehow
// need to fix it
void dump_callstack( void ) {
	HANDLE process = GetCurrentProcess();

	// DM!!!
	// we need to call this once during initialization
	// idk if we want true or false for last parameter yet
	SymInitialize( process, NULL, TRUE );

	CONTEXT context_record;
	RtlCaptureContext( &context_record );

	STACKFRAME64 stack_frame = {};
	stack_frame.AddrPC.Offset = context_record.Rip;
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrStack.Offset = context_record.Rsp;
	stack_frame.AddrStack.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Offset = context_record.Rbp;
	stack_frame.AddrFrame.Mode = AddrModeFlat;

	// TODO(DM): determine this via our own define (RZ_X64, RZ_86, etc)?
#ifdef _WIN64
	DWORD machine_image_type = IMAGE_FILE_MACHINE_AMD64;
#else
	DWORD machine_image_type = IMAGE_FILE_MACHINE_I386;
#endif

	printf( "Callstack:\n" );

	for ( ULONG i = 0; ; i++ ) {
		BOOL more_stack_left_to_walk = StackWalk( machine_image_type, process, GetCurrentThread(), &stack_frame, &context_record, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL );

		IMAGEHLP_SYMBOL64* symbol = cast( IMAGEHLP_SYMBOL64* ) malloc( sizeof( IMAGEHLP_SYMBOL64 ) + MAX_PATH * sizeof( TCHAR ) );
		symbol->SizeOfStruct = sizeof( IMAGEHLP_SYMBOL64 );
		symbol->MaxNameLength = MAX_PATH;	// DM!!! is this correct?

		DWORD displacement = 0;
		DWORD64 displacement64 = 0;

		/*BOOL got_symbol =*/ SymGetSymFromAddr( process, stack_frame.AddrPC.Offset, &displacement64, symbol );
		/*assert( got_symbol );
		unused( got_symbol );*/

		IMAGEHLP_LINE64 line = {};
		/*BOOL got_line =*/ SymGetLineFromAddr( process, stack_frame.AddrPC.Offset, &displacement, &line );
		/*assert( got_line );
		unused( got_line );*/

		char symbol_name[MAX_PATH];
		if ( UnDecorateSymbolName( symbol->Name, cast( PSTR ) symbol_name, MAX_PATH, UNDNAME_COMPLETE ) == 0 ) {
			fatal_error( "Stack walk failed: UnDecorateSymbolName failed: 0x%X.", GetLastError() );
		}

		// no point in showing functions after main
		if ( strncmp( symbol_name, "main", 5 ) == 0 ) {
			more_stack_left_to_walk = false;
		}

		printf( "[%lu]: %s (%s:%lu)\n", i, symbol_name, line.FileName, line.LineNumber );

		free( symbol );
		symbol = NULL;

		if ( !more_stack_left_to_walk ) {
			break;
		}
	}
}
#endif // _DEBUG

static void assert_dialog_internal( const char* file, const int line, const char* prefix, const char* msg ) {
	HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );

	SetConsoleTextAttribute( handle, CONSOLE_COLOR_RED );

	printf( "%s: %s line %d: ", prefix, file, line );

	SetConsoleTextAttribute( handle, CONSOLE_COLOR_YELLOW );

	printf( "%s\n", msg );

#ifdef _DEBUG
	dump_callstack();
#endif

	SetConsoleTextAttribute( handle, CONSOLE_COLOR_DEFAULT );

#ifdef _DEBUG
	_CrtDbgReport( _CRT_ASSERT, file, line, NULL, msg );
#else
	MessageBox( NULL, msg, "ASSERTION ERROR", MB_OK );
#endif
}

void fatal_error_internal( const char* file, const int line, const char* prefix, const char* fmt, ... ) {
#ifdef _DEBUG
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
#endif

	va_list args;
	va_start( args, fmt );

	// build error msg
	int len = string_vsnprintf( NULL, 0, fmt, args );
	len++;	// + 1 for null terminator

	s64 total_length = len;

	char* error_msg = cast( char* ) alloca( total_length );
	string_vsnprintf( error_msg, total_length, fmt, args );
	error_msg[total_length - 1] = 0;

	assert_dialog_internal( file, line, prefix, error_msg );

	va_end( args );
}

#endif // _WIN64