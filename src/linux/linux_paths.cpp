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

#include "../paths.h"

#include "../debug.h"
#include "../temp_storage.h"
#include "../typecast.h"
#include "../string.h"
#include "../defer.h"
#include "../linear_allocator.h"

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

string_t Path_AppPath( linearAllocator_t *allocator ) {
	Assert( allocator );

	char appPath[PATH_MAX] = {};
	s64 length = readlink( "/proc/self/exe", appPath, PATH_MAX );

	if ( length == -1 ) {
		int err = errno;
		FatalError( "Failed to get app path: %s.\n", strerror( err ) );
		return {};
	}

	return String_Alloc( allocator, appPath, TruncCast( u64, length ) );
}

string_t Path_GetCwd( linearAllocator_t *allocator ) {
	Assert( allocator );

	const char *cwd = getcwd( NULL, PATH_MAX );

	if ( !cwd ) {
		int err = errno;
		FatalError( "Failed to get CWD: %s.\n", strerror( err ) );
		return {};
	}

	return String_Alloc( allocator, cwd, strlen( cwd ) );
}

bool8 Path_SetCwd( const char *path ) {
	Assert( path );

	if ( chdir( path ) != 0 ) {
		int err = errno;
		FatalError( "Failed to set current directory to \"%s\": %s.\n", path, strerror( err ) );

		return false;
	}

	return true;
}

string_t Path_AbsolutePath( linearAllocator_t *allocator, const char *path ) {
	Assert( allocator );
	Assert( path );

	char pathCopy[PATH_MAX] = {};
	memcpy( pathCopy, path, strlen( path ) );
	const char *result = realpath( pathCopy, NULL );

	if ( !result ) {
		int err = errno;
		FatalError( "Failed to get absolute path of \"%s\": %s.\n", path, strerror( err ) );
		return {};
	}

	return String_Alloc( allocator, result, strlen( result ) );
}

bool8 Path_IsAbsolute( const char *path ) {
	Assert( path );

	return path[0] == '/';
}

string_t Path_FixSlashes( linearAllocator_t *allocator, string_t *str ) {
	return String_Replace( allocator, str, '\\', PATH_SEPARATOR );
}

#endif // __linux__
