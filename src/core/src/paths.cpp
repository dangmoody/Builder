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
#include <core_string.h>
#include <string_builder.h>
#include <defer.h>

#include <stdarg.h>
#include <string.h>

static const char *get_last_slash( const char *path ) {
	const char *last_slash = NULL;
	const char *last_back_slash = strrchr( path, '\\' );
	const char *last_forward_slash = strrchr( path, '/' );

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

const char *path_remove_file_from_path( const char *path ) {
	const char *last_slash = get_last_slash( path );

	if ( !last_slash ) {
		return NULL;
	}

	u64 path_length = cast( u64, last_slash ) - cast( u64, path );

	return temp_c_string( path, path_length );
}

const char *path_remove_path_from_file( const char *path ) {
	const char *last_slash = get_last_slash( path );

	if ( !last_slash ) {
		last_slash = path;
	} else {
		last_slash++;
	}

	return last_slash;
}

const char *path_remove_file_extension( const char *filename ) {
	const char *dot = strrchr( filename, '.' );

	if ( !dot ) {
		return filename;
	}

	u64 result_length = cast( u64, dot ) - cast( u64, filename );

	return temp_c_string( filename, result_length );
}

static const char *path_join_internalv( const int count, va_list args ) {
	StringBuilder builder = {};
	string_builder_init( &builder, g_temp_storage );

	For ( int, arg_index, 0, count ) {
		if ( arg_index > 0 ) {
			string_builder_appendf( &builder, "%c", PATH_SEPARATOR );
		}

		const char *part = va_arg( args, const char * );

		string_builder_appendf( &builder, part );
	}

	return string_builder_to_string( &builder );
}

const char *path_join_internal( const int count, ... ) {
	va_list args;
	va_start( args, count );
	const char *result = path_join_internalv( count, args );
	va_end( args );

	return result;
}

char *path_relative_path_to( const char *path_from, const char *path_to ) {
	assert( path_from );
	assert( path_to );

	path_from = path_fix_slashes( path_from );
	path_to   = path_fix_slashes( path_to );

	u32 num_same_chars = 0;
	u32 num_backs = 0;

	while ( path_from[num_same_chars] && path_to[num_same_chars] && path_from[num_same_chars] == path_to[num_same_chars] ) {
		num_same_chars += 1;
	}

	while ( num_same_chars > 0 && path_from[num_same_chars - 1] != PATH_SEPARATOR ) {
		num_same_chars -= 1;
	}

	const char *path_from_copy = path_from + num_same_chars;

	while ( *path_from_copy ) {
		if ( *path_from_copy == PATH_SEPARATOR ) {
			num_backs += 1;
		}
		path_from_copy += 1;
	}

	StringBuilder sb = {};
	string_builder_init( &sb, g_temp_storage );

	For ( u32, back_index, 0, num_backs ) {
		string_builder_appendf( &sb, "..%c", PATH_SEPARATOR );
	}

	string_builder_appendf( &sb, path_to + num_same_chars );

	return cast( char *, string_builder_to_string( &sb ) );
}
