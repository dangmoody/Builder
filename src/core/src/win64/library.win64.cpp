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

#include <library.h>

#include <debug.h>
#include <typecast.inl>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

/*
================================================================================================

	Library

================================================================================================
*/

Library library_load( const char* name ) {
	assert( name );

	return { LoadLibraryA( name ) };
}

bool8 library_unload( Library* library ) {
	assert( library && library->ptr );

	if ( !FreeLibrary( cast( HMODULE, library->ptr ) ) ) {
		return false;
	}

	library->ptr = NULL;

	return true;
}

void* library_get_symbol( const Library library, const char* symbol_name ) {
	assert( library.ptr );
	assert( symbol_name );

	FARPROC symbol = GetProcAddress( cast( HMODULE, library.ptr ), symbol_name );

	return cast( void*, symbol );
}

#endif // _WIN32