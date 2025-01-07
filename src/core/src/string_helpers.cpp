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

#include <string_helpers.h>
#include <debug.h>
#include <temp_storage.h>

#include <string.h>
#include <stdarg.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_SPRINTF_IMPLEMENTATION
#include "3rdparty/stb/stb_sprintf.h"
#pragma clang diagnostic pop

/*
================================================================================================

	String Helpers

================================================================================================
*/

bool8 string_equals( const char* lhs, const char* rhs ) {
	assertf( lhs, "lhs cannot be NULL." );
	assertf( rhs, "rhs cannot be NULL." );

	u64 lhs_len = strlen( lhs );
	return lhs_len == strlen( rhs ) && strncmp( lhs, rhs, lhs_len ) == 0;
}

bool8 string_starts_with( const char* str, const char* prefix ) {
	assertf( str, "str cannot be NULL." );
	assertf( prefix, "prefix cannot be NULL." );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 string_ends_with( const char* str, const char* suffix ) {
	assertf( str, "str cannot be NULL." );
	assertf( suffix, "suffix cannot be NULL." );

	u64 suffix_length = strlen( suffix );
	return strncmp( str + strlen( str ) - suffix_length, suffix, suffix_length ) == 0;
}

bool8 string_contains( const char* str, const char* substring ) {
	assertf( str, "str cannot be NULL." );
	assertf( substring, "sub-string cannot be NULL." );

	return strstr( str, substring ) != NULL;
}

void string_substring( const char* str, const u64 offset, const u64 count, char* out_string ) {
	const u64 string_length = strlen( str );

	assertf( str, "str cannot be NULL." );
	assertf( offset < string_length, "Attempted to get a substring where the offset is bigger than the length of the string.  You cannot do this." );
	assertf( count < string_length, "Attempted to get a substring where the length of the substring is bigger than the length of the string.  You cannot do this." );
	/*assertf( offset + count <= string_length,
		"Attempted to get a substring where the offset + length of the substring is bigger than the length of the string.  You cannot do this." );*/

	unused( string_length );

	strncpy( out_string, str + offset, count );
	out_string[count - 1] = 0;
}

// TODO(DM): 1/1/2023: this needs to be replaced with a sprintf #define that chooses stb if the use_stb define is set
int string_sprintf( char* buffer, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	int result = string_vsprintf( buffer, fmt, args );
	va_end( args );
	return result;
}

// TODO(DM): 1/1/2023: this needs to be replaced with a snprintf #define that chooses stb if the use_stb define is set
int string_snprintf( char* buffer, const s64 buffer_length, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	int result = string_vsnprintf( buffer, buffer_length, fmt, args );
	va_end( args );
	return result;
}

// TODO(DM): 1/1/2023: this needs to be replaced with a vsprintf #define that chooses stb if the use_stb define is set
int string_vsprintf( char* buffer, const char* fmt, const va_list args ) {
	return stbsp_vsprintf( buffer, fmt, args );
}

// TODO(DM): 1/1/2023: this needs to be replaced with a vsprintf #define that chooses stb if the use_stb define is set
int string_vsnprintf( char* buffer, const s64 buffer_length, const char* fmt, const va_list args ) {
	return stbsp_vsnprintf( buffer, TruncS64ToS32( buffer_length ), fmt, args );
}

char* tprintf( const char* fmt, ... ) {
	assertf( fmt, "Format string MUST be non-NULL.", __FUNCTION__ );

	va_list args;
	va_start( args, fmt );

	char* out_string = vtprintf( fmt, args );

	va_end( args );

	return out_string;
}

char* vtprintf( const char* fmt, const va_list args ) {
	assertf( fmt, "Format string MUST be non-NULL." );

	s64 string_length = cast( s64 ) string_vsnprintf( NULL, 0, fmt, args );
	string_length++;	// + 1 for null terminator

	char* out_string = cast( char* ) mem_temp_alloc( cast( u64 ) string_length * sizeof( char ) );

	string_vsnprintf( out_string, string_length, fmt, args );
	out_string[string_length - 1] = 0;

	return out_string;
}