/*
===========================================================================

Builder

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

#include "../file.h"
#include "../file_local.h"
#include "../paths.h"
#include "../array.inl"
#include "../defer.h"
#include "../string.h"
#include "../temp_storage.h"

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <cstdio>

/*
================================================================================================

	Linux File IO implementations

================================================================================================
*/

static file_t FS_OpenFileInternal( const char *filename, int flags ) {
	Assert( filename );

	int handle = open( filename, flags, S_IRWXU | S_IRWXG | S_IRWXO );
	if ( handle == -1 ) {
		return { INVALID_FILE_HANDLE, 0 };
	}

	return { TruncCast( u64, handle ), 0 };
}

file_t FS_OpenFile( const char *filename, const fileOpenFlags_t openFlags ) {
	Assert( filename );

	int openFlagsLinux;
	if ( ( openFlags & FILE_OPEN_READ ) && ( openFlags & FILE_OPEN_WRITE ) ) {
		openFlagsLinux = O_RDWR;
	} else if ( openFlags & FILE_OPEN_WRITE ) {
		openFlagsLinux = O_WRONLY;
	} else {
		openFlagsLinux = O_RDONLY;
	}

	return FS_OpenFileInternal( filename, openFlagsLinux );
}

file_t FS_OpenOrCreateFile( const char *filename, const bool8 keepExistingContent ) {
	Assert( filename );

	int flags = O_CREAT | O_RDWR;

	if ( !keepExistingContent ) {
		flags |= O_TRUNC;
	}

	return FS_OpenFileInternal( filename, flags );
}

bool8 FS_CloseFile( file_t* file ) {
	Assert( file );
	Assert( file->handle != INVALID_FILE_HANDLE );

	if ( close( TruncCast( int, file->handle ) ) != 0 ) {
		int err = errno;

		Error( "Failed to close file \"%s\": %s\n", strerror( err ) );

		return false;
	}

	return true;
}

bool8 FS_ReadFile( file_t* file, const u64 offset, const u64 size, void* outData ) {
	Assert( file && file->handle != INVALID_FILE_HANDLE );
	Assert( size );
	Assert( outData );

	ssize_t bytesRead = pread( TruncCast( int, file->handle ), outData, size, TruncCast( off_t, offset ) );

	return TruncCast( u64, bytesRead ) == size;
}

bool8 FS_WriteFile( file_t* file, const void* data, const u64 offset, const u64 size ) {
	Assert( file && file->handle != INVALID_FILE_HANDLE );
	Assert( data );
	Assert( size );

	ssize_t bytesWritten = pwrite( TruncCast( int, file->handle ), data, size, TruncCast( off_t, offset ) );

	return TruncCast( u64, bytesWritten ) == size;
}

bool8 FS_DeleteFile( const char *filename ) {
	Assert( filename );

	int result = remove( filename );

	if ( result != 0 ) {
		int err = errno;
		FatalError( "Failed to delete file \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 FS_GetFileSize( const char *filename, u64 *outSize ) {
	Assert( filename );
	Assert( outSize );

	struct stat fileStat = {};
	if ( stat( filename, &fileStat ) != 0 ) {
		return false;
	}

	*outSize = TruncCast( u64, fileStat.st_size );

	return true;
}

bool8 FS_GetFileLastWriteTime( const char *filename, u64 *outLastWriteTime ) {
	Assert( filename );
	Assert( outLastWriteTime );

	struct stat fileStat = {};
	if ( stat( filename, &fileStat ) != 0 ) {
		return false;
	}

	*outLastWriteTime = TruncCast( u64, fileStat.st_mtime );

	return true;
}

bool8 FS_GetAllFilesInFolder( const char *path, const fileVisitFlags_t visitFlags, fileVisitCallback_t visitCallback, void *userData ) {
	Assert( path );
	Assert( visitCallback );

	// AK: ideally we would take a string_t as a parameter, should we change this function and add a deprecated overload that does:
	// bool8 FS_GetAllFilesInFolder( const char *path... ) { FS_GetAllFilesInFolder( String_Set(path)... ) }
	string_t pathString = String_Set( path );
	if ( !String_EndsWith( &pathString, '\\' ) && !String_EndsWith( &pathString, '/' ) ) {
		pathString = String_Printf( Mem_GetTempStorage(), "%s%c", pathString.data, PATH_SEPARATOR );
	}

	array_t<string_t> directories;
	directories.Init( Mem_GetTempStorage() );
	directories.Add( pathString );

	u32 dirIndex = 0;

	while ( dirIndex < directories.count ) {
		const char *directory = String_Cstr( &directories[dirIndex] );

		//printf( "Scanning directory \"%s\"\n", directory );

		DIR *dir = opendir( directory );
		defer { closedir( dir ); };

		dirIndex += 1;

		if ( !dir ) {
			int err = errno;
			printf( "Can't open dir \"%s\": %s\n", directory, strerror( err ) );
			return false;
		}

		struct dirent *entry = NULL;
		while ( ( entry = readdir( dir ) ) != NULL ) {
			if ( String_Equals( entry->d_name, "." ) || String_Equals( entry->d_name, ".." ) ) {
				continue;
			}
			bool8 isDirectory = entry->d_type == DT_DIR;

			// TODO: AK: 17/07/2026: currently we add a trailing slash to the end of all paths as
			// the file globbing in builder relies on there not being new double slashes in the outputted
			// path portion of the full filename, we should evaluate whether we want this or if it is just
			// here because builder 'demanded' it (my bad gang)
			string_t fullFilename;
			if ( isDirectory ) {
				fullFilename = String_Printf( Mem_GetTempStorage(), "%s%s/", directory, entry->d_name );
			} else {
				fullFilename = String_Printf( Mem_GetTempStorage(), "%s%s", directory, entry->d_name );
			}

			struct stat fileStat = {};
			if ( stat( fullFilename.data, &fileStat ) != 0 ) {
				/*int err = errno;
				printf( "Can't stat \"%s\": %s\n", fullFilename, strerror( err ) );*/
				return false;
			}

			fileInfo_t fileInfo = {
				.sizeBytes			= TruncCast( u64, fileStat.st_size ),
				.lastWriteTime	= TruncCast( u64, fileStat.st_mtime ),
				.isDirectory		= isDirectory,
				.filename			= entry->d_name,
				.fullFilename		= fullFilename.data,
			};

			if ( fileInfo.isDirectory ) {
				if ( visitFlags & FILE_VISIT_FOLDERS ) {
					visitCallback( &fileInfo, userData );
				}

				if ( visitFlags & FILE_VISIT_RECURSIVE ) {
					directories.Add( fullFilename );
				}
			} else if ( visitFlags & FILE_VISIT_FILES ) {
				visitCallback( &fileInfo, userData );
			}
		}
	}

	return true;
}

bool8 FS_FileExists( const char *filename ) {
	Assert( filename );

	return access( filename, 0 ) == 0;
}

bool8 FS_CreateFolderInternal( const char *path ) {
	int result = mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO );
	int err = errno;

	if ( ( result != 0 ) && ( err != EEXIST ) ) {
		FatalError( "ERROR: Failed to create directory \"%s\": %s\n", path, strerror( err ) );
	}

	return result == 0;
}

bool8 FS_DeleteFolder( const char *path ) {
	Assert( path );

	int result = rmdir( path );

	if ( result != 0 ) {
		int err = errno;
		FatalError( "Failed to delete folder \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 FS_FolderExists( const char *path ) {
	Assert( path );

	DIR* dir = opendir( path );
	int err = errno;

	if ( dir ) {
		closedir( dir );
		return true;
	} else if ( err == ENOENT ) {
		return false;
	}

	FatalError( "Failed to check if the folder exists: %s\n", strerror( err ) );

	return false;
}

#endif // __linux__
