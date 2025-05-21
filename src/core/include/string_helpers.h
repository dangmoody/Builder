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

#include <stdarg.h>

#include "core_types.h"
#include "dll_export.h"


// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
CORE_API bool8	string_equals( const char* lhs, const char* rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
CORE_API bool8	string_starts_with( const char* str, const char* prefix );

// Returns true if the last characters of string 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
CORE_API bool8	string_ends_with( const char* str, const char* suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
CORE_API bool8	string_contains( const char* str, const char* substring );

// Takes a slice of characters from string 'str' starting at 'offset' + 'count' characters and puts it into out_string.
CORE_API void	string_substring( const char* str, const u64 offset, const u64 count, char* out_string );

// TODO(DM): 13/2/2023: these wrappers shouldnt really exist
// what we should do instead is just #define sprintf to be either the C implementation or the stb one (or whatever)
// and then people can just call sprintf instead of having to use these
// I think that would be better
CORE_API int	string_sprintf( char* buffer, const char* fmt, ... );
CORE_API int	string_vsprintf( char* buffer, const char* fmt, va_list args );
CORE_API int	string_snprintf( char* buffer, const s64 buffer_length, const char* fmt, ... );
CORE_API int	string_vsnprintf( char* buffer, const s64 buffer_length, const char* fmt, va_list args );

// Returns a char* that is the result of calling sprintf with the given format string and var args, uses temp storage to allocate the result string.
CORE_API char*	tprintf( const char* fmt, ... );

// Returns a char* that is the result of calling sprintf with the given format string and args list, uses temp storage to allocate the result string.
CORE_API char*	vtprintf( const char* fmt, va_list args );