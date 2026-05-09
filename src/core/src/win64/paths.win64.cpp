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

#include <paths.h>

#include <core_helpers.h>
#include <temp_storage.h>
#include <debug.h>
#include <typecast.inl>
#include <core_string.h>

#include <Windows.h>
#include <Shlwapi.h>

/*
================================================================================================

	Paths

	Win64-specific functionality

================================================================================================
*/

const char *path_app_path() {
	char *app_full_path = cast( char *, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

	return app_full_path;
}

const char *path_current_working_directory() {
	char *cwd = cast( char *, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );
	cwd[length] = 0;

	return cwd;
}

const char *path_absolute_path( const char *path ) {
	assert( path );

	char *absolute_path = cast( char *, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetFullPathName( path, MAX_PATH, absolute_path, NULL );
	absolute_path[length] = 0;

	return absolute_path;
}

bool8 path_is_absolute( const char *path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

const char *path_canonicalize( const char *path ) {
	assert( path );

	const char *path_copy = path_fix_slashes( path );

	u64 max_path_length = ( strlen( path_copy ) + 1 ) * sizeof( char );

	char *result = cast( char *, mem_temp_alloc( max_path_length ) );

	BOOL success = PathCanonicalizeA( result, path_copy );
	assert( success );
	unused( success );

	result[max_path_length - 1] = 0;

	if ( *result == '/' ) {
		result++;
	} else if ( *result == '\\' ) {
		result++;
	}

	return result;
}

void path_fix_slashes( String *str ) {
	return string_replace( str, '/', PATH_SEPARATOR );
}

bool8 path_set_current_directory( const char *path ) {
	assert( path );

	return cast( bool8, SetCurrentDirectory( path ) );
}

#endif // _WIN32
