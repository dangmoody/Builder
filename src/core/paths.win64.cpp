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

#endif // _WIN64