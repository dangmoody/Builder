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

		const char* last_slash = strrchr( app_full_path, '/' );
		if ( !last_slash ) last_slash = strrchr( app_full_path, '\\' );
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
	u64 pathLength = strlen( path );

	char* result = cast( char* ) mem_temp_alloc( ( pathLength + 1 ) * sizeof( char ) );
	strncpy( result, path, pathLength * sizeof( char ) );
	result[pathLength] = 0;

	const char* last_slash = strrchr( result, '/' );
	if ( !last_slash ) last_slash = strrchr( result, '\\' );

	if ( last_slash ) {
		u64 offset = cast( u64 ) last_slash - cast( u64 ) result;
		result[offset] = 0;
	} else {
		return path;
	}

	return result;
}

const char* paths_remove_path_from_file( const char* path ) {
	const char* last_slash = strrchr( path, '/' );
	if ( !last_slash ) last_slash = strrchr( path, '\\' );

	if ( !last_slash ) {
		last_slash = path;
	} else {
		last_slash++;
	}

	return last_slash;
}

const char* paths_remove_file_extension( const char* filename ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
	const char* dot = strrchr( filename, '.' );

	if ( dot ) {
		u64 offset = cast( u64 ) dot - cast( u64 ) filename;
		( cast( char* ) filename )[offset] = 0;
	}

	dot = filename;

	return dot;
#pragma clang diagnostic pop
}

bool8 paths_is_path_absolute( const char* path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

#endif // _WIN64