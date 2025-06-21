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

#include <allocation_context.h>
#include <debug.h>
#include <typecast.inl>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static void string_reserve_internal( String* dst, const u64 length ) {
	if ( length > dst->count ) {
		dst->allocator = ( dst->allocator == NULL ) ? mem_get_current_allocator() : dst->allocator;

		mem_push_allocator( dst->allocator );
		defer( mem_pop_allocator() );

		dst->data = cast( char*, mem_realloc( dst->data, length * sizeof( char ) ) );
	}
}

static void string_copy_internal( String* dst, const String* src ) {
	string_copy_from_c_string( dst, src->data, src->count );
}

/*
================================================================================================

	String

================================================================================================
*/

String::String( const char* str ) {
	string_copy_from_c_string( this, str, strlen( str ) );
}

String::String( const String& str ) {
	string_copy_internal( this, &str );
}

String::~String() {
	if ( data ) {
		assert( allocator != nullptr );

		mem_push_allocator( allocator );

		mem_free( data );
		data = NULL;

		mem_pop_allocator();
	}
}

String& String::operator=( const char* str ) {
	string_copy_from_c_string( this, str, strlen( str ) );
	return *this;
}

String& String::operator=( const String& str ) {
	string_copy_internal( this, &str );
	return *this;
}

char String::operator[]( const u64 index ) {
	assert( index < count );
	return data[index];
}

char String::operator[]( const u64 index ) const {
	assert( index < count );
	return data[index];
}

//================================================================

bool8 string_equals( const String* lhs, const String* rhs ) {
	assert( lhs );
	assert( rhs );

	return strcmp( lhs->data, rhs->data ) == 0;
}

void string_copy_from_c_string( String* dst, const char* src, const u64 length ) {
	string_reserve_internal( dst, length + 1 );

	dst->count = length;

	memcpy( dst->data, src, length );
	dst->data[length] = 0;
}

void string_printf( String* dst, const char* fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	defer( va_end( args ) );

	u64 length = cast( u64, string_vsnprintf( NULL, 0, fmt, args ) );

	string_reserve_internal( dst, length + 1 );

	dst->count = length;

	string_vsnprintf( dst->data, trunc_cast( s64, dst->count + 1 ), fmt, args );
	dst->data[length] = 0;
}