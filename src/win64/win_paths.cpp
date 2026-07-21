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

#include "../paths.h"

#include "../helpers.h"
#include "../temp_storage.h"
#include "../debug.h"
#include "../typecast.h"
#include "../string.h"

#include <Windows.h>
#include <Shlwapi.h>

/*
================================================================================================

	Paths

	Win64-specific functionality

================================================================================================
*/

String path_app_path( LinearAllocator *allocator ) {
	char app_full_path[MAX_PATH] = {};
	DWORD length = GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

	return string_alloc( allocator, app_full_path, length );
}

String path_get_cwd( LinearAllocator *allocator ) {
	char cwd[MAX_PATH] = {};
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );

	return string_alloc( allocator, cwd, length );
}

bool8 path_set_cwd( const char *path ) {
	assert( path );

	return cast( bool8, SetCurrentDirectory( path ) );
}

String path_absolute_path( LinearAllocator *allocator, const char *path ) {
	assert( path );

	char absolute_path[MAX_PATH] = {};
	DWORD length = GetFullPathName( path, MAX_PATH, absolute_path, NULL );

	return string_alloc( allocator, absolute_path, length );
}

bool8 path_is_absolute( const char *path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

String path_fix_slashes( LinearAllocator *allocator, String *str ) {
	return string_replace( allocator, str, '/', PATH_SEPARATOR );
}

#endif // _WIN32
