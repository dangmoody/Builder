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

#include <string_builder.h>

#include <debug.h>
#include <allocation_context.h>
#include <string_helpers.h>
#include <typecast.inl>

#include <stdio.h>
#include <stdarg.h>

/*
================================================================================================

	String Builder

================================================================================================
*/

void string_builder_reset( StringBuilder* builder ) {
	assert( builder );

	StringBuilderBuffer* current = builder->head;

	while ( current ) {
		StringBuilderBuffer* next = current->next;

		mem_free( current->data );
		current->data = NULL;

		current->next = NULL;
		mem_free( current );

		current = next;
	}
}

void string_builder_destroy( StringBuilder* builder ) {
	string_builder_reset( builder );
}

static void string_builder_appendfv( StringBuilder* builder, const char* fmt, va_list args ) {
	assert( builder );
	assert( fmt );
	assert( args );

	StringBuilderBuffer* buffer = cast( StringBuilderBuffer*, mem_alloc( sizeof( StringBuilderBuffer ) ) );
	//buffer->next = NULL;
	memset( buffer, 0, sizeof( StringBuilderBuffer ) );

	buffer->length = trunc_cast( u32, string_vsnprintf( NULL, 0, fmt, args ) );

	buffer->data = cast( char*, mem_alloc( trunc_cast( u64, ( buffer->length + 1 ) ) * sizeof( char ) ) );
	string_vsnprintf( buffer->data, buffer->length + 1, fmt, args );
	buffer->data[buffer->length] = 0;

	// if no head then this is the first element
	if ( !builder->head ) {
		builder->head = buffer;
		builder->tail = buffer;
	}

	builder->tail->next = buffer;
	builder->tail = buffer;
	builder->tail->next = NULL;
}

void string_builder_appendf( StringBuilder* builder, const char* fmt, ... ) {
	assert( builder );
	assert( fmt );

	va_list args;
	va_start( args, fmt );

	string_builder_appendfv( builder, fmt, args );

	va_end( args );
}

const char* string_builder_to_string( StringBuilder* builder ) {
	char* result = NULL;
	u64 total_length = 0;
	u64 offset = 0;

	StringBuilderBuffer* current = builder->head;

	if ( !current ) {
		return NULL;
	}

	while ( current ) {
		total_length += current->length;

		current = current->next;
	}

	total_length += 1;

	result = cast( char*, mem_alloc( total_length * sizeof( char ) ) );

	current = builder->head;

	while ( current ) {
		strncpy( result + offset, current->data, current->length );

		offset += current->length;

		current = current->next;
	}

	result[total_length - 1] = 0;

	return result;
}