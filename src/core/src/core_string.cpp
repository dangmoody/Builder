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
	if ( length > dst->alloced ) {
		dst->alloced = length;

		dst->allocator = ( dst->allocator == NULL ) ? mem_get_current_allocator() : dst->allocator;

		mem_push_allocator( dst->allocator );

		// TODO(DM): 07/02/2025: using our own cast( T, x ) function doesnt work here when we enable CORE_MEMORY_TRACKING
		// this is because in that instance mem_realloc calls track_allocation_internal() followed immediately by track_free_internal(), which the compiler wont like if we were to do the following:
		//
		//	data = cast( T*, mem_realloc( data, alloced * sizeof( T ) ) );
		//
		// therefore we need to make the mem_realloc() define call just the one function instead of one after the other
		dst->data = (u8*) mem_realloc( dst->data, dst->alloced * sizeof( u8 ) );

		mem_pop_allocator();
	}
}

static void string_copy_internal( String* dst, const String* src ) {
	char* data_as_char = cast( char*, src->data );
	string_copy_from_c_string( dst, data_as_char, src->count );
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
		assert(allocator != nullptr);
		mem_push_allocator(allocator);
		mem_free( data );
		mem_pop_allocator();
		data = NULL;
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

u8 String::operator[]( const u64 index ) {
	assert( index < count );
	return data[index];
}

u8 String::operator[]( const u64 index ) const {
	assert( index < count );
	return data[index];
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

	u64 length = cast( u64, vsnprintf( NULL, 0, fmt, args ) );

	string_reserve_internal( dst, length + 1 );

	dst->count = length;

	vsnprintf( cast( char*, dst->data ), dst->alloced, fmt, args );
	dst->data[length] = 0;
}