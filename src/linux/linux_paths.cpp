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

#ifdef __linux__

#include <paths.h>

#include <core_helpers.h>
#include <debug.h>
#include <paths.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <core_string.h>
#include <defer.h>
#include <linear_allocator.h>

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

String path_app_path( LinearAllocator *allocator ) {
	assert( allocator );

	char app_path[PATH_MAX] = {};
	s64 length = readlink( "/proc/self/exe", app_path, PATH_MAX );

	if ( length == -1 ) {
		int err = errno;
		fatal_error( "Failed to get app path: %s.\n", strerror( err ) );
		return {};
	}

	return string_alloc( allocator, app_path, trunc_cast( u64, length ) );
}

String path_get_cwd( LinearAllocator *allocator ) {
	assert( allocator );

	const char *cwd = getcwd( NULL, PATH_MAX );

	if ( !cwd ) {
		int err = errno;
		fatal_error( "Failed to get CWD: %s.\n", strerror( err ) );
		return {};
	}

	return string_alloc( allocator, cwd, strlen( cwd ) );
}

bool8 path_set_cwd( const char *path ) {
	assert( path );

	if ( chdir( path ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to set current directory to \"%s\": %s.\n", path, strerror( err ) );

		return false;
	}

	return true;
}

String path_absolute_path( LinearAllocator *allocator, const char *path ) {
	assert( allocator );
	assert( path );

	char path_copy[PATH_MAX] = {};
	memcpy( path_copy, path, strlen( path ) );
	const char *result = realpath( path_copy, NULL );

	if ( !result ) {
		int err = errno;
		fatal_error( "Failed to get absolute path of \"%s\": %s.\n", path, strerror( err ) );
		return {};
	}

	return string_alloc( allocator, result, strlen( result ) );
}

bool8 path_is_absolute( const char *path ) {
	assert( path );

	return path[0] == '/';
}

String path_fix_slashes( LinearAllocator *allocator, String *str ) {
	return string_replace( allocator, str, '\\', PATH_SEPARATOR );
}

#endif // __linux__
