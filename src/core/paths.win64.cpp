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
		return path;
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

struct Paths {
	char*	app_path;
};

void paths_init() {
	assert( g_core_context.paths == NULL );

	g_core_context.paths = cast( Paths* ) mem_alloc( sizeof( Paths ) );

	// app path
	{
		char app_full_path[MAX_PATH];

		GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

		const char* last_slash = strrchr( app_full_path, '\\' );
		if ( !last_slash ) last_slash = strrchr( app_full_path, '/' );
		//last_slash++;

		u64 app_base_path_length = cast( u64 ) last_slash - cast( u64 ) app_full_path;

		g_core_context.paths->app_path = cast( char* ) mem_alloc( ( app_base_path_length + 1 ) * sizeof( char ) );

		strncpy( g_core_context.paths->app_path, app_full_path, app_base_path_length );
		g_core_context.paths->app_path[app_base_path_length] = 0;
	}
}

void paths_shutdown() {
	assert( g_core_context.paths );

	// app path
	mem_free( g_core_context.paths->app_path );
	g_core_context.paths->app_path = NULL;

	mem_free( g_core_context.paths );
	g_core_context.paths = NULL;
}

const char* paths_get_app_path() {
	return g_core_context.paths->app_path;
}

const char* paths_get_absolute_path( const char* file ) {
	//char absolute_path[MAX_PATH];
	char* absolute_path = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
	GetFullPathName( file, MAX_PATH, absolute_path, NULL );
	return absolute_path;
}

const char* paths_remove_file_from_path( const char* path ) {
	const char* dot = strrchr( path, '.' );

	if ( !dot ) {
		return path;
	}

	const char* last_slash = get_last_slash( path );

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
	u64 maxPathLength = strlen( path ) * sizeof( char );

	char* result = cast( char* ) mem_temp_alloc( maxPathLength );
	memset( result, 0, maxPathLength );

	BOOL success = PathCanonicalizeA( result, path );
	assert( success );

	return result;
}

#endif // _WIN64