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

#include <library.h>
#include <debug.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
================================================================================================

	Library

================================================================================================
*/

Library library_load( const char* name ) {
	assert( name );

	HMODULE library = LoadLibraryA( name );

	assertf( library, "Failed to load dynamic library \"%s\": 0x%X", name, GetLastError() );

	return { library };
}

void library_unload( Library* library ) {
	assert( library && library->ptr );

	BOOL freed = FreeLibrary( cast( HMODULE, library->ptr ) );

	assert( freed );
	unused( freed );

	library->ptr = NULL;
}

void* library_get_proc_address( const Library library, const char* func_name ) {
	assert( library.ptr );
	assert( func_name );

	FARPROC proc_addr = GetProcAddress( cast( HMODULE, library.ptr ), func_name );

	//assert( proc_addr );

	return cast( void*, proc_addr );
}

#endif // _WIN64