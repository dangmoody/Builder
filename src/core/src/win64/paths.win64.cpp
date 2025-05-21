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

#include <paths.h>

#include <allocation_context.h>
#include <temp_storage.h>
#include <debug.h>
#include <typecast.inl>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

static const char* get_last_slash( const char* path ) {
	const char* last_slash = NULL;
	const char* last_back_slash = strrchr( path, '\\' );
	const char* last_forward_slash = strrchr( path, '/' );

	if ( !last_back_slash && !last_forward_slash ) {
		return NULL;
	}

	if ( cast( u64, last_back_slash ) > cast( u64, last_forward_slash ) ) {
		last_slash = last_back_slash;
	} else {
		last_slash = last_forward_slash;
	}

	return last_slash;
}

/*
================================================================================================

	Paths

================================================================================================
*/

const char* path_app_path() {
	char* app_full_path = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
	app_full_path = cast( char*, path_remove_file_from_path( app_full_path ) );
#pragma clang diagnostic pop

	return app_full_path;
}

const char* path_current_working_directory() {
	char* cwd = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );
	cwd[length] = 0;

	return cwd;
}

const char* path_absolute_path( const char* file ) {
	char* absolute_path = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	GetFullPathName( file, MAX_PATH, absolute_path, NULL );
	return absolute_path;
}

const char* path_remove_file_from_path( const char* path ) {
	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		return NULL;
	}

	u64 path_length = cast( u64, last_slash ) - cast( u64, path );

	char* result = cast( char*, mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) ) );
	strncpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	return result;
}

const char* path_remove_path_from_file( const char* path ) {
	const char* last_slash = get_last_slash( path );

	if ( !last_slash ) {
		last_slash = path;
	} else {
		last_slash++;
	}

	return last_slash;
}

const char* path_remove_file_extension( const char* filename ) {
	const char* dot = strrchr( filename, '.' );

	if ( !dot ) {
		return filename;
	}

	u64 result_length = cast( u64, dot ) - cast( u64, filename );

	char* result = cast( char*, mem_temp_alloc( ( result_length + 1 ) * sizeof( char ) ) );
	strncpy( result, filename, result_length * sizeof( char ) );
	result[result_length] = 0;

	return result;
}

bool8 path_is_absolute( const char* path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

const char* path_canonicalise( const char* path ) {
	const char* path_copy = path_fix_slashes( path );

	u64 max_path_length = ( strlen( path_copy ) + 1 ) * sizeof( char );

	char* result = cast( char*, mem_temp_alloc( max_path_length ) );

	BOOL success = PathCanonicalizeA( result, path_copy );
	assert( success );

	result[max_path_length - 1] = 0;

	if ( *result == '/' ) {
		result++;
	} else if ( *result == '\\' ) {
		result++;
	}

	return result;
}

const char* path_fix_slashes( const char* path ) {
	u64 path_length = strlen( path );
	char* result = cast( char*, mem_temp_alloc( ( path_length + 1 ) * sizeof( char ) ) );
	memcpy( result, path, path_length * sizeof( char ) );
	result[path_length] = 0;

	For ( u64, char_index, 0, path_length ) {
		if ( result[char_index] == '/' ) {
			result[char_index] = '\\';
		}
	}

	return result;
}

char* path_relative_path_to(const char* path_from, const char* path_to)
{
	const char* result;
	if(PathRelativePathTo(result, path_from, FILE_ATTRIBUTE_NORMAL, path_to, FILE_ATTRIBUTE_NORMAL))
	{
		error("Unable to compute relative path, ensure provided paths exist.\nFrom Path: %s\nTo Path: %s", path_from, path_to);
	}
	return result;
}

bool8 path_set_current_directory(const char* path)
{
    return SetCurrentDirectory(path);
}

#endif // _WIN32