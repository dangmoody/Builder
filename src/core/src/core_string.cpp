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
#include <temp_storage.h>
#include <core_helpers.h>
#include <linear_allocator.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static String string_vprintf( LinearAllocator *allocator, const char *fmt, va_list args ) {
	assert( allocator );
	assert( fmt );

	String result;

	va_list args_copy;
	va_copy( args_copy, args );

	int length = vsnprintf( NULL, 0, fmt, args );
	assert( length >= 0 );

	result.count = cast( u64, length );

	result.data = cast( char *, linear_allocator_alloc( allocator, result.count + 1 ) );
	vsnprintf( result.data, result.count + 1, fmt, args_copy );
	result.data[result.count] = 0;

	va_end( args_copy );

	return result;
}

/*
================================================================================================

	String

================================================================================================
*/

String string_set( LinearAllocator *allocator, const char *str ) {
	assert( allocator );
	assert( str );

	return string_set( allocator, str, strlen( str ) );
}

String string_set( LinearAllocator *allocator, const char *str, const u64 length ) {
	assert( allocator );
	assert( str );

	String result = {
		.data	= cast( char *, linear_allocator_alloc( allocator, length + 1 ) ),
		.count	= length,
	};

	memcpy( result.data, str, length );
	result.data[length] = 0;

	return result;
}

String string_printf( LinearAllocator *allocator, const char *fmt, ... ) {
	assert( allocator );
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	String result = string_vprintf( allocator, fmt, args );
	va_end( args );

	return result;
}

String string_copy( LinearAllocator *allocator, const String *src ) {
	assert( allocator );
	assert( src );

	return string_set( allocator, src->data, src->count );
}

bool8 string_equals( const char *lhs, const char *rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhs_len = strlen( lhs );
	u64 rhs_len = strlen( rhs );

	return ( lhs_len == rhs_len ) && memcmp( lhs, rhs, lhs_len ) == 0;
}

bool8 string_equals( const String *lhs, const String *rhs ) {
	assert( lhs );
	assert( rhs );

	return ( lhs->count == rhs->count ) && ( memcmp( lhs->data, rhs->data, lhs->count ) == 0 );
}

bool8 string_starts_with( const char *str, const char *prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 string_starts_with( const String *str, const String *prefix ) {
	assert( str );
	assert( prefix );

	return ( prefix->count <= str->count ) && ( memcmp( str->data, prefix->data, prefix->count ) == 0 );
}

bool8 string_ends_with( const char *str, const char end ) {
	assert( str );

	u64 len = strlen( str );

	return ( len > 0 ) && ( str[len - 1] == end );
}

bool8 string_ends_with( const char *str, const char *suffix ) {
	assert( str );
	assert( suffix );

	u64 len = strlen( str );
	u64 suffix_length = strlen( suffix );

	return ( suffix_length <= len ) && ( memcmp( str + len - suffix_length, suffix, suffix_length ) == 0 );
}

bool8 string_ends_with( const String *str, const char end ) {
	assert( str );

	return ( str->count > 0 ) && ( str->data[str->count - 1] == end );
}

bool8 string_ends_with( const String *str, const String *suffix ) {
	assert( str );
	assert( suffix );

	return ( suffix->count <= str->count ) && ( memcmp( str->data + ( str->count - suffix->count ), suffix->data, suffix->count ) == 0 );
}

bool8 string_contains( const char *str, const char *substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

bool8 string_contains( const String *str, const String *substring ) {
	assert( str );
	assert( substring );

	return memmem( str->data, str->count, substring->data, substring->count ) != NULL;
}

void string_replace( String* str, const char old_char, const char new_char ) {
	assert( str );

	For ( u64, char_index, 0, str->count ) {
		if ( str->data[char_index] == old_char ) {
			str->data[char_index] = new_char;
		}
	}
}

bool8 string_find_from_left( const String *str, const char c, u64 *out_index ) {
	assert( str );
	assert( out_index );

	char *pos = cast( char *, memchr( str->data, c, str->count ) );

	if ( !pos ) {
		return false;
	}

	*out_index = cast( u64, pos ) - cast( u64, str->data );

	return true;
}

bool8 string_find_from_right( const String *str, const char c, u64 *out_index ) {
	assert( str );
	assert( out_index );

	char *pos = cast( char *, memrchr( str->data, c, str->count ) );

	if ( !pos ) {
		return false;
	}

	*out_index = cast( u64, pos ) - cast( u64, str->data );

	return true;
}

const char *temp_printf( const char *fmt, ... ) {
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	String result = string_vprintf( g_temp_storage, fmt, args );
	va_end( args );

	return result.data;
}

char *temp_c_string( const char *from ) {
	assert( from );

	return temp_c_string( from, strlen( from ) );
}

char *temp_c_string( const char *from, const u64 num_chars ) {
	assert( from );
	assert( num_chars );

	char *result = cast( char *, mem_temp_alloc( num_chars + 1 ) );
	memcpy( result, from, num_chars );
	result[num_chars] = 0;

	return result;
}
