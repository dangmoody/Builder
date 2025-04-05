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

struct Allocator;

/*
================================================================================================

	String

	Container type used to represent text.

	Unlike C++'s std::string, this string type doesn't allow for appending additional data on
	the end of it.  If you want to do that, use StringBuilder.

	This string type only calls realloc() when making the string hold a larger piece of text.

================================================================================================
*/

struct String {
	u8*			data = NULL;
	u64			count = 0;
	u64			alloced = 0;
	Allocator*	allocator = NULL;

				String() {}
				String( const char* str );
				String( const String& str );
				~String();

	String&		operator=( const char* str );
	String&		operator=( const String& str );

	u8			operator[]( const u64 index );
	u8			operator[]( const u64 index ) const;
};

CORE_API void	string_copy_from_c_string( String* dst, const char* src, const u64 src_length );
CORE_API void	string_printf( String* dst, const char* fmt, ... );