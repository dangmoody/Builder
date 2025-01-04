/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include <string_builder.h>

#include <debug.h>
#include <allocation_context.h>

#include <stdio.h>
#include <stdarg.h>

#include "string_helpers.h"

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

	StringBuilderBuffer* buffer = cast( StringBuilderBuffer* ) mem_alloc( sizeof( StringBuilderBuffer ) );
	buffer->next = NULL;

	int length = string_vsnprintf( NULL, 0, fmt, args );
	length++;

	buffer->data = cast( char* ) mem_alloc( cast( u64 ) length * sizeof( char ) );

	string_vsnprintf( buffer->data, length, fmt, args );

	// if no head then this is the first element
	if ( !builder->head ) {
		builder->head = buffer;
		builder->tail = buffer;
	}

	builder->tail->next = buffer;
	builder->tail = buffer;
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
	u64 length = 0;
	u64 offset = 0;

	StringBuilderBuffer* current = builder->head;

	while ( current ) {
		length += cast( u64 ) string_snprintf( NULL, 0, current->data );

		current = current->next;
	}

	result = cast( char* ) mem_alloc( length * sizeof( char ) );

	current = builder->head;

	while ( current ) {
		offset += cast( u64 ) string_snprintf( result + offset, cast( s64 ) length, current->data );

		current = current->next;
	}

	return result;
}