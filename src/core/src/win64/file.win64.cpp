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

#ifdef _WIN32

#include <file.h>
#include "../file_local.h"

#include <debug.h>
#include <defer.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <paths.h>
#include <core_array.inl>
#include <core_string.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

/*
================================================================================================

	Win64 File IO implementations

================================================================================================
*/

static File open_file_internal( const char* filename, const DWORD access_flags, const DWORD creation_disposition ) {
	assert( filename );

	DWORD file_share_flags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD flags_and_attribs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

	HANDLE handle = CreateFileA( filename, access_flags, file_share_flags, NULL, creation_disposition, flags_and_attribs, NULL );
	//assertf( handle != INVALID_HANDLE_VALUE, "Failed to create/open file \"%s\": 0x%X", filename, GetLastError() );

	// TODO(DM): allow setting a logging level for the file system? verbose logging?
	//printf( "%s last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	return { cast( u64, handle ), 0 };
}

//================================================================

File file_open( const char* filename ) {
	assert( filename );

	return open_file_internal( filename, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING );
}

File file_open_or_create( const char* filename, const bool8 keep_existing_content ) {
	assert( filename );

	DWORD creation_disposition = ( keep_existing_content ) ? CREATE_NEW : CREATE_ALWAYS;

	//return open_or_create_file_internal( filename, GENERIC_READ | GENERIC_WRITE, creation_disposition );
	DWORD access_flags = GENERIC_READ | GENERIC_WRITE;
	File file_handle = open_file_internal( filename, access_flags, creation_disposition );

	if ( file_handle.handle != INVALID_FILE_HANDLE ) {
		return file_handle;
	}

	return open_file_internal( filename, access_flags, creation_disposition );
}

bool8 file_close( File* file ) {
	assert( file );
	assert( file->handle != INVALID_FILE_HANDLE );

	HANDLE handle = cast( HANDLE, file->handle );

	BOOL result = CloseHandle( handle );

	//printf( "%s() last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	file->handle = INVALID_FILE_HANDLE;

	return cast( bool8, result );
}

bool8 file_copy( const char* original_path, const char* new_path ) {
	assert( original_path );
	assert( new_path );

	BOOL copied = CopyFileA( original_path, new_path, FALSE );
	assertf( copied, "Failed to copy file \"%s\" to \"%s\": 0x%x.", original_path, new_path, GetLastError() );

	return cast( bool8, copied );
}

bool8 file_rename( const char* old_filename, const char* new_filename ) {
	assert( old_filename );
	assert( new_filename );

	bool8 renamed = cast( bool8, MoveFileA( old_filename, new_filename ) );

	assertf( renamed, "Failed to rename file \"%s\" to \"%s\": 0x%x.", old_filename, new_filename, GetLastError() );

	return renamed;
}

bool8 file_read( File* file, const u64 offset, const u64 size, void* out_data ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( out_data );

	if ( size == 0 ) {
		return 0;
	}

	HANDLE handle = cast( HANDLE, file->handle );

	DWORD bytes_read = 0;
	DWORD bytes_to_read = cast( DWORD, size );

	OVERLAPPED overlapped = {};
	overlapped.Offset = cast( DWORD, offset >> 0 ) & 0xFFFFFFFF;
	overlapped.OffsetHigh = cast( DWORD, offset >> 32 ) & 0xFFFFFFFF;

	BOOL result = ReadFile( handle, out_data, bytes_to_read, &bytes_read, &overlapped );

	DWORD last_error = GetLastError();

	if ( !result && ( last_error == ERROR_IO_PENDING ) ) {
		result = GetOverlappedResult( handle, &overlapped, &bytes_read, TRUE );
		if ( !result ) {
			last_error = GetLastError();
			assertf( result, "Failed to read from file 0x%x.", GetLastError() );
			return 0;
		}
	}

	if ( !result || bytes_read != bytes_to_read ) {
		assertf( result, "Failed to read all required data from file 0x%x.", GetLastError() );
		return 0;
	}

	return bytes_read == size;
}

bool8 file_write( File* file, const void* data, const u64 offset, const u64 size ) {
	assert( file );
	assert( file->handle );
	assert( data );

	if ( size == 0 ) {
		return false;
	}

	HANDLE handle = cast( HANDLE, file->handle );

	DWORD bytes_written = 0;
	DWORD bytes_to_write = cast( DWORD, size );

	OVERLAPPED overlapped = {};
	overlapped.Offset = cast( DWORD, offset >> 0 ) & 0xFFFFFFFF;
	overlapped.OffsetHigh = cast( DWORD, offset >> 32 ) & 0xFFFFFFFF;

	BOOL result = WriteFile( handle, cast( const char*, data ), bytes_to_write, &bytes_written, &overlapped );

	DWORD last_error = GetLastError();

	if ( !result && ( last_error == ERROR_IO_PENDING ) ) {
		result = GetOverlappedResult( handle, &overlapped, &bytes_written, TRUE );
		if ( !result ) {
			last_error = GetLastError();
			assertf( result, "Failed to write to file 0x%x.", GetLastError() );
			return 0;
		}
	}

	if ( !result || bytes_written != bytes_to_write ) {
		assertf( result, "Failed to write all required data to file 0x%x.", GetLastError() );
		return 0;
	}

	return bytes_written == size;
}

bool8 file_delete( const char* filename ) {
	BOOL result = DeleteFile( filename );
	assertf( result, "Failed to delete file %s: 0x%x.", filename, GetLastError() );
	return cast( bool8, result );
}

bool8 file_get_size( const char* filename, u64* out_size ) {
	assert( filename );
	assert( out_size );

	File file = open_file_internal( filename, 0, OPEN_EXISTING );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return false;
	}

	defer { file_close( &file ); };

	LARGE_INTEGER large_int = {};

	if ( !GetFileSizeEx( cast( HANDLE, file.handle ), &large_int ) ) {
		return false;
	}

	*out_size = cast( u64, large_int.QuadPart );

	return true;
}

bool8 file_get_last_write_time( const char* filename, u64* out_last_write_time ) {
	assert( filename );
	assert( out_last_write_time );

	File file = open_file_internal( filename, 0, OPEN_EXISTING );

	if ( file.handle == INVALID_FILE_HANDLE ) {
		return false;
	}

	defer { file_close( &file ); };

	FILETIME lastWriteTime = {};

	if ( !GetFileTime( cast( HANDLE, file.handle ), NULL, NULL, &lastWriteTime ) ) {
		return false;
	}

	*out_last_write_time = ( cast( u64, lastWriteTime.dwHighDateTime ) << 32 ) | lastWriteTime.dwLowDateTime;

	return true;
}

bool8 file_get_all_files_in_folder( const char* path, const bool8 recursive, const bool8 visit_folders, FileVisitCallback visit_callback, void* user_data ) {
	assert( path );
	assert( visit_callback );

	Array<const char*> directories;	// TODO(DM): 02/10/2025: allocate this on temp storage
	directories.add( path );

	u32 dir_index = 0;

	while ( dir_index < directories.count ) {
		const char* dir = directories[dir_index];

		dir_index += 1;

		const char* search_path = NULL;
		if ( string_ends_with( dir, "/" ) ) {
			search_path = tprintf( "%s*", dir );
		} else {
			search_path = tprintf( "%s%c*", dir, '/' );
		}

		WIN32_FIND_DATA find_data = {};
		HANDLE handle = FindFirstFile( search_path, &find_data );

		if ( handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		while ( 1 ) {
			FileInfo file_info = {
				.is_directory		= cast( bool8, find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ),
				.last_write_time	= ( trunc_cast( u64, find_data.ftLastWriteTime.dwHighDateTime ) << 32 ) | find_data.ftLastWriteTime.dwLowDateTime,
				.size_bytes			= ( trunc_cast( u64, find_data.nFileSizeHigh ) << 32 ) | find_data.nFileSizeLow,
				.filename			= find_data.cFileName,
				.full_filename		= tprintf( "%s%c%s", dir, '/', file_info.filename ),
			};

			if ( file_info.is_directory ) {
				if ( !string_equals( find_data.cFileName, "." ) && !string_equals( find_data.cFileName, ".." ) ) {
					if ( visit_folders ) {
						visit_callback( &file_info, user_data );
					}

					if ( recursive ) {
						directories.add( file_info.full_filename );
					}
				}
			} else {
				visit_callback( &file_info, user_data );
			}

			if ( !FindNextFile( handle, &find_data ) ) {
				break;
			}
		}

		if ( !FindClose( handle ) ) {
			return false;
		}
	}

	return true;
}

bool8 file_exists( const char* filename ) {
	assert( filename );

	return GetFileAttributes( filename ) != INVALID_FILE_ATTRIBUTES;
}

bool8 create_folder_internal( const char* path ) {
	assert( path );

	if ( folder_exists( path ) ) {
		return true;
	}

	SECURITY_ATTRIBUTES attributes = {};
	attributes.nLength = sizeof( SECURITY_ATTRIBUTES );

	bool8 result = cast( bool8, CreateDirectoryA( path, &attributes ) );

	return result;
}

bool8 folder_delete( const char* path ) {
	assert( path );

	bool8 result = cast( bool8, RemoveDirectoryA( path ) );

	if ( !result ) {
		error( "Failed to delete folder path \"%s\": 0x%X.\n", path, GetLastError() );
	}

	return result;
}

bool8 folder_exists( const char* path ) {
	assert( path );

	DWORD attribs = GetFileAttributes( path );

	return ( attribs != INVALID_FILE_ATTRIBUTES ) && ( ( attribs & FILE_ATTRIBUTE_DIRECTORY ) != 0 );
}

#endif // _WIN32