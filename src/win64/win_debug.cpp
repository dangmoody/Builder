/*
===========================================================================

Builder

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

#include "../debug.h"

#include "../stb_local.h"

#include "../array.inl"
#include "../string.h"
#include "../helpers.h"
#include "../linear_allocator.h"
#include "../temp_storage.h"
#include "../typecast.h"
#include "../defer.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>	// alloca

void SetConsoleTextColor( const consoleTextColor_t color ) {
	HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );

	WORD colorCode = 0;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		colorCode = 0x07; break;
		case CONSOLE_TEXT_COLOR_RED:			colorCode = 0x0C; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			colorCode = 0x0E; break;
		case CONSOLE_TEXT_COLOR_BLUE:			colorCode = 0x01; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	colorCode = 0x09; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		colorCode = 0x07; break;
	}

	Assert( colorCode != 0 );

	SetConsoleTextAttribute( handle, colorCode );
}

s32 GetLastErrorCode() {
	return TruncCast( s32, GetLastError() );
}

void AssertInternal( const char *file, const int line, const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	defer { va_end( args ); };

	va_list argsCopy;
	va_copy( argsCopy, args );
	defer { va_end( argsCopy ); };

	int length = stbsp_vsnprintf( NULL, 0, fmt, args );
	char *buffer = Cast( char *, alloca( length + 1 ) );
	stbsp_vsnprintf( buffer, length + 1, fmt, argsCopy );
	buffer[length] = 0;

	SetConsoleTextColor( CONSOLE_TEXT_COLOR_RED );

	print( "ASSERT FAILURE: %s line %d: ", file, line );

	SetConsoleTextColor( CONSOLE_TEXT_COLOR_YELLOW );

	print( "%s\n", buffer );

	SetConsoleTextColor( CONSOLE_TEXT_COLOR_DEFAULT );

	// TODO: DM: when we eventually figure out what the X11/wayland/XCB equivalents to these are...
	// move the code above into debug.cpp and then make it call a function like "dialog_box_internal()", or something
#ifdef _DEBUG
	_CrtDbgReport( _CRT_ASSERT, file, line, NULL, buffer );
#else
	MessageBox( NULL, buffer, "ASSERTION ERROR", MB_OK );
#endif
}

#endif // _WIN32
