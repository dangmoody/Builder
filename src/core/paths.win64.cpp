/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef _WIN64

#include "allocation_context.h"
#include "debug.h"

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Shlwapi.h>

static const char* get_last_slash( const char* path ) {
	const char* last_slash = NULL;
	const char* last_back_slash = strrchr( path, '\\' );
	const char* last_forward_slash = strrchr( path, '/' );

	if ( !last_back_slash && !last_forward_slash ) {
		return NULL;
	}

	if ( cast( u64 ) last_back_slash > cast( u64 ) last_forward_slash ) {
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

const char* paths_get_app_path() {
	char* app_full_path = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
	DWORD length = GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
	app_full_path = cast( char* ) paths_remove_file_from_path( app_full_path );
#pragma clang diagnostic pop

	return app_full_path;
}

const char* paths_get_absolute_path( const char* file ) {
	char* absolute_path = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
	GetFullPathName( file, MAX_PATH, absolute_path, NULL );
	return absolute_path;
}

const char* paths_remove_file_from_path( const char* path ) {
	const char* dot = strrchr( path, '.' );

	if ( !dot ) {
		return NULL;
	}

	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		return NULL;
	}

	u64 path_length = cast( u64 ) last_slash - cast( u64 ) path;

	char* result = cast( char* ) mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) );
	strncpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	return result;
}

const char* paths_remove_path_from_file( const char* path ) {
	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		last_slash = path;
	} else {
		last_slash++;
	}

	return last_slash;
}

const char* paths_remove_file_extension( const char* filename ) {
	const char* dot = strrchr( filename, '.' );

	if ( !dot ) {
		return filename;
	}

	u64 result_length = cast( u64 ) dot - cast( u64 ) filename;

	char* result = cast( char* ) mem_temp_alloc( ( result_length + 1 ) * sizeof( char ) );
	strncpy( result, filename, result_length * sizeof( char ) );
	result[result_length] = 0;

	return result;
}

bool8 paths_is_path_absolute( const char* path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

const char* paths_canonicalise_path( const char* path ) {
	const char* path_copy = paths_fix_slashes( path );

	u64 max_path_length = strlen( path_copy ) * sizeof( char );

	char* result = cast( char* ) mem_temp_alloc( max_path_length );
	memset( result, 0, max_path_length );

	BOOL success = PathCanonicalizeA( result, path_copy );
	assert( success );

	return result;
}

const char* paths_fix_slashes( const char* path ) {
	u64 path_length = strlen( path );
	char* result = cast( char* ) mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) );
	memcpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	For ( u64, i, 0, path_length ) {
		if ( result[i] == '/' ) {
			result[i] = '\\';
		}
	}

	return result;
}

#endif // _WIN64