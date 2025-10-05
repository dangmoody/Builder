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

#pragma once

#include "core_types.h"
#include "dll_export.h"

/*
================================================================================================

	File IO

	Set of basic functions for interacting with Files.  All implementations are OS dependent
	and nothing uses the CRT.  All functions are also thread-safe.

================================================================================================
*/

#define INVALID_FILE_HANDLE U64_MAX

#ifdef __linux__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif // __linux__

struct File {
	u64		handle;
	u64		offset;
};

// TODO(DM): 05/10/2025: support for symlinks
struct FileInfo {
	bool8		is_directory;
	u64			last_write_time;
	u64			size_bytes;
	const char*	filename;
	const char*	full_filename;
};

typedef void ( *FileVisitCallback )( const FileInfo* file_info, void* user_data );


// Opens the file for reading and writing.
CORE_API File	file_open( const char* filename );

// If the file exists then opens it for reading and writing, otherwise creates it and then opens it.
CORE_API File	file_open_or_create( const char* filename, const bool8 keep_existing_content = false );

// Closes the file.
CORE_API bool8	file_close( File* file );

// Copies the file at 'original_path' to 'new_path'.
CORE_API bool8	file_copy( const char* original_path, const char* new_path );

// Renames 'old_filename' to 'new_filename'.
CORE_API bool8	file_rename( const char* old_filename, const char* new_filename );

// Convenience function to free any memory allocated through functions like "file_read_entire".
CORE_API void	file_free_buffer( char** buffer );

// Opens the file, reads the entire contents, stores it in 'out_buffer', and stores the length of the file in 'out_file_length'.
// Returns number of bytes read if successful, otherwise returns 0, the out buffer stays null, and out_file_length doesn't get written to.
// Storing 'out_file_length' is optional.
// Call "file_free_buffer()" to release the memory read into the out buffer.
CORE_API bool8	file_read_entire( const char* filename, char** out_buffer, u64* out_file_length = NULL );

// Returns true if 'size' bytes was successfully read from the file starting from it's current offset and puts the result into 'out_data', otherwise returns false and the out buffer stays null.
CORE_API bool8	file_read( File* file, const u64 size, void* out_data );

// Returns true if 'size' bytes was successfully read from the file starting from 'offset' and puts the result into 'out_data', otherwise returns false and the out buffer stays null.
// Also sets the file's internal offset to 'offset' + 'size' if the read was successful.
CORE_API bool8	file_read( File* file, const u64 offset, const u64 size, void* out_data );

// Writes the specified buffer into the file, overwriting all previous content.
// Returns true if the write was successful, otherwise returns false.
CORE_API bool8	file_write_entire( const char* filename, const void* data, const u64 size );

// Writes the specified buffer into 'file' starting from the file's current offset.
// Returns true if the write was successful and increases the file's offset by 'size' bytes, otherwise returns false.
CORE_API bool8	file_write( File* file, const void* data, const u64 size );

// writes the specified string into 'file' starting from the file's current offset.
// Returns true if the write was succesful and increases the file's offset by the length of the string, otherwise returns false.
CORE_API bool8	file_write( File* file, const char* data );

// Writes the specified buffer into the file at the specified offset.
// Returns true if the write was successful, otherwise returns false.
CORE_API bool8	file_write( File* file, const void* data, const u64 offset, const u64 size );

// Writes a string to the file and gives you a new line afterwards
CORE_API bool8	file_write_line(File* file, const char* line);

// Returns true if successfully deletes the file, otherwise returns false.
CORE_API bool8	file_delete( const char* filename );

// Returns true and fills out_file_info if the file can be found, otherwise returns false.
CORE_API bool8	file_get_info( const char* filename, FileInfo* out_file_info );

// Returns true if all files found in path can be successfully visited, otherwise returns false.
// For each file found, 'visit_callback' gets called.
// If 'visit_folders' is true then 'visit_callback' will also fire for each folder that gets visited.
// 'user_data' can be NULL.
CORE_API bool8	file_get_all_files_in_folder( const char* path, const bool8 recursive, const bool8 visit_folders, FileVisitCallback visit_callback, void* user_data );

// If the folder at the given path already exists then returns true.
// If the folder at the given path does NOT exist but was successfully created then returns true.
// Otherwise returns false because the folder did NOT previously exist and could not be created.
CORE_API bool8	folder_create_if_it_doesnt_exist( const char* path );

// Returns true if the given folder path exists and could be deleted, otherwise returns false.
CORE_API bool8	folder_delete( const char* path );

// Returns true if the given folder path exists, otherwise returns false.
CORE_API bool8	folder_exists( const char* path );

// Returns the number of files that are found in 'path'.
// TODO(DM): add recursive search
CORE_API u64	folder_get_num_files( const char* path );

#ifdef __linux__
#pragma clang diagnostic pop
#endif //__linux__
