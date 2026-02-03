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

#include <core_string.h>
#include <debug.h>
#include <typecast.inl>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#pragma clang diagnostic ignored "-Wc++20-designator"
#pragma clang diagnostic ignored "-Wreorder-init-list"
#endif

static void string_realloc_internal( String* out_str, const u64 length ) {
	if ( length > out_str->count ) {
		out_str->data = cast( char*, realloc( out_str->data, length * sizeof( char ) ) );
	}
}

/*
================================================================================================

	String

================================================================================================
*/

void string_zero( String* out_str ) {
	out_str->data = NULL;
	out_str->count = 0;
}

void string_printf( String* out_str, const char* fmt, ... ) {
	assert( out_str );
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	defer { va_end( args ); };

	u64 length = cast( u64, vsnprintf( NULL, 0, fmt, args ) );

	string_realloc_internal( out_str, length + 1 );
	vsnprintf( out_str->data, length, fmt, args );
	out_str->data[length] = 0;
}

void string_copy( String* dst, String* src ) {
	string_copy_from_c_string( dst, src->data, src->count );
}

void string_copy_from_c_string( String* out_str, const char* c_str ) {
	string_copy_from_c_string( out_str, c_str, strlen( c_str ) );
}

void string_copy_from_c_string( String* out_str, const char* c_str, const u64 length ) {
	string_realloc_internal( out_str, length + 1 );
	memcpy( out_str->data, c_str, length );
	out_str->data[length] = 0;
}

void string_free( String* str ) {
	if ( str->data ) {
		free( str->data );
		str->data = NULL;
	}
}

bool8 string_equals( const char* lhs, const char* rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhs_len = strlen( lhs );
	return lhs_len == strlen( rhs ) && strncmp( lhs, rhs, lhs_len ) == 0;
}

bool8 string_starts_with( const char* str, const char* prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 string_ends_with( const char* str, const char end ) {
	assert( str );

	return str[strlen( str ) - 1] == end;
}

bool8 string_ends_with( const char* str, const char* suffix ) {
	assert( str );
	assert( suffix );

	u64 suffix_length = strlen( suffix );
	return strncmp( str + strlen( str ) - suffix_length, suffix, suffix_length ) == 0;
}

bool8 string_contains( const char* str, const char* substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

const char* temp_printf( const char* fmt, ... ) {
	assert( fmt );

	va_list args;
	va_start( args, fmt );

	s64 string_length = string_vsnprintf( NULL, 0, fmt, args );
	string_length += 1;	// + 1 for null terminator

	char* out_string = cast( char*, mem_temp_alloc( trunc_cast( u64, string_length ) * sizeof( char ) ) );

	string_vsnprintf( out_string, string_length, fmt, args );
	out_string[string_length - 1] = 0;

	va_end( args );

	return out_string;
}

char* temp_c_string( const char* from ) {
	return temp_c_string( from, strlen( from ) );
}

char* temp_c_string( const char* from, const u64 length ) {
	char* result = cast( char*, mem_temp_alloc( ( length + 1 ) * sizeof( char ) ) );
	strncpy( result, from, length * sizeof( char ) );
	result[length] = 0;

	return result;
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
