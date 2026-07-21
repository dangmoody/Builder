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

#ifdef __linux__

#include "../debug.h"

#include "../array.inl"
#include "../string.h"
#include "../linear_allocator.h"
#include "../temp_storage.h"
#include "../defer.h"

#include <malloc.h>
#include <errno.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

void SetConsoleTextColor( const consoleTextColor_t color ) {
	const char *colorLinux = NULL;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		colorLinux = "\033[0m";    break;
		case CONSOLE_TEXT_COLOR_RED:			colorLinux = "\033[0;31m"; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			colorLinux = "\033[0;32m"; break;
		case CONSOLE_TEXT_COLOR_BLUE:			colorLinux = "\033[1;34m"; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	colorLinux = "\033[1;94m"; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		colorLinux = "\033[1;37m"; break;
	}

	Assert( colorLinux != NULL );

	Print( "%s", colorLinux );
}

s32 GetLastErrorCode() {
	return errno;
}

void AssertInternal( const char *file, const int line, const char *fmt, ... ) {
	UNUSED( file );
	UNUSED( line );
	UNUSED( fmt );

	// TODO: DM: write me!
}

#endif // __linux__
