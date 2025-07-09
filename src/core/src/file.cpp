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

#include <file.h>

/*
================================================================================================

	Platform-agnostic File IO functions

================================================================================================
*/

void file_free_buffer( char** buffer ) {
	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	mem_free( *buffer );
	*buffer = NULL;
}

bool8 file_read_entire( const char* filename, char** outBuffer, u64* out_file_length ) {
	assertf( filename, "Specified file name to read from cannot be null." );
	assertf( !*outBuffer, "Specified out-buffer MUST be null because this function news it." );

	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	File file = file_open( filename );

	if ( file.ptr == NULL ) {
		return 0;
	}

	u64 file_size = file_get_size( file );

	char* temp = cast( char*, mem_alloc( file_size + 1 ) );

	bool8 read = file_read( &file, 0, file_size, temp );

	file_close( &file );

	temp[file_size] = 0;

	*outBuffer = temp;

	if ( out_file_length ) {
		*out_file_length = file_size;
	}

	return read;
}

bool8 file_read( File* file, const u64 size, void* out_data ) {
	bool8 read = file_read( file, file->offset, size, out_data );

	if ( read ) {
		file->offset += size;
	}

	return read;
}

bool8 file_write_entire( const char* filename, const void* data, const u64 size ) {
	assertf( filename, "File name cannot be null." );
	assertf( data, "Write data cannot be null." );

	File file = file_open_or_create( filename );

	if ( file.ptr == NULL ) {
		return false;
	}

	defer( file_close( &file ) );

	return file_write( &file, data, 0, size );
}

bool8 file_write( File* file, const void* data, const u64 size ) {
	bool8 written = file_write( file, data, file->offset, size );

	if ( written ) {
		file->offset += size;
	}

	return written;
}

bool8 file_write( File* file, const char* data ) {
	return file_write( file, data, strlen( data ) * sizeof( char ) );
}

bool8 file_write_line( File* file, const char* line ) {
	bool8 main_write = file_write( file, line );
	return main_write && file_write( file, "\n" );
}

bool8 folder_create_if_it_doesnt_exist( const char* path ) {
	assertf( path, "Path cannot be NULL." );

	// if already here, no problem
	if ( folder_exists( path ) ) {
		return true;
	}

	// otherwise create the folder
	{
		bool8 result = false;

		u64 path_len = strlen( path );

		// dont process trailing slash if one exists
		// otherwise we will get duplicate results for sub-dirs to parse
		//if ( path[path_len - 1] == '/' ) {
		//	path_len--;
		//}

		for ( u64 i = 0; i <= path_len; i++ ) {
			if ( path[i] != '/' && path[i] != '\0' && path[i] != '\\' ) {
				continue;
			}

			char name[1024] = {};
			strncpy( name, path, i );

			result |= create_folder_internal( name );
		}

		return result;
	}
}