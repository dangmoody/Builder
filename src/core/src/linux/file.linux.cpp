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

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/*
================================================================================================

	Linux File IO implementations

================================================================================================
*/

static bool8 create_folder_internal( const char* path ) {
	int result = mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO );
	int err = errno;

	if ( ( result != 0 ) && ( err != EEXIST ) ) {
		assertf( result == 0, "ERROR: Failed to create directory \"%s\": %s\n", path, strerror( err ) );
	}

	return result == 0;
}

File file_open( const char* filename ) {
	unused( filename );

	return { INVALID_FILE_HANDLE, 0 };
}

File file_open_or_create( const char* filename, const bool8 keep_existing_content ) {
	unused( filename );
	unused( keep_existing_content );

	return { INVALID_FILE_HANDLE, 0 };
}

bool8 file_close( File* file ) {
	unused( file );

	return false;
}

bool8 file_copy( const char* original_path, const char* new_path ) {
	unused( original_path );
	unused( new_path );

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

u64 file_get_size( const File file ) {
	struct stat file_stat = {};
	int result = fstat( trunc_cast( int, file.handle ), &file_stat );
	int err = errno;

	assertf( result == 0, "Failed to get size of file: %s\n", strerror( err ) );

	return trunc_cast( u64, file_stat.st_size );
}

File file_find_first( const char* path, FileInfo* out_file_info ) {
	unused( path );
	unused( out_file_info );

	return { INVALID_FILE_HANDLE, 0 };
}

bool8 file_find_next( File* first_file, FileInfo* out_file_info ) {
	unused( first_file );
	unused( out_file_info );

	return false;
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

	return 0;
}

#endif // __linux__
