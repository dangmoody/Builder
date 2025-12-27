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

#include <paths.h>

#include <core_helpers.h>
#include <temp_storage.h>
#include <typecast.inl>

#include <string.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#endif

static const char* get_last_slash( const char* path ) {
	const char* last_slash = NULL;
	const char* last_back_slash = strrchr( path, '\\' );
	const char* last_forward_slash = strrchr( path, '/' );

	if ( !last_back_slash && !last_forward_slash ) {
		return NULL;
	}

	if ( cast( u64, last_back_slash ) > cast( u64, last_forward_slash ) ) {
		last_slash = last_back_slash;
	} else {
		last_slash = last_forward_slash;
	}

	return last_slash;
}

/*
================================================================================================

	Paths

================================================================================================
*/

const char* path_remove_file_from_path( const char* path ) {
	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		return NULL;
	}

	u64 path_length = cast( u64, last_slash ) - cast( u64, path );

	char* result = cast( char*, mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) ) );
	strncpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	return result;
}

const char* path_remove_path_from_file( const char* path ) {
	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		last_slash = path;
	} else {
		last_slash++;
	}

	return last_slash;
}

const char* path_remove_file_extension( const char* filename ) {
	const char* dot = strrchr( filename, '.' );

	if ( !dot ) {
		return filename;
	}

	u64 result_length = cast( u64, dot ) - cast( u64, filename );

	char* result = cast( char*, mem_temp_alloc( ( result_length + 1 ) * sizeof( char ) ) );
	strncpy( result, filename, result_length * sizeof( char ) );
	result[result_length] = 0;

	return result;
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
