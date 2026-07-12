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

#include <paths.h>

#include <core_helpers.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <core_string.h>
#include <string_builder.h>
#include <core_math.h>
#include <defer.h>

#include <stdarg.h>
#include <string.h>

static bool8 get_last_slash( const String *path, u64 *out_last_slash_pos ) {
	assert( path );

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

String path_remove_file_from_path( const String *path ) {
	assert( path );

	u64 last_slash_pos = 0;
	if ( !get_last_slash( path, &last_slash_pos ) ) {
		return *path;
	}

	return String {
		.data	= path->data,
		.count	= last_slash_pos,
	};
}

String path_remove_path_from_file( const String *path ) {
	assert( path );

	u64 last_slash_pos = 0;
	if ( !get_last_slash( path, &last_slash_pos ) ) {
		return *path;
	}

	last_slash_pos += 1;

	return {
		.data	= path->data + last_slash_pos,
		.count	= path->count - last_slash_pos,
	};
}

String path_remove_file_extension( const String *filename ) {
	assert( filename );

	u64 dot_pos = 0;
	if ( !string_find_from_right( filename, '.', &dot_pos ) ) {
		return *filename;
	}

	return {
		.data	= filename->data,
		.count	= dot_pos
	};
}

static String path_join_internalv( LinearAllocator *allocator, const int count, va_list args ) {
	assert( allocator );

	StringBuilder builder = string_builder_create( allocator );

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
	assert( allocator );

	va_list args;
	va_start( args, count );
	String result = path_join_internalv( allocator, count, args );
	va_end( args );

	return result;
}

String path_relative_path_to( LinearAllocator *allocator, const char *from, const char *to ) {
	assert( from );
	assert( to );

	u64 pos = mem_temp_tell();
	defer { mem_temp_rewind_to( pos ); };

	String from_str = string_set( from );
	String to_str = string_set( to );
	from_str = path_fix_slashes( mem_get_temp_storage(), &from_str );
	to_str = path_fix_slashes( mem_get_temp_storage(), &to_str );

	// determine the directory part of 'from'
	// if the last path segment contains a dot then treat it as a filename and strip it
	// otherwise treat the whole path as a directory and ensure it ends with a slash
	String from_dir;
	{
		u64 last_slash_pos = 0;
		bool8 has_slash = string_find_from_right( &from_str, PATH_SEPARATOR, &last_slash_pos );

		bool8 last_segment_has_dot = false;

		u64 last_segment_start = has_slash ? last_slash_pos + 1 : 0;

		for ( u64 index_in_last_segment = last_segment_start; index_in_last_segment < from_str.count; index_in_last_segment++ ) {
			if ( from_str.data[index_in_last_segment] == '.' ) {
				last_segment_has_dot = true;
				break;
			}
		}

		if ( last_segment_has_dot ) {
			// from_dir = {
			// 	.data  = from_str.data,
			// 	.count = last_slash_pos + 1,
			// };
			from_dir = string_set( from_str.data, last_slash_pos + 1 );
		} else if ( from_str.data[from_str.count - 1] == PATH_SEPARATOR ) {
			from_dir = from_str;
		} else {
			from_dir = string_printf( mem_get_temp_storage(), "%.*s%c", (int) from_str.count, from_str.data, PATH_SEPARATOR );
		}
	}

	// walk both paths simultaneously, recording the end of the last complete
	// segment that matched (i.e. right after a slash)
	u64 common = 0;
	u64 char_index = 0;
	while ( char_index < from_dir.count && char_index < to_str.count && from_dir.data[char_index] == to_str.data[char_index] ) {
		char_index++;
		if ( from_dir.data[char_index - 1] == PATH_SEPARATOR ) {
			common = char_index;
		}
	}

	// if one path ends exactly at a segment boundary in the other then include that boundary
	if ( char_index < from_dir.count && from_dir.data[char_index] == PATH_SEPARATOR && char_index == to_str.count ) {
		common = char_index;
	} else if ( char_index == from_dir.count && char_index < to_str.count && to_str.data[char_index] == PATH_SEPARATOR ) {
		common = char_index;
	} else if ( char_index == from_dir.count && char_index == to_str.count ) {
		common = char_index;
	}

	// count directory segments remaining in from_dir after the common prefix
	// the character at 'common' is the boundary separator itself
	// skip it so we don't count it as an extra level
	u64 count_start = common;
	if ( count_start < from_dir.count && from_dir.data[count_start] == PATH_SEPARATOR ) {
		count_start++;
	}

	u64 num_backs = 0;

	for ( u64 char_pos = count_start; char_pos < from_dir.count; char_pos++ ) {
		if ( from_dir.data[char_pos] == PATH_SEPARATOR ) {
			num_backs++;
		}
	}

	StringBuilder sb = string_builder_create( allocator );

	u64 result_length = 0;

	For ( u64, back_index, 0, num_backs ) {
		bool8 is_last = ( back_index == num_backs - 1 );

		if ( !is_last || common < to_str.count ) {
			string_builder_appendf( &sb, "..%c", PATH_SEPARATOR );
			result_length += 3;
		} else {
			string_builder_appendf( &sb, ".." );
			result_length += 2;
		}
	}

	if ( common < to_str.count ) {
		string_builder_appendf( &sb, "%.*s", (int) ( to_str.count - common ), to_str.data + common );
		result_length += to_str.count - common;
	}

	return {
		.data  = cast( char *, string_builder_to_string( &sb ) ),
		.count = result_length,
	};
}
