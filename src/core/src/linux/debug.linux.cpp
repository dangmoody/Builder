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

s32 get_last_error_code() {
	// errno is a global and therefore not thread safe
	// so it MUST ALWAYS be cached ASAP
	int err = errno;
	return err;
}

#endif // __linux__