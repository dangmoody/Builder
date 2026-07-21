/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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

#include "string_builder.h"

#include "linear_allocator.h"
#include "debug.h"
#include "typecast.h"

#include "stb_local.h"

#include <stdarg.h>
#include <memory.h>
#include <string.h>

/*
================================================================================================

	String Builder

================================================================================================
*/

StringBuilder SB_Create( LinearAllocator *allocator ) {
	assert( allocator );

	return {
		.allocator	= allocator,
		.head		= NULL,
		.tail		= NULL,
	};
}

void SB_Appendf( StringBuilder *builder, const char *fmt, ... ) {
	assert( builder );
	assert( fmt );

	va_list args;
	va_start( args, fmt );

	StringBuilderBuffer *buffer = cast( StringBuilderBuffer *, Mem_AllocatorAlloc( builder->allocator, sizeof( StringBuilderBuffer ) ) );
	memset( buffer, 0, sizeof( StringBuilderBuffer ) );

	va_list argsCopy;
	va_copy( argsCopy, args );

	buffer->length = trunc_cast( u32, stbsp_vsnprintf( NULL, 0, fmt, args ) );

	buffer->data = cast( char *, Mem_AllocatorAlloc( builder->allocator, buffer->length + 1, 1 ) );
	stbsp_vsnprintf( buffer->data, cast( int, buffer->length + 1 ), fmt, argsCopy );
	va_end( argsCopy );
	buffer->data[buffer->length] = 0;

	// if no head then this is the first element
	if ( !builder->head ) {
		builder->head = buffer;
		builder->tail = buffer;
	}

	builder->tail->next = buffer;
	builder->tail = buffer;
	builder->tail->next = NULL;

	va_end( args );
}

const char *SB_ToString( StringBuilder *builder ) {
	char *result = NULL;
	u64 totalLength = 0;
	u64 offset = 0;

	StringBuilderBuffer *current = builder->head;

	if ( !current ) {
		return NULL;
	}

	while ( current ) {
		totalLength += current->length;

		current = current->next;
	}

	totalLength += 1;

	result = cast( char *, Mem_AllocatorAlloc( builder->allocator, totalLength * sizeof( char ), 1 ) );

	current = builder->head;

	while ( current ) {
		strncpy( result + offset, current->data, current->length );

		offset += current->length;

		current = current->next;
	}

	result[totalLength - 1] = 0;

	return result;
}
