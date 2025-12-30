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

#include <temp_storage.h>
#include <debug.h>
#include <typecast.inl>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Shlwapi.h>

/*
================================================================================================

	Paths

	Win64-specific functionality

================================================================================================
*/

const char* path_app_path() {
	char* app_full_path = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetModuleFileNameA( NULL, app_full_path, MAX_PATH );

	return app_full_path;
}

const char* path_current_working_directory() {
	char* cwd = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );
	cwd[length] = 0;

	return cwd;
}

const char* path_absolute_path( const char* file ) {
	assert( file );

	char* absolute_path = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	GetFullPathName( file, MAX_PATH, absolute_path, NULL );
	return absolute_path;
}

bool8 path_is_absolute( const char* path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

const char* path_canonicalise( const char* path ) {
	assert( path );

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
	char* result = temp_c_string( path, path_length );

	For ( u64, char_index, 0, path_length ) {
		if ( result[char_index] == '/' ) {
			result[char_index] = '\\';
		}
	}

	return result;
}

// TODO(DM): 10/10/2025: I'm very tempted to remove this implementation and make the linux one work cross platform
// for some reason this windows implementation cares whether or not the files we are referencing are files or directories and it starts returning very wrong answers if we are wrong about the file type we specify vs the actual path
// the linux implementation doesnt care about any of that and instead just compares the strings (which is all it needs to do)
char* path_relative_path_to( const char* path_from, const char* path_to ) {
	assert( path_from );
	assert( path_to );

	char* result = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );

	if ( !PathRelativePathTo( result, path_fix_slashes( path_from ), FILE_ATTRIBUTE_DIRECTORY, path_fix_slashes( path_to ), FILE_ATTRIBUTE_DIRECTORY ) ) {
		error( "Unable to compute relative path, ensure provided paths exist.\nFrom Path: %s\nTo Path: %s", path_from, path_to );
	}

	return result;
}

bool8 path_set_current_directory( const char* path ) {
	assert( path );

	return cast( bool8, SetCurrentDirectory( path ) );
}

#endif // _WIN32