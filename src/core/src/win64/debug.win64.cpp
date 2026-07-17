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

#ifdef _WIN32

#include <debug.h>

#include "../stb_local.h"

#include <core_array.inl>
#include <core_string.h>
#include <core_helpers.h>
#include <linear_allocator.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <defer.h>

#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>	// alloca

Array<String> get_callstack( LinearAllocator *allocator ) {
	Array<String> callstack;
	callstack.init( allocator );

	HANDLE process = GetCurrentProcess();

	static bool8 sym_initialized = false;
	if ( !sym_initialized ) {
		SymInitialize( process, NULL, TRUE );
		sym_initialized = true;
	}

	// TODO: DM: 05/04/2026: can we do better than a hardcoded constant?
	void *frames[1024];
	u16 frame_count = CaptureStackBackTrace( 1, 1024, frames, NULL );

	u8 symbol_buffer[sizeof( SYMBOL_INFO ) + MAX_SYM_NAME];

	For ( u16, i, 0, frame_count ) {
		DWORD64 address = cast( DWORD64, cast( u64, frames[i] ) );

		SYMBOL_INFO *symbol = cast( SYMBOL_INFO *, symbol_buffer );
		memset( symbol, 0, sizeof( SYMBOL_INFO ) );
		symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
		symbol->MaxNameLen = MAX_SYM_NAME;

		DWORD64 displacement = 0;
		if ( SymFromAddr( process, address, &displacement, symbol ) ) {
			callstack.add( string_alloc( allocator, symbol->Name ) );
		} else {
			callstack.add( string_alloc( allocator, "???" ) );
		}
	}

	return callstack;
}

void set_console_text_color( const ConsoleTextColor color ) {
	HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );

	WORD color_code = 0;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		color_code = 0x07; break;
		case CONSOLE_TEXT_COLOR_RED:			color_code = 0x0C; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			color_code = 0x0E; break;
		case CONSOLE_TEXT_COLOR_BLUE:			color_code = 0x01; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	color_code = 0x09; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		color_code = 0x07; break;
	}

	assert( color_code != 0 );

	SetConsoleTextAttribute( handle, color_code );
}

s32 get_last_error_code() {
	return trunc_cast( s32, GetLastError() );
}

void assert_internal( const char *file, const int line, const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	defer { va_end( args ); };

	va_list args_copy;
	va_copy( args_copy, args );
	defer { va_end( args_copy ); };

	int length = stbsp_vsnprintf( NULL, 0, fmt, args );
	char *buffer = cast( char *, alloca( length + 1 ) );
	stbsp_vsnprintf( buffer, length + 1, fmt, args_copy );
	buffer[length] = 0;

	set_console_text_color( CONSOLE_TEXT_COLOR_RED );

	print( "ASSERT FAILURE: %s line %d: ", file, line );

	set_console_text_color( CONSOLE_TEXT_COLOR_YELLOW );

	print( "%s\n", buffer );

	set_console_text_color( CONSOLE_TEXT_COLOR_DEFAULT );

	// TODO: DM: when we eventually figure out what the X11/wayland/XCB equivalents to these are...
	// move the code above into debug.cpp and then make it call a function like "dialog_box_internal()", or something
#ifdef _DEBUG
	_CrtDbgReport( _CRT_ASSERT, file, line, NULL, buffer );
#else
	MessageBox( NULL, buffer, "ASSERTION ERROR", MB_OK );
#endif
}

#endif // _WIN32
