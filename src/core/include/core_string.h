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

#include "int_types.h"
#include "dll_export.h"

/*
================================================================================================

	String

	Container type used to hold a contiguous block of text.

	This string type only calls realloc() when making the string hold a larger piece of text,
	but unlike std::strings, Core Strings can't be appended or resized.  If you want to do
	that, use StringBuilder.

================================================================================================
*/

struct String {
	char*				data;
	u64					count;
};

CORE_API void			string_zero( String* out_str );

CORE_API void			string_printf( String* out_str, const char* fmt, ... );

CORE_API void			string_copy( String* dst, String* src );

CORE_API void			string_copy_from_c_string( String* out_str, const char* c_str );
CORE_API void			string_copy_from_c_string( String* out_str, const char* c_str, const u64 length );

CORE_API void			string_free( String* str );

// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
CORE_API bool8			string_equals( const char* lhs, const char* rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
CORE_API bool8			string_starts_with( const char* str, const char* prefix );

// Returns true if the last character of 'str' is the valueo of 'end'.  Case sensitive.
CORE_API bool8			string_ends_with( const char* str, const char end );

// Returns true if the last characters of 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
CORE_API bool8			string_ends_with( const char* str, const char* suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
CORE_API bool8			string_contains( const char* str, const char* substring );

// Returns a printf-formatted string with the given format string and var args that's been allocated via temp storage.
CORE_API const char*	temp_printf( const char* fmt, ... );

// Returns a copy of 'from' that has been allocated on temp storage.
CORE_API char*			temp_c_string( const char* from );

// Copies 'length' characters from 'from' and allocates it on temp storage.
CORE_API char*			temp_c_string( const char* from, const u64 length );
