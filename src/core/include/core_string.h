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

struct LinearAllocator;

/*
================================================================================================

	String

	Container type used to hold a contiguous block of text.

	Unlike std::strings, Core Strings are treated as array views into string data and can't be
	appended or resized.  If you want to do that, use StringBuilder.

================================================================================================
*/

struct String {
	char			*data;
	u64				count;
};

// Allocates a copy of the null-terminated C string 'str' using 'allocator'.
CORE_API String		string_set( LinearAllocator *allocator, const char *str );

// Allocates a copy of the first 'length' characters of 'str' using 'allocator'.
CORE_API String		string_set( LinearAllocator *allocator, const char *str, const u64 length );

// Allocates a printf-formatted string using 'allocator'.
CORE_API String		string_printf( LinearAllocator *allocator, const char *fmt, ... );

// Allocates a copy of 'src' using 'allocator'.
CORE_API String		string_copy( LinearAllocator *allocator, const String *src );

// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
CORE_API bool8		string_equals( const char *lhs, const char *rhs );
CORE_API bool8		string_equals( const String *lhs, const String *rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
CORE_API bool8		string_starts_with( const char *str, const char *prefix );
CORE_API bool8		string_starts_with( const String *str, const String *prefix );

// Returns true if the last character of 'str' is the value of 'end'.  Case sensitive.
CORE_API bool8		string_ends_with( const char *str, const char end );
CORE_API bool8		string_ends_with( const String *str, const char end );

// Returns true if the last characters of 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
CORE_API bool8		string_ends_with( const char *str, const char *suffix );
CORE_API bool8		string_ends_with( const String *str, const String *suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
CORE_API bool8		string_contains( const char *str, const char c );
CORE_API bool8		string_contains( const char *str, const char *substring );
CORE_API bool8		string_contains( const String *str, const char c );
CORE_API bool8		string_contains( const String *str, const String *substring );

// Replaces every occurrence of 'old_char' in 'str' with 'new_char'.
CORE_API void		string_replace( String *str, const char old_char, const char new_char );

// Returns true if character 'c' is found in 'str', searching left to right, and sets 'out_index' to the position of the first occurrence.  Returns false if 'c' cannot be found.
CORE_API bool8		string_find_from_left( const String *str, const char c, u64 *out_index );

// Returns true if character 'c' is found in 'str', searching right to left, and sets 'out_index' to the position of the last occurrence.  Returns false if 'c' cannot be found.
CORE_API bool8		string_find_from_right( const String *str, const char c, u64 *out_index );

// Returns a printf-formatted string with the given format string and var args that's been allocated via temp storage.
CORE_API const char	*temp_printf( const char *fmt, ... );

// Returns a copy of 'from' that has been allocated on temp storage.
CORE_API char		*temp_c_string( const char *from );

// Copies 'length' characters from 'from' and allocates it on temp storage.
CORE_API char		*temp_c_string( const char *from, const u64 length );
