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
#include "../file_local.h"
#include <paths.h>
#include <core_array.inl>
#include <defer.h>
#include <core_string.h>
#include <temp_storage.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <stdio.h>

/*
================================================================================================

	Linux File IO implementations

================================================================================================
*/

static File open_file_internal( const char *filename, int flags ) {
	assert( filename );

	int handle = open( filename, flags, S_IRWXU | S_IRWXG | S_IRWXO );
	if ( handle == -1 ) {
		return { INVALID_FILE_HANDLE, 0 };
	}

	return { trunc_cast( u64, handle ), 0 };
}

File file_open( const char *filename, const FileOpenFlags open_flags ) {
	assert( filename );

	int open_flags_linux;
	if ( ( open_flags & FILE_OPEN_READ ) && ( open_flags & FILE_OPEN_WRITE ) ) {
		open_flags_linux = O_RDWR;
	} else if ( open_flags & FILE_OPEN_WRITE ) {
		open_flags_linux = O_WRONLY;
	} else {
		open_flags_linux = O_RDONLY;
	}

	return open_file_internal( filename, open_flags_linux );
}

File file_open_or_create( const char *filename, const bool8 keep_existing_content ) {
	assert( filename );

	int flags = O_CREAT | O_RDWR;

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

bool8 file_copy( const char *original_path, const char *new_path ) {
	assert( original_path );
	assert( new_path );

	char *buffer = NULL;
	if ( !file_read_entire( original_path, &buffer ) ) {
		return false;
	}

	defer { file_free_buffer( &buffer ); };

	if ( !file_write_entire( new_path, buffer, strlen( buffer ) ) ) {
		return false;
	}

	return true;
}

bool8 file_rename( const char *old_filename, const char *new_filename ) {
	assert( old_filename );
	assert( new_filename );

	if ( rename( old_filename, new_filename ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to rename file \"%s\" to \"%s\": %s.\n", old_filename, new_filename, strerror( err ) );
		return false;
	}

	return true;
}

bool8 file_read( File* file, const u64 offset, const u64 size, void* out_data ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( size );
	assert( out_data );

	ssize_t bytes_read = pread( trunc_cast( int, file->handle ), out_data, size, trunc_cast( off_t, offset ) );

	return trunc_cast( u64, bytes_read ) == size;
}

bool8 file_write( File* file, const void* data, const u64 offset, const u64 size ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( data );
	assert( size );

	ssize_t bytes_written = pwrite( trunc_cast( int, file->handle ), data, size, trunc_cast( off_t, offset ) );

	return trunc_cast( u64, bytes_written ) == size;
}

bool8 file_delete( const char *filename ) {
	assert( filename );

	int result = remove( filename );

	if ( result != 0 ) {
		int err = errno;
		fatal_error( "Failed to delete file \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 file_get_size( const char *filename, u64 *out_size ) {
	assert( filename );
	assert( out_size );

	struct stat file_stat = {};
	if ( stat( filename, &file_stat ) != 0 ) {
		return false;
	}

	*out_size = trunc_cast( u64, file_stat.st_size );

	return true;
}

bool8 file_get_last_write_time( const char *filename, u64 *out_last_write_time ) {
	assert( filename );
	assert( out_last_write_time );

	struct stat file_stat = {};
	if ( stat( filename, &file_stat ) != 0 ) {
		return false;
	}

	*out_last_write_time = trunc_cast( u64, file_stat.st_mtime );

	return true;
}

bool8 file_get_all_files_in_folder( const char *path, const FileVisitFlags visit_flags, FileVisitCallback visit_callback, void *user_data ) {
	assert( path );
	assert( visit_callback );

	Array<const char *> directories;
	directories.init( g_temp_storage );
	directories.add( path );

	u32 dir_index = 0;

	while ( dir_index < directories.count ) {
		const char *directory = directories[dir_index];

		//printf( "Scanning directory \"%s\"\n", directory );

		DIR *dir = opendir( directory );
		defer { closedir( dir ); };

		dir_index += 1;

		if ( !dir ) {
			int err = errno;
			printf( "Can't open dir \"%s\": %s\n", directory, strerror( err ) );
			return false;
		}

		struct dirent *entry = NULL;
		while ( ( entry = readdir( dir ) ) != NULL ) {
			if ( string_equals( entry->d_name, "." ) || string_equals( entry->d_name, ".." ) ) {
				continue;
			}

			const char *full_filename = path_join( directory, entry->d_name );

			struct stat file_stat = {};
			if ( stat( full_filename, &file_stat ) != 0 ) {
				/*int err = errno;
				printf( "Can't stat \"%s\": %s\n", full_filename, strerror( err ) );*/
				return false;
			}

			FileInfo file_info = {
				.size_bytes			= trunc_cast( u64, file_stat.st_size ),
				.last_write_time	= trunc_cast( u64, file_stat.st_mtime ),
				.is_directory		= S_ISDIR( file_stat.st_mode ),
				.filename			= entry->d_name,
				.full_filename		= full_filename,
			};

			if ( file_info.is_directory ) {
				if ( visit_flags & FILE_VISIT_FOLDERS ) {
					visit_callback( &file_info, user_data );
				}

				if ( visit_flags & FILE_VISIT_RECURSIVE ) {
					directories.add( full_filename );
				}
			} else if ( visit_flags & FILE_VISIT_FILES ) {
				visit_callback( &file_info, user_data );
			}
		}
	}

	return true;
}

bool8 file_exists( const char *filename ) {
	assert( filename );

	return access( filename, 0 ) == 0;
}

bool8 create_folder_internal( const char *path ) {
	int result = mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO );
	int err = errno;

	if ( ( result != 0 ) && ( err != EEXIST ) ) {
		fatal_error( "ERROR: Failed to create directory \"%s\": %s\n", path, strerror( err ) );
	}

	return result == 0;
}

bool8 folder_delete( const char *path ) {
	assert( path );

	int result = rmdir( path );

	if ( result != 0 ) {
		int err = errno;
		fatal_error( "Failed to delete folder \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 folder_exists( const char *path ) {
	assert( path );

	DIR* dir = opendir( path );
	int err = errno;

	if ( dir ) {
		closedir( dir );
		return true;
	} else if ( err == ENOENT ) {
		return false;
	}

	fatal_error( "Failed to check if the folder exists: %s\n", strerror( err ) );

	return false;
}

#endif // __linux__
