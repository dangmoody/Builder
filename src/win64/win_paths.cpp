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

#include "../paths.h"

#include "../helpers.h"
#include "../temp_storage.h"
#include "../debug.h"
#include "../typecast.h"
#include "../string.h"

#include <Windows.h>
#include <Shlwapi.h>

/*
================================================================================================

	Paths

	Win64-specific functionality

================================================================================================
*/

string_t Path_AppPath( linearAllocator_t *allocator ) {
	char appFullPath[MAX_PATH] = {};
	DWORD length = GetModuleFileNameA( NULL, appFullPath, MAX_PATH );

	return String_Alloc( allocator, appFullPath, length );
}

string_t Path_GetCwd( linearAllocator_t *allocator ) {
	char cwd[MAX_PATH] = {};
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );

	return String_Alloc( allocator, cwd, length );
}

bool8 Path_SetCwd( const char *path ) {
	Assert( path );

	return Cast( bool8, SetCurrentDirectory( path ) );
}

string_t Path_AbsolutePath( linearAllocator_t *allocator, const char *path ) {
	Assert( path );

	char absolutePath[MAX_PATH] = {};
	DWORD length = GetFullPathName( path, MAX_PATH, absolutePath, NULL );

	return String_Alloc( allocator, absolutePath, length );
}

bool8 Path_IsAbsolute( const char *path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

string_t Path_FixSlashes( linearAllocator_t *allocator, string_t *str ) {
	return String_Replace( allocator, str, '/', PATH_SEPARATOR );
}

#endif // _WIN32
