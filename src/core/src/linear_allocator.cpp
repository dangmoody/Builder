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

#include "linear_allocator.h"

#include <typecast.inl>
#include <core_helpers.h>
#include <core_memory.h>
#include <core_math.h>
#include <debug.h>
#include <os.h>

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

LinearAllocator *linear_allocator_create( const u64 reserved_bytes ) {
	assert( reserved_bytes );

	u32 page_size = os_get_virtual_memory_page_size();

	u64 actual_reserved_bytes = reserved_bytes;

	if ( actual_reserved_bytes % page_size != 0 ) {
		actual_reserved_bytes = align_up( actual_reserved_bytes, page_size );

		warning(
			"LinearAllocator: specified reserved bytes (%llu) is not a multiple of the virtual memory page size (%u bytes).\n"
			"The OS dictates that any virtual memory pages that get reserved will automatically be a multiple of %u, so the specified reserved bytes will be rounded up to %llu bytes."
			, reserved_bytes, page_size, page_size, actual_reserved_bytes );
	}

	// TODO: DM: 29/12/2025: alloc the whole allocator plus its entire arena in one virtual alloc call
	LinearAllocator *allocator = cast( LinearAllocator *, malloc( sizeof( LinearAllocator ) ) );
	memset( allocator, 0, sizeof( LinearAllocator ) );
	allocator->ptr = cast( u8 *, virtual_reserve( actual_reserved_bytes ) );
	allocator->reserved_bytes = actual_reserved_bytes;
	allocator->virtual_memory_page_size = page_size;

	return allocator;
}

void linear_allocator_destroy( LinearAllocator *allocator ) {
	assert( allocator );

	virtual_free( allocator->ptr );

	free( allocator );
	allocator = NULL;
}

void* linear_allocator_alloc( LinearAllocator *allocator, const u64 size_bytes, const u32 alignment ) {
	assert( allocator );
	assert( size_bytes );

	allocator->offset = align_up( allocator->offset, alignment );

	if ( allocator->offset >= allocator->comitted_bytes || allocator->offset + size_bytes > allocator->comitted_bytes ) {
		u64 new_comitted_bytes = align_up( allocator->offset + size_bytes, allocator->virtual_memory_page_size );

		assert( new_comitted_bytes <= allocator->reserved_bytes );

		void *result = virtual_commit( allocator->ptr + allocator->comitted_bytes, new_comitted_bytes - allocator->comitted_bytes );
		assert( result );
		unused( result );

		allocator->comitted_bytes = new_comitted_bytes;
	}

	u8 *ptr = allocator->ptr + allocator->offset;

	allocator->offset += size_bytes;

	return ptr;
}

void linear_allocator_reset( LinearAllocator *allocator ) {
	allocator->offset = 0;
}

u64 linear_allocator_tell( LinearAllocator *allocator ) {
	return allocator->offset;
}

void linear_allocator_rewind_to( LinearAllocator *allocator, const u64 offset ) {
	allocator->offset = offset;
}

void linear_allocator_rewind_by( LinearAllocator *allocator, const u64 bytes ) {
	allocator->offset -= bytes;
}
