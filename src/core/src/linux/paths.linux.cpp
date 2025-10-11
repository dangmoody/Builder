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

#include <core_types.h>
#include <debug.h>
#include <paths.h>
#include <temp_storage.h>
#include <string_builder.h>

#include <stdio.h>
#include <unistd.h>
#include <limits.h>

/*
================================================================================================

	Paths

	Linux-specific functionality

================================================================================================
*/

const char* path_app_path() {
	char* result = cast( char*, mem_temp_alloc( PATH_MAX * sizeof( char ) ) );
	s64 length = readlink( "/proc/self/exe", result, PATH_MAX );

	if ( length == -1 ) {
		int err = errno;
		fatal_error( "Failed to get app path: %s.\n", strerror( err ) );
	}

	result[length] = 0;

	return path_remove_file_from_path( result );
}

const char* path_current_working_directory() {
	char temp[PATH_MAX];

	const char* cwd = getcwd( temp, sizeof( temp ) );

	if ( !cwd ) {
		int err = errno;
		fatal_error( "Failed to get CWD: %s.\n", strerror( err ) );
	}

	return cwd;
}

const char* path_absolute_path( const char* file ) {
	unused( file );

	assert( false );

	return NULL;
}

bool8 path_is_absolute( const char* path ) {
	assert( path );

	return path[0] == '/';
}

const char* path_canonicalise( const char* path ) {
	assert( path );

	char* path_copy = cast( char*, mem_temp_alloc( PATH_MAX * sizeof( char ) ) );
	strncpy( path_copy, path, PATH_MAX * sizeof( char ) );

	const char* result = realpath( path_copy, NULL );
	if ( !result ) {
		int err = errno;
		printf( "Failed to get real path of \"%s\": %s.\n", path, strerror( err ) );
		return NULL;
	}

	return result;
}

const char* path_fix_slashes( const char* path ) {
	u64 path_length = strlen( path );
	char* result = cast( char*, mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) ) );
	memcpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	For ( u64, char_index, 0, path_length ) {
		if ( result[char_index] == '\\' ) {
			result[char_index] = '/';
		}
	}

	return result;
}

char* path_relative_path_to( const char* path_from, const char* path_to ) {
	assert( path_from );
	assert( path_to );

	const char* path_from_copy = path_from;
	const char* path_to_copy = path_to;

	u32 num_same_chars = 0;
	u32 num_backs = 0;

	while ( path_from_copy[num_same_chars] && path_to_copy[num_same_chars] && path_from_copy[num_same_chars] == path_to_copy[num_same_chars] ) {
		num_same_chars += 1;
	}

	path_from_copy = path_from + num_same_chars;
	path_to_copy = path_to + num_same_chars;

	// skip the first one of these if there is one
	/*if ( *path_from_copy == '/' ) {
		path_from_copy += 1;
	}*/

	while ( *path_from_copy ) {
		if ( *path_from_copy == '/' ) {
			num_backs += 1;
		}
		path_from_copy += 1;
	}

	StringBuilder sb = {};
	string_builder_reset( &sb );

	For ( u32, back_index, 0, num_backs ) {
		string_builder_appendf( &sb, "../" );
	}

	string_builder_appendf( &sb, path_to + num_same_chars );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
	//char* result = cast( char*, mem_temp_alloc( PATH_MAX * sizeof( char ) ) );
	char* result = cast( char*, string_builder_to_string( &sb ) );
#pragma clang diagnostic pop

	return result;
}

bool8 path_set_current_directory( const char* path ) {
	if ( chdir( path ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to set current directory: %s.\n", strerror( err ) );

		return false;
	}

	return true;
}

#endif // __linux__