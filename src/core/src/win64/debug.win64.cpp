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

#ifdef _WIN32

#include <debug.h>

#include <core_types.h>
#include <string_helpers.h>
#include <typecast.inl>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include <stdio.h>	// printf, vprintf
#include <stdlib.h>	// malloc, free
#include <malloc.h>	// alloca

/*
================================================================================================

	Win64 debug

================================================================================================
*/

void set_console_text_color( const ConsoleTextColor color ) {
	HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );

	WORD color_win64 = 0;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		color_win64 = 0x07; break;
		case CONSOLE_TEXT_COLOR_RED:			color_win64 = 0x0C; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			color_win64 = 0x0E; break;
		case CONSOLE_TEXT_COLOR_BLUE:			color_win64 = 0x01; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	color_win64 = 0x09; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		color_win64 = 0x07; break;
	}

	assert( color_win64 != 0 );

	SetConsoleTextAttribute( handle, color_win64 );
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

		IMAGEHLP_SYMBOL64* symbol = cast( IMAGEHLP_SYMBOL64*, malloc( sizeof( IMAGEHLP_SYMBOL64 ) + MAX_PATH * sizeof( TCHAR ) ) );
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
		if ( UnDecorateSymbolName( symbol->Name, cast( PSTR, symbol_name ), MAX_PATH, UNDNAME_COMPLETE ) == 0 ) {
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

	SetConsoleTextAttribute( handle, CONSOLE_TEXT_COLOR_RED );

	printf( "%s: %s line %d: ", prefix, file, line );

	SetConsoleTextAttribute( handle, CONSOLE_TEXT_COLOR_YELLOW );

	printf( "%s\n", msg );

#ifdef _DEBUG
	dump_callstack();
#endif

	SetConsoleTextAttribute( handle, CONSOLE_TEXT_COLOR_DEFAULT );

#ifdef _DEBUG
	_CrtDbgReport( _CRT_ASSERT, file, line, NULL, msg );
#else
	MessageBox( NULL, msg, "ASSERTION ERROR", MB_OK );
#endif
}

errorCode_t get_last_error_code() {
	return GetLastError();
}

#endif // _WIN32