/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef _WIN64

#include "library.h"
#include "debug.h"

#define WIN32_LEAN_AND_MEAN 1
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

	BOOL freed = FreeLibrary( cast( HMODULE ) library->ptr );

	assert( freed );
	unused( freed );

	library->ptr = NULL;
}

void* library_get_proc_address( const Library library, const char* func_name ) {
	assert( library.ptr );
	assert( func_name );

	FARPROC proc_addr = GetProcAddress( cast( HMODULE ) library.ptr, func_name );

	//assert( proc_addr );

	return cast( void* ) proc_addr;
}

#endif // _WIN64