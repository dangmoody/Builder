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

#include "../library.h"

#include "../debug.h"
#include "../typecast.h"

#include <Windows.h>

/*
================================================================================================

	library_t

================================================================================================
*/

library_t Library_Load( const char *name ) {
	Assert( name );

	return { LoadLibraryA( name ) };
}

bool8 Library_Unload( library_t *library ) {
	Assert( library && library->ptr );

	if ( !FreeLibrary( Cast( HMODULE, library->ptr ) ) ) {
		return false;
	}

	library->ptr = NULL;

	return true;
}

void* Library_GetSymbol( const library_t library, const char *symbolName ) {
	Assert( library.ptr );
	Assert( symbolName );

	FARPROC symbol = GetProcAddress( Cast( HMODULE, library.ptr ), symbolName );

	return Cast( void *, symbol );
}

#endif // _WIN32
