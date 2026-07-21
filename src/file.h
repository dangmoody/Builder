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

struct string_t;

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

struct file_t {
	u64		handle;
	u64		offset;
};

enum fileVisitFlagBits_t {
	FILE_VISIT_RECURSIVE	= BIT( 0 ),
	FILE_VISIT_FILES		= BIT( 1 ),
	FILE_VISIT_FOLDERS		= BIT( 2 ),
};
typedef u32 fileVisitFlags_t;

enum fileOpenFlagBits_t {
	FILE_OPEN_READ	= BIT( 0 ),
	FILE_OPEN_WRITE	= BIT( 1 ),
};
typedef u32 fileOpenFlags_t;

// TODO: DM: 05/10/2025: support for symlinks
struct fileInfo_t {
	u64			sizeBytes;
	u64			lastWriteTime;
	bool8		isDirectory;
	const char	*filename;
	const char	*fullFilename;
};

typedef void ( *fileVisitCallback_t )( const fileInfo_t *fileInfo, void *userData );


// Opens the file with the specified access flags.
file_t	FS_OpenFile( const char *filename, const fileOpenFlags_t openFlags = FILE_OPEN_READ | FILE_OPEN_WRITE );

// If the file exists then opens it for reading and writing, otherwise creates it and then opens it.
file_t	FS_OpenOrCreateFile( const char *filename, const bool8 keepExistingContent = false );

// Closes the file.
bool8	FS_CloseFile( file_t *file );

// Convenience function to free any memory allocated through functions like "FS_ReadEntireFile".
void	FS_FreeFileBuffer( string_t *buffer );

// Opens the file, reads the entire contents, stores it in 'outBuffer', and stores the length of the file in 'outFileLength'.
// Returns number of bytes read if successful, otherwise returns 0, the out buffer stays null, and outFileLength doesn't get written to.
// Storing 'outFileLength' is optional.
// Call "FS_FreeFileBuffer()" to release the memory read into the out buffer.
bool8	FS_ReadEntireFile( const char *filename, string_t *outBuffer );

// Returns true if 'size' bytes was successfully read from the file starting from it's current offset and puts the result into 'outData', otherwise returns false and the out buffer stays null.
bool8	FS_ReadFile( file_t *file, const u64 size, void *outData );

// Returns true if 'size' bytes was successfully read from the file starting from 'offset' and puts the result into 'outData', otherwise returns false and the out buffer stays null.
// Also sets the file's internal offset to 'offset' + 'size' if the read was successful.
bool8	FS_ReadFile( file_t *file, const u64 offset, const u64 size, void* outData );

// Writes the specified buffer into the file, overwriting all previous content.
// Returns true if the write was successful, otherwise returns false.
bool8	FS_WriteEntireFile( const char *filename, const void *data, const u64 size );

// Writes the specified buffer into 'file' starting from the file's current offset.
// Returns true if the write was successful and increases the file's offset by 'size' bytes, otherwise returns false.
bool8	FS_WriteFile( file_t *file, const void *data, const u64 size );

// writes the specified string into 'file' starting from the file's current offset.
// Returns true if the write was succesful and increases the file's offset by the length of the string, otherwise returns false.
bool8	FS_WriteFile( file_t *file, const char *data );

// Writes the specified buffer into the file at the specified offset.
// Returns true if the write was successful, otherwise returns false.
bool8	FS_WriteFile( file_t *file, const void *data, const u64 offset, const u64 size );

// Returns true if successfully deletes the file, otherwise returns false.
bool8	FS_DeleteFile( const char *filename );

// If the file exists sets 'outSize' to the size of the file and returns true, otherwise returns false.
bool8	FS_GetFileSize( const char *filename, u64 *outSize );

// If the file exists sets 'outLastWriteTime' to the timestamp of when the file was last written to and returns true, otherwise returns false.
bool8	FS_GetFileLastWriteTime( const char *filename, u64 *outLastWriteTime );

// Returns true if all files found in path can be successfully visited, otherwise returns false.
// For each file found, 'visitCallback' gets called.
// If 'visitFolders' is true then 'visitCallback' will also fire for each folder that gets visited.
// 'userData' can be NULL.
bool8	FS_GetAllFilesInFolder( const char *path, const fileVisitFlags_t visitFlags, fileVisitCallback_t visitCallback, void *userData );

// Returns true if the file actually exists on the file system, otherwise returns false.
bool8	FS_FileExists( const char* filename );

// If the folder at the given path already exists then returns true.
// If the folder at the given path does NOT exist but was successfully created then returns true.
// Otherwise returns false because the folder did NOT previously exist and could not be created.
bool8	FS_CreateFolderIfItDoesntExist( const char *path );

// Returns true if the given folder path exists and could be deleted, otherwise returns false.
bool8	FS_DeleteFolder( const char *path );

// Returns true if the given folder path exists, otherwise returns false.
bool8	FS_FolderExists( const char *path );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
