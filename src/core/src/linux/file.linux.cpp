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

#include <file.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/*
================================================================================================

	Linux File IO implementations

================================================================================================
*/

static File open_file_internal( const char* filename, int flags ) {
	assert( filename );

	int handle = open( filename, flags );
	if ( handle == -1 ) {
		int err = errno;

		error( "Failed to open file \"%s\": %s\n", filename, strerror( err ) );

		return { INVALID_FILE_HANDLE, 0 };
	}

	return { trunc_cast( u64, handle ), 0 };
}

static bool8 create_folder_internal( const char* path ) {
	int result = mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO );
	int err = errno;

	if ( ( result != 0 ) && ( err != EEXIST ) ) {
		assertf( result == 0, "ERROR: Failed to create directory \"%s\": %s\n", path, strerror( err ) );
	}

	return result == 0;
}

File file_open( const char* filename ) {
	assert( filename );

	return open_file_internal( filename, 0 );
}

File file_open_or_create( const char* filename, const bool8 keep_existing_content ) {
	assert( filename );

	int flags = O_CREAT;

	if ( !keep_existing_content ) {
		flags |= O_TRUNC;
	}

	return open_file_internal( filename, flags );
}

bool8 file_close( File* file ) {
	assert( file );
	assert( file->handle != INVALID_FILE_HANDLE );

	if ( close( trunc_cast( int, file->handle ) ) != 0 ) {
		int err = errno;

		error( "Failed to close file \"%s\": %s\n", strerror( err ) );

		return false;
	}

	return true;
}

bool8 file_copy( const char* original_path, const char* new_path ) {
	unused( original_path );
	unused( new_path );

	assert( false );
	
	return false;
}

bool8 file_rename( const char* old_filename, const char* new_filename ) {
	assert( old_filename );
	assert( new_filename );

	int result = rename( old_filename, new_filename );
	int err = errno;

	assertf( result, "Failed to rename file \"%s\" to \"%s\": %s.\n", strerror( err ) );

	return result == 0;
}

bool8 file_read( File* file, const u64 offset, const u64 size, void* out_data ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( size );
	assert( out_data );

	ssize_t bytes_read = pread( trunc_cast( int, file->handle ), out_data, size, trunc_cast( off_t, offset ) );
	int err = errno;

	assertf( trunc_cast( u64, bytes_read ) == size, "Failed to read %llu bytes of file at offset %llu: %s.\n", size, offset, strerror( err ) );

	return trunc_cast( u64, bytes_read ) == size;
}

bool8 file_write( File* file, const void* data, const u64 offset, const u64 size ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( data );
	assert( size );

	ssize_t bytes_written = pwrite( trunc_cast( int, file->handle ), data, size, trunc_cast( off_t, offset ) );
	int err = errno;

	assertf( trunc_cast( u64, bytes_written ) == size, "Failed to write %llu bytes of file at offset %llu: %s.\n", size, offset, strerror( err ) );

	return trunc_cast( u64, bytes_written ) == size;
}

bool8 file_delete( const char* filename ) {
	assert( filename );

	int result = remove( filename );
	int err = errno;

	assertf( result == 0, "Failed to delete file \"%s\": %s.\n", strerror( err ) );

	return result == 0;
}

bool8 file_get_info( const char* filename, FileInfo* out_file_info ) {
	assert( filename );
	assert( out_file_info );

	struct stat file_stat = {};
	if ( !stat( entry->d_name, &file_stat ) ) {
		return false;
	}

	*out_file_info = FileInfo {
		.is_directory		= S_ISDIR( file_stat.st_mode ),
		.last_write_time	= file_stat.st_mtime,
		.size_bytes			= file_stat.st_size,
		.filename			= entry->d_name,
	};

	return true;
}

bool8 file_visit( const char* path, FileVisitCallback file_visit_callback ) {
	assert( path );
	assert( file_visit_callback );

	DIR* dir = opendir( path );

	if ( !dir ) {
		return false;
	}

	struct dirent* entry = NULL;
	while ( ( entry = readdir( dir ) ) != NULL ) {
		if ( string_equals( entry->d_name, "." ) || string_equals( entry->d_name, ".." ) ) {
			continue;
		}

		FileInfo file_info = {};

		if ( !file_get_info( entry->d_name, &file_info ) ) {
			return false;
		}

		file_visit_callback( &file_info );
	}

	if ( !closedir( dir ) ) {
		return false;
	}

	return true;
}

bool8 folder_delete( const char* path ) {
	assert( path );

	int result = rmdir( path );
	int err = errno;

	assertf( result == 0, "Failed to delete folder \"%s\": %s.\n", strerror( err ) );

	return result == 0;
}

bool8 folder_exists( const char* path ) {
	assert( path );

	DIR* dir = opendir( path );
	int err = errno;

	if ( dir ) {
		closedir( dir );
		return true;
	} else if ( err == ENOENT ) {
		return false;
	}

	assertf( false, "Failed to check if the folder exists: %s\n", strerror( err ) );
	return false;
}

// TODO(DM): 09/07/2025: do we still want this?
u64 folder_get_num_files( const char* path ) {
	unused( path );

	assert( false );

	return 0;
}

#endif // __linux__
