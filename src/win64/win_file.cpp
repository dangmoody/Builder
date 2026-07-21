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

#ifdef _WIN32

#include "../file.h"
#include "../file_local.h"

#include "../debug.h"
#include "../defer.h"
#include "../temp_storage.h"
#include "../typecast.h"
#include "../paths.h"
#include "../array.inl"
#include "../string.h"

#include <Windows.h>

/*
================================================================================================

	Win64 File IO implementations

================================================================================================
*/

static file_t FS_OpenFileInternal( const char *filename, const DWORD openFlags, const DWORD creationDisposition ) {
	Assert( filename );

	DWORD fileShareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD flagsAndAttribs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

	HANDLE handle = CreateFileA( filename, openFlags, fileShareFlags, NULL, creationDisposition, flagsAndAttribs, NULL );
	//Assertf( handle != INVALID_HANDLE_VALUE, "Failed to create/open file \"%s\": 0x%X", filename, GetLastError() );

	// TODO: DM: allow setting a logging level for the file system? verbose logging?
	//printf( "%s last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	return { Cast( u64, handle ), 0 };
}

//================================================================

file_t FS_OpenFile( const char *filename, const fileOpenFlags_t openFlags ) {
	Assert( filename );

	DWORD openFlagsWin = 0;
	if ( openFlags & FILE_OPEN_READ )  openFlagsWin |= GENERIC_READ;
	if ( openFlags & FILE_OPEN_WRITE ) openFlagsWin |= GENERIC_WRITE;

	return FS_OpenFileInternal( filename, openFlagsWin, OPEN_EXISTING );
}

file_t FS_OpenOrCreateFile( const char *filename, const bool8 keepExistingContent ) {
	Assert( filename );

	DWORD creationDisposition = ( keepExistingContent ) ? CREATE_NEW : CREATE_ALWAYS;

	DWORD openFlags = GENERIC_READ | GENERIC_WRITE;
	file_t fileHandle = FS_OpenFileInternal( filename, openFlags, creationDisposition );

	if ( fileHandle.handle != INVALID_FILE_HANDLE ) {
		return fileHandle;
	}

	return FS_OpenFileInternal( filename, openFlags, creationDisposition );
}

bool8 FS_CloseFile( file_t *file ) {
	Assert( file );
	Assert( file->handle != INVALID_FILE_HANDLE );

	HANDLE handle = Cast( HANDLE, file->handle );

	BOOL result = CloseHandle( handle );

	//printf( "%s() last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	file->handle = INVALID_FILE_HANDLE;

	return Cast( bool8, result );
}

bool8 FS_ReadFile( file_t *file, const u64 offset, const u64 size, void *outData ) {
	Assert( file && file->handle != INVALID_FILE_HANDLE );
	Assert( outData );

	if ( size == 0 ) {
		return 0;
	}

	HANDLE handle = Cast( HANDLE, file->handle );

	DWORD bytesRead = 0;
	DWORD bytesToRead = Cast( DWORD, size );

	OVERLAPPED overlapped = {};
	overlapped.Offset = Cast( DWORD, offset >> 0 ) & 0xFFFFFFFF;
	overlapped.OffsetHigh = Cast( DWORD, offset >> 32 ) & 0xFFFFFFFF;

	BOOL result = ReadFile( handle, outData, bytesToRead, &bytesRead, &overlapped );

	DWORD lastError = GetLastError();

	if ( !result && ( lastError == ERROR_IO_PENDING ) ) {
		result = GetOverlappedResult( handle, &overlapped, &bytesRead, TRUE );
		if ( !result ) {
			lastError = GetLastError();
			//Assertf( result, "Failed to read from file 0x%x.", GetLastError() );
			return 0;
		}
	}

	if ( !result || bytesRead != bytesToRead ) {
		//Assertf( result, "Failed to read all required data from file 0x%x.", GetLastError() );
		return 0;
	}

	return bytesRead == size;
}

bool8 FS_WriteFile( file_t *file, const void *data, const u64 offset, const u64 size ) {
	Assert( file );
	Assert( file->handle );
	Assert( data );

	if ( size == 0 ) {
		return false;
	}

	HANDLE handle = Cast( HANDLE, file->handle );

	DWORD bytesWritten = 0;
	DWORD bytesToWrite = Cast( DWORD, size );

	OVERLAPPED overlapped = {};
	overlapped.Offset = Cast( DWORD, offset >> 0 ) & 0xFFFFFFFF;
	overlapped.OffsetHigh = Cast( DWORD, offset >> 32 ) & 0xFFFFFFFF;

	BOOL result = WriteFile( handle, Cast( const char *, data ), bytesToWrite, &bytesWritten, &overlapped );

	DWORD lastError = GetLastError();

	if ( !result && ( lastError == ERROR_IO_PENDING ) ) {
		result = GetOverlappedResult( handle, &overlapped, &bytesWritten, TRUE );
		if ( !result ) {
			lastError = GetLastError();
			//Assertf( result, "Failed to write to file 0x%x.", GetLastError() );
			return 0;
		}
	}

	if ( !result || bytesWritten != bytesToWrite ) {
		//Assertf( result, "Failed to write all required data to file 0x%x.", GetLastError() );
		return 0;
	}

	return bytesWritten == size;
}

bool8 FS_DeleteFile( const char *filename ) {
	BOOL result = DeleteFile( filename );
	//Assertf( result, "Failed to delete file %s: 0x%x.", filename, GetLastError() );
	return Cast( bool8, result );
}

bool8 FS_GetFileSize( const char *filename, u64 *outSize ) {
	Assert( filename );
	Assert( outSize );

	file_t file = FS_OpenFileInternal( filename, 0, OPEN_EXISTING );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return false;
	}

	defer { FS_CloseFile( &file ); };

	LARGE_INTEGER largeInt = {};

	if ( !GetFileSizeEx( Cast( HANDLE, file.handle ), &largeInt ) ) {
		return false;
	}

	*outSize = Cast( u64, largeInt.QuadPart );

	return true;
}

bool8 FS_GetFileLastWriteTime( const char *filename, u64 *outLastWriteTime ) {
	Assert( filename );
	Assert( outLastWriteTime );

	file_t file = FS_OpenFileInternal( filename, 0, OPEN_EXISTING );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return false;
	}

	defer { FS_CloseFile( &file ); };

	FILETIME lastWriteTime = {};

	if ( !GetFileTime( Cast( HANDLE, file.handle ), NULL, NULL, &lastWriteTime ) ) {
		return false;
	}

	*outLastWriteTime = ( Cast( u64, lastWriteTime.dwHighDateTime ) << 32 ) | lastWriteTime.dwLowDateTime;

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
		const char* dir = String_Cstr( &directories[dirIndex] );

		dirIndex += 1;

		string_t searchPath = String_Printf( Mem_GetTempStorage(), "%s*", dir );

		WIN32_FIND_DATA findData = {};
		HANDLE handle = FindFirstFile( searchPath.data, &findData );

		if ( handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		while ( 1 ) {
			bool8 isDirectory = Cast( bool8, findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
			// TODO: DM: 10/05/2026: really, this wants to be done via Path_Join
			// but theres a few things that rely on this behaviour
			// so changing this will cause side effects in various places/codebases that use core
			// TODO: AK: 17/07/2026: currently we add a trailing slash to the end of all paths as
			// the file globbing in builder relies on there not being new double slashes in the outputted
			// path portion of the full filename, we should evaluate whether we want this or if it is just
			// here because builder 'demanded' it (my bad gang)
			string_t fullFilename;
			if ( isDirectory ) {
				fullFilename = String_Printf( Mem_GetTempStorage(), "%s%s\\", dir, findData.cFileName );
			} else {
				fullFilename = String_Printf( Mem_GetTempStorage(), "%s%s", dir, findData.cFileName );
			}

			fileInfo_t fileInfo = {
				.sizeBytes			= ( TruncCast( u64, findData.nFileSizeHigh ) << 32 ) | findData.nFileSizeLow,
				.lastWriteTime	= ( TruncCast( u64, findData.ftLastWriteTime.dwHighDateTime ) << 32 ) | findData.ftLastWriteTime.dwLowDateTime,
				.isDirectory		= isDirectory,
				.filename			= findData.cFileName,
				.fullFilename		= fullFilename.data,
			};

			if ( fileInfo.isDirectory ) {
				if ( !String_Equals( findData.cFileName, "." ) && !String_Equals( findData.cFileName, ".." ) ) {
					if ( visitFlags & FILE_VISIT_FOLDERS ) {
						visitCallback( &fileInfo, userData );
					}

					if ( visitFlags & FILE_VISIT_RECURSIVE ) {
						directories.Add( fullFilename );
					}
				}
			} else if ( visitFlags & FILE_VISIT_FILES ) {
				visitCallback( &fileInfo, userData );
			}

			if ( !FindNextFile( handle, &findData ) ) {
				break;
			}
		}

		if ( !FindClose( handle ) ) {
			return false;
		}
	}

	return true;
}

bool8 FS_FileExists( const char *filename ) {
	Assert( filename );

	return GetFileAttributes( filename ) != INVALID_FILE_ATTRIBUTES;
}

bool8 FS_CreateFolderInternal( const char *path ) {
	Assert( path );

	if ( FS_FolderExists( path ) ) {
		return true;
	}

	SECURITY_ATTRIBUTES attributes = {};
	attributes.nLength = sizeof( SECURITY_ATTRIBUTES );

	bool8 result = Cast( bool8, CreateDirectoryA( path, &attributes ) );

	return result;
}

bool8 FS_DeleteFolder( const char *path ) {
	Assert( path );

	bool8 result = Cast( bool8, RemoveDirectoryA( path ) );

	if ( !result ) {
		Error( "Failed to delete folder path \"%s\": 0x%X.\n", path, GetLastError() );
	}

	return result;
}

bool8 FS_FolderExists( const char *path ) {
	Assert( path );

	DWORD attribs = GetFileAttributes( path );

	return ( attribs != INVALID_FILE_ATTRIBUTES ) && ( ( attribs & FILE_ATTRIBUTE_DIRECTORY ) != 0 );
}

#endif // _WIN32
