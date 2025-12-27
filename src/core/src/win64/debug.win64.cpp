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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

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

#endif // _WIN32
