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

#include "file.h"
#include "file_local.h"

#include "typecast.h"
#include "debug.h"
#include "string.h"

#include <malloc.h>
#include <string.h>

/*
================================================================================================

	Platform-agnostic File IO functions

================================================================================================
*/

void FS_FreeFileBuffer( string_t *buffer ) {
	Assert( buffer );

	free( buffer->data );
	buffer->data = NULL;

	buffer->count = 0;
}

bool8 FS_ReadEntireFile( const char *filename, string_t *outBuffer ) {
	Assert( filename );
	Assert( outBuffer );

	u64 fileSize = 0;
	if ( !FS_GetFileSize( filename, &fileSize ) ) {
		return false;
	}

	file_t file = FS_OpenFile( filename );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return 0;
	}

	char *temp = Cast( char *, malloc( fileSize + 1 ) );

	bool8 read = FS_ReadFile( &file, 0, fileSize, temp );

	FS_CloseFile( &file );

	temp[fileSize] = 0;

	outBuffer->data = temp;
	outBuffer->count = fileSize;

	return read;
}

bool8 FS_ReadFile( file_t *file, const u64 size, void *outData ) {
	bool8 read = FS_ReadFile( file, file->offset, size, outData );

	if ( read ) {
		file->offset += size;
	}

	return read;
}

bool8 FS_WriteEntireFile( const char *filename, const void *data, const u64 size ) {
	Assert( filename );
	Assert( data );

	file_t file = FS_OpenOrCreateFile( filename );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return false;
	}

	if ( !FS_WriteFile( &file, data, 0, size ) ) {
		return false;
	}

	if ( !FS_CloseFile( &file ) ) {
		return false;
	}

	return true;
}

bool8 FS_WriteFile( file_t *file, const void *data, const u64 size ) {
	bool8 written = FS_WriteFile( file, data, file->offset, size );

	if ( written ) {
		file->offset += size;
	}

	return written;
}

bool8 FS_WriteFile( file_t *file, const char *data ) {
	return FS_WriteFile( file, data, strlen( data ) * sizeof( char ) );
}

bool8 FS_CreateFolderIfItDoesntExist( const char *path ) {
	Assert( path );

	if ( FS_FolderExists( path ) ) {
		return true;
	}

	// otherwise create the folder
	{
		bool8 result = false;

		u64 pathLen = strlen( path );

		// dont process trailing slash if one exists
		// otherwise we will get duplicate results for sub-dirs to parse
		//if ( path[pathLen - 1] == '/' ) {
		//	pathLen--;
		//}

		for ( u64 i = 0; i <= pathLen; i++ ) {
#if defined( __linux__ )
			if ( path[i] != '/' && path[i] != '\0') {
				continue;
			}

			// dont process root folder
			if ( i == 0 && path[i] == '/' ) {
				continue;
			}
#elif defined( _WIN32 )
			if ( path[i] != '/' && path[i] != '\0' && path[i] != '\\' ) {
				continue;
			}
#endif

			char name[1024] = {};
			strncpy( name, path, i );

			if ( !FS_FolderExists( name ) ) {
				result |= FS_CreateFolderInternal( name );
			}
		}

		return result;
	}
}
