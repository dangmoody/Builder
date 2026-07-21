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

#pragma once

#include "int_types.h"

struct linearAllocator_t;

/*
================================================================================================

	string_t

	Container type used to hold a contiguous block of text.

	Core Strings are not null-terminated.  They are string views into string data and cannot be
	appended or resized (like how std::strings can).  If you want to do that, use stringBuilder_t.

	Core Strings do not own the memory that they hold.  They are either read only strings or
	the memory was given to them from some memory allocation call.

	Strings set from a string literal point into read-only memory and cannot not be directly
	mutated.

	Strings that are allocated are still string views, but their characters can be mutated
	directly.

	A string_t's count reflects the number of characters that are logically part of the string,
	and the underlying data buffer may extend beyond the count (e.g. when a string_t is a
	substring of a larger allocation).  Do not use `data` directly.

================================================================================================
*/

struct string_t {
	char	*data;
	u64		count;
};

// Sets the contents of the string to the specified string literal.
// This string will not own the data it holds.
string_t	String_Set( const char *str );
string_t	String_Set( const char *str, const u64 count );

// Sets the contents of the string to the range of specified string literal.
// This string will not own the data it holds.
string_t	String_Substring( const char *str, const u64 offset, const u64 count );

// Allocates a copy of the null-terminated C string 'str' using 'allocator'.
// This string will hold the data it holds.
string_t	String_Alloc( linearAllocator_t *allocator, const char *str );

// Allocates a copy of the first 'length' characters of 'str' using 'allocator'.
// // This string will hold the data it holds.
string_t	String_Alloc( linearAllocator_t *allocator, const char *str, const u64 length );

// Allocates a printf-formatted string using 'allocator'.
string_t	String_Printf( linearAllocator_t *allocator, const char *fmt, ... );

// Allocates a copy of 'src' using 'allocator'.
string_t	String_Copy( linearAllocator_t *allocator, const string_t *src );

// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
bool8		String_Equals( const char *lhs, const char *rhs );
bool8		String_Equals( const string_t *lhs, const string_t *rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
bool8		String_StartsWith( const char *str, const char *prefix );
bool8		String_StartsWith( const string_t *str, const string_t *prefix );

// Returns true if the last character of 'str' is the value of 'end'.  Case sensitive.
bool8		String_EndsWith( const char *str, const char end );
bool8		String_EndsWith( const string_t *str, const char end );

// Returns true if the last characters of 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
bool8		String_EndsWith( const char *str, const char *suffix );
bool8		String_EndsWith( const string_t *str, const string_t *suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
bool8		String_Contains( const char *str, const char c );
bool8		String_Contains( const char *str, const char *substring );
bool8		String_Contains( const string_t *str, const char c );
bool8		String_Contains( const string_t *str, const string_t *substring );

// Replaces every occurrence of 'oldChar' in 'str' with 'newChar'.
string_t	String_Replace( linearAllocator_t *allocator, string_t *str, const char oldChar, const char newChar );

// Returns true if character 'c' is found in 'str', searching left to right, and sets 'outIndex' to the position of the first occurrence.  Returns false if 'c' cannot be found.
bool8		String_FindFromLeft( const string_t *str, const char c, u64 *outIndex );

// Returns true if character 'c' is found in 'str', searching right to left, and sets 'outIndex' to the position of the last occurrence.  Returns false if 'c' cannot be found.
bool8		String_FindFromRight( const string_t *str, const char c, u64 *outIndex );

// Returns a null-terminated copy of 'str' allocated on temp storage, truncated to 'str->count'.
// Use this to safely pass a string_t as a '%s' argument to printf.
const char	*String_Cstr( const string_t *str );

// Returns a printf-formatted string with the given format string and var args that's been allocated via temp storage.
const char	*TempPrintf( const char *fmt, ... );

// Returns a copy of 'from' that has been allocated on temp storage.
char		*TempCString( const char *from );

// Copies 'length' characters from 'from' and allocates it on temp storage.
char		*TempCString( const char *from, const u64 length );
