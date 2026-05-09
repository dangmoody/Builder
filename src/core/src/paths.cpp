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
#include <core_math.h>

#include <stdarg.h>
#include <string.h>

static bool8 get_last_slash( String *path, u64 *out_last_slash_pos ) {
	u64 last_forward_slash = 0;
	u64 last_back_slash = 0;

	if ( !string_find_from_right( path, '/', &last_forward_slash ) && !string_find_from_right( path, '\\', &last_back_slash ) ) {
		return false;
	}

	if ( out_last_slash_pos ) {
		*out_last_slash_pos = max( last_forward_slash, last_back_slash );
	}

	return true;
}

/*
================================================================================================

	Paths

================================================================================================
*/

void path_remove_file_from_path( String *path ) {
	u64 last_slash_pos = 0;
	if ( !get_last_slash( path, &last_slash_pos ) ) {
		return;
	}

	u64 file_length = path->count - last_slash_pos;

	path->count -= file_length;
	path->data[path->count] = 0;
}

void path_remove_path_from_file( String *path ) {
	u64 last_slash_pos = 0;
	if ( !get_last_slash( path, &last_slash_pos ) ) {
		return;
	}

	last_slash_pos += 1;	// want to skip past the last slash

	path->data += last_slash_pos;
	path->count -= last_slash_pos;
}

void path_remove_file_extension( String *filename ) {
	u64 dot_pos = 0;
	if ( !string_find_from_right( filename, '.', &dot_pos ) ) {
		return;
	}

	u64 extension_length = filename->count - dot_pos;

	filename->count -= extension_length;
	filename->data[filename->count] = 0;
}

static String path_join_internalv( LinearAllocator *allocator, const int count, va_list args ) {
	StringBuilder builder = {};
	string_builder_init( &builder, allocator );

	For ( int, arg_index, 0, count ) {
		if ( arg_index > 0 ) {
			string_builder_appendf( &builder, "%c", PATH_SEPARATOR );
		}

		const char *part = va_arg( args, const char * );

		string_builder_appendf( &builder, "%s", part );
	}

	const char *final_path = string_builder_to_string( &builder );

	String str = {
		.data	= cast( char *, final_path ),
		.count	= strlen( final_path ),
	};

	return str;
}

String path_join_internal( LinearAllocator *allocator, const int count, ... ) {
	va_list args;
	va_start( args, count );
	String result = path_join_internalv( allocator, count, args );
	va_end( args );

	return result;
}

// TODO: DM: 09/05/2026: this function probably wants to return a String
// that way we can avoid the horrible StringBuilder hack at the end
char *path_relative_path_to( const char *path_from, const char *path_to ) {
	assert( path_from );
	assert( path_to );

	// TODO: DM: 09/05/2026: this wasnt needed on linux but is on windows
	// why does windows require slashes to be "fixed"?
	// path_from = path_fix_slashes( path_from );
	// path_to   = path_fix_slashes( path_to );
	String path_from2 = string_set( g_temp_storage, path_from );
	String path_to2 = string_set( g_temp_storage, path_to );
	path_fix_slashes( &path_from2 );
	path_fix_slashes( &path_to2 );
	path_from = path_from2.data;
	path_to = path_to2.data;

	u64 num_same_chars = 0;
	u64 num_backs = 0;

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

	For ( u64, back_index, 0, num_backs ) {
		string_builder_appendf( &sb, "..%c", PATH_SEPARATOR );
	}

	string_builder_appendf( &sb, "%s", path_to + num_same_chars );

	return cast( char *, string_builder_to_string( &sb ) );
}
