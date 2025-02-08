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

#ifdef _WIN64

#include <file.h>

#include <debug.h>
#include <allocation_context.h>
#include <defer.h>
#include <temp_storage.h>
#include <typecast.inl>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
================================================================================================

	File

================================================================================================
*/

static File open_file_internal( const char* filename, const DWORD access_flags, const DWORD creation_disposition ) {
	assertf( filename, "Null file name was specified!" );

	DWORD file_share_flags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD flags_and_attribs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

	HANDLE handle = CreateFileA( filename, access_flags, file_share_flags, NULL, creation_disposition, flags_and_attribs, NULL );
	//assertf( handle != INVALID_HANDLE_VALUE, "Failed to create/open file \"%s\": 0x%X", filename, GetLastError() );

	// TODO(DM): allow setting a logging level for the file system? verbose logging?
	//printf( "%s last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	return { handle, 0 };
}

//static File open_or_create_file_internal( const char* filename, const DWORD access_flags, const DWORD creation_disposition ) {
//	assert( filename );
//
//	File file_handle = open_file_internal( filename, access_flags, creation_disposition );
//
//	if ( file_handle.ptr != INVALID_HANDLE_VALUE ) {
//		return file_handle;
//	}
//
//	return open_file_internal( filename, access_flags, creation_disposition );
//}

static bool8 create_folder_internal( const char* path ) {
	if ( folder_exists( path ) ) {
		return true;
	}

	bool8 result = false;

#if 0
	SECURITY_ATTRIBUTES secattr = {};
	result = (bool8) CreateDirectory( path, &secattr );

	if ( !result ) {
		printf( "WIN32: Failed creating folder \"%s\": %lu\n", path, GetLastError() );
	}
#else
	SECURITY_ATTRIBUTES attributes = {};
	attributes.nLength = sizeof( SECURITY_ATTRIBUTES );

	result = cast( bool8, CreateDirectoryA( path, &attributes ) );
#endif

	return result;
}

//================================================================

File file_open( const char* filename ) {
	return open_file_internal( filename, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING );
}

File file_open_or_create( const char* filename, const bool8 keep_existing_content ) {
	DWORD creation_disposition = ( keep_existing_content ) ? CREATE_NEW : CREATE_ALWAYS;

	//return open_or_create_file_internal( filename, GENERIC_READ | GENERIC_WRITE, creation_disposition );
	DWORD access_flags = GENERIC_READ | GENERIC_WRITE;
	File file_handle = open_file_internal( filename, access_flags, creation_disposition );

	if ( file_handle.ptr != INVALID_HANDLE_VALUE ) {
		return file_handle;
	}

	return open_file_internal( filename, access_flags, creation_disposition );
}

bool8 file_close( File* file ) {
	assertf( file, "The file->ptr can be NULL, but the pointer to the handle MUST be non-NULL." );

	HANDLE handle = cast( HANDLE, file->ptr );

	BOOL result = CloseHandle( handle );

	//printf( "%s() last error: 0x%08X\n", __FUNCTION__, GetLastError() );

	file->ptr = NULL;
	file = NULL;

	return cast( bool8, result );
}

bool8 file_copy( const char* original_path, const char* new_path ) {
	assertf( original_path, "Original filename cannot be null.\n" );
	assertf( new_path, "New filename cannot be null.\n" );

	BOOL copied = CopyFileA( original_path, new_path, FALSE );
	assertf( copied, "Failed to copy file \"%s\" to \"%s\": 0x%x.", original_path, new_path, GetLastError() );

	return cast( bool8, copied );
}

bool8 file_rename( const char* old_filename, const char* new_filename ) {
	assertf( old_filename, "Old filename cannot be NULL." );
	assertf( new_filename, "New filename cannot be NULL." );

	bool8 renamed = cast( bool8, MoveFileA( old_filename, new_filename ) );

	assertf( renamed, "Failed to rename file \"%s\" to \"%s\": 0x%x.", old_filename, new_filename, GetLastError() );

	return renamed;
}

void file_free_buffer( char** buffer ) {
	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	mem_free( *buffer );
	*buffer = NULL;
}

bool8 file_read_entire( const char* filename, char** outBuffer, u64* out_file_length ) {
	assertf( filename, "Specified file name to read from cannot be null." );
	assertf( !*outBuffer, "Specified out-buffer MUST be null because this function news it." );

	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	File file = file_open( filename );

	if ( file.ptr == NULL ) {
		return 0;
	}

	u64 file_size = file_get_size( file );

	char* temp = cast( char*, mem_alloc( file_size + 1 ) );

	bool8 read = file_read( &file, 0, file_size, temp );

	file_close( &file );

	temp[file_size] = 0;

	*outBuffer = temp;

	if ( out_file_length ) {
		*out_file_length = file_size;
	}

	return read;
}

bool8 file_read( File* file, const u64 size, void* out_data ) {
	bool8 read = file_read( file, file->offset, size, out_data );

	if ( read ) {
		file->offset += size;
	}

	return read;
}

bool8 file_read( File* file, const u64 offset, const u64 size, void* out_data ) {
	assertf( file && file->ptr, "File cannot be null." );
	assertf( out_data, "Out data cannot be null." );

	if ( size == 0 ) {
		return 0;
	}

	HANDLE handle = cast( HANDLE, file->ptr );

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

bool8 file_write_entire( const char* filename, const void* data, const u64 size ) {
	assertf( filename, "File name cannot be null." );
	assertf( data, "Write data cannot be null." );

	File file = {};

	// we want to wipe any pre-existing content in this file (if it exists) and overwrite it with the new content
	// so check that it exists first and if it does then delete it and re-create it
	file = open_file_internal( filename, GENERIC_READ, OPEN_EXISTING );

	if ( file.ptr ) {
		bool8 closed = file_close( &file );
		assert( closed );
		unused( closed );

		bool8 deleted = file_delete( filename );
		assert( deleted );
		unused( deleted );
	}

	// now write our new content to it
	file = file_open_or_create( filename );

	if ( file.ptr == NULL ) {
		return false;
	}

	defer( file_close( &file ) );

	return file_write( &file, data, 0, size );
}

bool8 file_write( File* file, const void* data, const u64 size ) {
	bool8 written = file_write( file, data, file->offset, size );

	if ( written ) {
		file->offset += size;
	}

	return written;
}

bool8 file_write( File* file, const char* data ) {
	return file_write( file, data, strlen( data ) * sizeof( char ) );
}

bool8 file_write_line( File* file, const char* line ) {
	bool8 main_write = file_write( file, line );
	return main_write && file_write( file, "\n" );
}

bool8 file_write( File* file, const void* data, const u64 offset, const u64 size ) {
	assertf( file && file->ptr, "File cannot be null." );
	assertf( data, "Write data cannot be null." );

	if ( size == 0 ) {
		return false;
	}

	HANDLE handle = cast( HANDLE, file->ptr );

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

u64 file_get_size( const File file ) {
	if ( file.ptr == INVALID_HANDLE_VALUE ) {
		return 0;
	}

	HANDLE handle = cast( HANDLE, file.ptr );

	LARGE_INTEGER large_int = {};
	BOOL got_file_size = GetFileSizeEx( handle, &large_int );

	assertf( got_file_size, "Failed to get size for file: 0x%x.", GetLastError() );
	unused( got_file_size );

	return cast( u64, large_int.QuadPart );
}

static void get_file_info_internal( const WIN32_FIND_DATA* find_data, FileInfo* out_file_info ) {
	assert( find_data );
	assert( out_file_info );

	out_file_info->is_directory = ( find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );

	out_file_info->last_write_time = ( cast( u64, find_data->ftLastWriteTime.dwHighDateTime ) << 32 ) | find_data->ftLastWriteTime.dwLowDateTime;

	strncpy( out_file_info->filename, find_data->cFileName, MAX_PATH );
}

File file_find_first( const char* path, FileInfo* out_file_info ) {
	assertf( path, "Path cannot be null." );
	assertf( out_file_info, "Out file info cannot be null." );

	WIN32_FIND_DATA info = {};
	HANDLE handle = FindFirstFile( path, &info );

	//assertf( handle != INVALID_HANDLE_VALUE, "Failed to find files at path \"%s\": 0x%X.", path, GetLastError() );

	get_file_info_internal( &info, out_file_info );

	return { handle, 0 };
}

bool8 file_find_next( File* first_file, FileInfo* out_file_info ) {
	assertf( first_file && first_file->ptr, "first_file cannot be invalid." );
	assertf( out_file_info, "Out file info cannot be null." );

	HANDLE handle = cast( HANDLE, first_file->ptr );

	WIN32_FIND_DATA info = {};
	bool8 found = cast( bool8, FindNextFile( handle, &info ) );

	if ( !found ) {
		// TODO(DM): switch/case this for other errors if we need to account for others
		DWORD lastError = GetLastError();
		if ( lastError != ERROR_NO_MORE_FILES && lastError != ERROR_INVALID_HANDLE ) {
			assertf( found, "Failed to get next file from first file: 0x%X.", lastError );
		}

		return false;
	}

	get_file_info_internal( &info, out_file_info );

	return true;
}

bool8 folder_create_if_it_doesnt_exist( const char* path ) {
	assertf( path, "Path cannot be NULL." );

	// if already here, no problem
	if ( folder_exists( path ) ) {
		return true;
	}

	// otherwise create the folder
	{
		bool8 result = false;

		u64 path_len = strlen( path );

		// dont process trailing slash if one exists
		// otherwise we will get duplicate results for sub-dirs to parse
		//if ( path[path_len - 1] == '/' ) {
		//	path_len--;
		//}

		for ( u64 i = 0; i <= path_len; i++ ) {
			if ( path[i] != '/' && path[i] != '\0' && path[i] != '\\' ) {
				continue;
			}

			char name[1024] = {};
			strncpy( name, path, i );

			result |= create_folder_internal( name );
		}

		return result;
	}
}

bool8 folder_delete( const char* path ) {
	assertf( path, "Path cannot be NULL." );

	bool8 result = cast( bool8, RemoveDirectoryA( path ) );

	if ( !result ) {
		error( "Failed to delete folder path \"%s\": 0x%X.\n", path, GetLastError() );
	}

	return result;
}

bool8 folder_exists( const char* path ) {
	assertf( path, "Path cannot be NULL." );

	DWORD attribs = GetFileAttributes( path );

	return ( attribs != INVALID_FILE_ATTRIBUTES ) && ( ( attribs & FILE_ATTRIBUTE_DIRECTORY ) != 0 );
}

u64 folder_get_num_files( const char* path ) {
	assert( path );

	u64 path_len = strlen( path );

	char* path_copy = cast( char*, mem_temp_alloc( path_len + 1 ) );
	strncpy( path_copy, path, path_len * sizeof( char ) );
	path_copy[path_len] = 0;

	if ( path_copy[path_len - 1] == '/' || path_copy[path_len - 1] == '\\' ) {
		path_copy[path_len - 1] = 0;
		path_len--;
	}

	WIN32_FIND_DATA find_data = {};
	HANDLE filehandle = FindFirstFile( path_copy, &find_data );

	u64 count = 0;

	do {
		if ( find_data.cFileName[0] == '.' || find_data.cFileName[0] == 0 ) {
			continue;
		}

		count++;
	} while ( FindNextFile( filehandle, &find_data ) );

	return count;
}

#endif // _WIN64