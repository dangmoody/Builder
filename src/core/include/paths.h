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

#pragma once

#include "core_types.h"
#include "dll_export.h"

/*
================================================================================================

	File path helper functions

	A series of helper functions for OS-dependent API calls for things like getting the CWD, as
	well as some non OS-dependent things.

================================================================================================
*/

#ifdef _WIN32
	#define PATH_SEPARATOR '\\'
#else
	#define PATH_SEPARATOR '/'
#endif

// Returns the absolute path of where the current program is running from.
CORE_API const char*	path_app_path();

// Returns the path that your program is currently running from.
CORE_API const char*	path_current_working_directory();

// Returns the absolute path of 'path'.
CORE_API const char*	path_absolute_path( const char *path );

// Given a file path that also includes a filename, will remove the filename part, leaving just the path.
CORE_API const char*	path_remove_file_from_path( const char* path );

// Given a file path that also includes a filename, will remove the path part, leaving just the filename.
CORE_API const char*	path_remove_path_from_file( const char* path );

// Returns the name of a file without its file extension, if there is one.
CORE_API const char*	path_remove_file_extension( const char* filename );

// On Windows:   Returns true if the path starts with a letter followed by a colon (for example: "C:"), otherwise returns false.
// On Mac/Linux: Returns true if the path starts with two backslashes or a single forward slash, otherwise returns false.
CORE_API bool8			path_is_absolute( const char* path );

CORE_API const char*	path_canonicalise( const char* path );

// Make sure that any slashes found in 'path' are what the OS expects them to be.
CORE_API const char*	path_fix_slashes( const char* path );

CORE_API char*			path_relative_path_to( const char* pathFrom, const char* pathTo );

CORE_API bool8			path_set_current_directory( const char* path );

// Takes a potentially-infinite number of strings and separates each one with a slash (back slash on Windows, forward slash on all other platforms).
// template<class... Args>
// CORE_API const char*	path_join( const char* first, const Args*... args );