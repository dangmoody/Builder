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

struct LinearAllocator;
struct String;

/*
================================================================================================

	File path helper functions

	A series of helper functions for OS-dependent API calls for things like getting the CWD, as
	well as some non OS-dependent things.

================================================================================================
*/

#ifdef _WIN32
	#define PATH_SEPARATOR	'\\'
#else
	#define PATH_SEPARATOR	'/'
#endif

// Returns the absolute path of where the current program is running from.
String				path_app_path( LinearAllocator *allocator );

// Returns the path that your program is currently running from.
String				path_get_cwd( LinearAllocator *allocator );

// Sets the current working directory (cwd) that the program will run from to the specified path.
bool8				path_set_cwd( const char *path );

// Returns the absolute path of 'file'.
String				path_absolute_path( LinearAllocator *allocator, const char *path );

// If 'path' is a path with a filename, then returns just the path part with the filename part removed.  Otherwise returns the original string.
String				path_remove_file_from_path( const String *path );

// If 'path' is a path with a filename, then returns just the filename part with the path part removed.  Otherwise returns the original string.
String				path_remove_path_from_file( const String *path );

// If 'filename' is a filename with a file extension then returns that filename without the file extension.  Otherwise returns the original filename.
String				path_remove_file_extension( const String *filename );

// On Windows:   Returns true if the path starts with a letter followed by a colon (for example: "C:"), otherwise returns false.
// On Mac/Linux: Returns true if the path starts with two backslashes or a single forward slash, otherwise returns false.
bool8				path_is_absolute( const char *path );

// Make sure that any slashes found in 'path' are what the OS expects them to be.
String				path_fix_slashes( LinearAllocator *allocator, String *str );

String				path_relative_path_to( LinearAllocator *allocator, const char *from, const char *to );

// DO NOT CALL THIS DIRECTLY.
// CALL path_join INSTEAD.
String				path_join_internal( LinearAllocator *allocator, const int count, ... );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"

template<typename ...Args>
inline int path_va_args_count( Args&&... ) {
	return sizeof...( Args );
}

// Takes a variable number of strings and separates each one with a slash (back slash on Windows, forward slash on all other platforms).
#define path_join( allocator, ... )	path_join_internal( allocator, path_va_args_count( __VA_ARGS__ ), __VA_ARGS__ )

#pragma clang diagnostic pop
