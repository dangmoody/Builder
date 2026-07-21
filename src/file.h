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

#pragma once

#include "int_types.h"

struct String;

/*
================================================================================================

	File IO

	Set of functions for manipulating files and folders.  All implementations are OS dependent.

================================================================================================
*/

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wpadded"
#endif

#define INVALID_FILE_HANDLE U64_MAX

struct File {
	u64		handle;
	u64		offset;
};

enum FileVisitFlagBits {
	FILE_VISIT_RECURSIVE	= bit( 0 ),
	FILE_VISIT_FILES		= bit( 1 ),
	FILE_VISIT_FOLDERS		= bit( 2 ),
};
typedef u32 FileVisitFlags;

enum FileOpenFlagBits {
	FILE_OPEN_READ	= bit( 0 ),
	FILE_OPEN_WRITE	= bit( 1 ),
};
typedef u32 FileOpenFlags;

// TODO: DM: 05/10/2025: support for symlinks
struct FileInfo {
	u64			size_bytes;
	u64			last_write_time;
	bool8		is_directory;
	const char	*filename;
	const char	*full_filename;
};

typedef void ( *FileVisitCallback )( const FileInfo *file_info, void *user_data );


// Opens the file with the specified access flags.
File	file_open( const char *filename, const FileOpenFlags open_flags = FILE_OPEN_READ | FILE_OPEN_WRITE );

// If the file exists then opens it for reading and writing, otherwise creates it and then opens it.
File	file_open_or_create( const char *filename, const bool8 keep_existing_content = false );

// Closes the file.
bool8	file_close( File *file );

// Convenience function to free any memory allocated through functions like "file_read_entire".
void	file_free_buffer( String *buffer );

// Opens the file, reads the entire contents, stores it in 'out_buffer', and stores the length of the file in 'out_file_length'.
// Returns number of bytes read if successful, otherwise returns 0, the out buffer stays null, and out_file_length doesn't get written to.
// Storing 'out_file_length' is optional.
// Call "file_free_buffer()" to release the memory read into the out buffer.
bool8	file_read_entire( const char *filename, String *out_buffer );

// Returns true if 'size' bytes was successfully read from the file starting from it's current offset and puts the result into 'out_data', otherwise returns false and the out buffer stays null.
bool8	file_read( File *file, const u64 size, void *out_data );

// Returns true if 'size' bytes was successfully read from the file starting from 'offset' and puts the result into 'out_data', otherwise returns false and the out buffer stays null.
// Also sets the file's internal offset to 'offset' + 'size' if the read was successful.
bool8	file_read( File *file, const u64 offset, const u64 size, void* out_data );

// Writes the specified buffer into the file, overwriting all previous content.
// Returns true if the write was successful, otherwise returns false.
bool8	file_write_entire( const char *filename, const void *data, const u64 size );

// Writes the specified buffer into 'file' starting from the file's current offset.
// Returns true if the write was successful and increases the file's offset by 'size' bytes, otherwise returns false.
bool8	file_write( File *file, const void *data, const u64 size );

// writes the specified string into 'file' starting from the file's current offset.
// Returns true if the write was succesful and increases the file's offset by the length of the string, otherwise returns false.
bool8	file_write( File *file, const char *data );

// Writes the specified buffer into the file at the specified offset.
// Returns true if the write was successful, otherwise returns false.
bool8	file_write( File *file, const void *data, const u64 offset, const u64 size );

// Returns true if successfully deletes the file, otherwise returns false.
bool8	file_delete( const char *filename );

// If the file exists sets 'out_size' to the size of the file and returns true, otherwise returns false.
bool8	file_get_size( const char *filename, u64 *out_size );

// If the file exists sets 'out_last_write_time' to the timestamp of when the file was last written to and returns true, otherwise returns false.
bool8	file_get_last_write_time( const char *filename, u64 *out_last_write_time );

// Returns true if all files found in path can be successfully visited, otherwise returns false.
// For each file found, 'visit_callback' gets called.
// If 'visit_folders' is true then 'visit_callback' will also fire for each folder that gets visited.
// 'user_data' can be NULL.
bool8	file_get_all_files_in_folder( const char *path, const FileVisitFlags visit_flags, FileVisitCallback visit_callback, void *user_data );

// Returns true if the file actually exists on the file system, otherwise returns false.
bool8	file_exists( const char* filename );

// If the folder at the given path already exists then returns true.
// If the folder at the given path does NOT exist but was successfully created then returns true.
// Otherwise returns false because the folder did NOT previously exist and could not be created.
bool8	folder_create_if_it_doesnt_exist( const char *path );

// Returns true if the given folder path exists and could be deleted, otherwise returns false.
bool8	folder_delete( const char *path );

// Returns true if the given folder path exists, otherwise returns false.
bool8	folder_exists( const char *path );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
