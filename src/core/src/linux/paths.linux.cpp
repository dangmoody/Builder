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

#include <paths.h>

#include <core_helpers.h>
#include <debug.h>
#include <paths.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <core_string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/*
================================================================================================

	Paths

	Linux-specific functionality

================================================================================================
*/

const char *path_app_path() {
	char *result = cast( char *, mem_temp_alloc( PATH_MAX * sizeof( char ) ) );
	s64 length = readlink( "/proc/self/exe", result, PATH_MAX );

	if ( length == -1 ) {
		int err = errno;
		fatal_error( "Failed to get app path: %s.\n", strerror( err ) );
	}

	result[length] = 0;

	return result;
}

const char *path_get_cwd() {
	char *temp = cast( char *, mem_temp_alloc( PATH_MAX * sizeof( char ) ) );

	const char *cwd = getcwd( temp, PATH_MAX * sizeof( char ) );

	if ( !cwd ) {
		int err = errno;
		fatal_error( "Failed to get CWD: %s.\n", strerror( err ) );
	}

	return cwd;
}

bool8 path_set_cwd( const char *path ) {
	if ( chdir( path ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to set current directory: %s.\n", strerror( err ) );

		return false;
	}

	return true;
}

const char *path_absolute_path( const char *path ) {
	char *path_copy = temp_c_string( path, PATH_MAX * sizeof( char ) );

	const char *result = realpath( path_copy, NULL );

	if ( !result ) {
		int err = errno;
		fatal_error( "Failed to get absolute path of \"%s\": %s.\n", path, strerror( err ) );
		return NULL;
	}

	return result;
}

bool8 path_is_absolute( const char *path ) {
	assert( path );

	return path[0] == '/';
}

const char *path_canonicalize( const char *path ) {
	assert( path );

	char *path_copy = temp_c_string( path, PATH_MAX * sizeof( char ) );

	const char *result = realpath( path_copy, NULL );

	if ( !result ) {
		int err = errno;
		fatal_error( "Failed to canoncalize path \"%s\": %s.\n", path, strerror( err ) );
		return NULL;
	}

	return result;
}

const char *path_fix_slashes( const char *path ) {
	return string_replace( path, '\\', '/' );
}

#endif // __linux__
