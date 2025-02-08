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

#include <allocator_linear.h>
#include <allocation_context.h>

#include <core_types.h>
#include <memory_units.h>
#include <debug.h>

#include <stdlib.h>	// malloc, free

/*
================================================================================================

	linear allocator

================================================================================================
*/

LinearAllocator* linear_allocator_create( const u64 size_bytes) {
	assertf( size_bytes, "Can't create a linear allocator with a size of 0 bytes." );

	void* memory = malloc( sizeof( LinearAllocator ) + size_bytes );

	LinearAllocator* allocator = cast( LinearAllocator*, memory );
	allocator->offset = 0;
	allocator->size_bytes = size_bytes;
	allocator->arena = cast( u8*, memory ) + sizeof( LinearAllocator );

	return allocator;
}

void linear_allocator_destroy( LinearAllocator* allocator ) {
	assertf( allocator, "Linear allocator MUST be non-NULL." );

	free( allocator );
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

void* linear_allocator_alloc( LinearAllocator* allocator, const u64 size_bytes, const MemoryAlignment alignment ) {
	assertf( allocator, "Linear allocator handle MUST be non-NULL." );
	assertf( size_bytes, "Not allowed to allocate 0 bytes from linear allocator." );
	assertf( alignment != 0, "Not allowed to allocate memory from allocator with an alignment of 0 bytes." );

	u64 padding = padding_up( allocator->offset, alignment );

	{
		u64 remaining_bytes = allocator->size_bytes - allocator->offset;
		u64 total_size = size_bytes + padding;
		assertf( allocator->offset + total_size <= allocator->size_bytes,
				 "Linear allocator wanted to allocate %d bytes, but it only has %d bytes left.  It needs at least another %d bytes.\n",
				 total_size, remaining_bytes, ( size_bytes - total_size )
		);
	}

	allocator->offset += padding;

	void* ptr = ( cast( u8*, allocator->arena ) ) + allocator->offset;

	allocator->offset += size_bytes;

	return ptr;
}

#pragma clang diagnostic pop

void* linear_allocator_realloc( LinearAllocator* allocator, void* ptr, const u64 newSize, const MemoryAlignment alignment) {
	assert( allocator );
	assert( ptr );
	assert( newSize );

	unused( allocator );
	unused( ptr );
	unused( newSize );
	unused( alignment );

	fatal_error( "Linear allocator does not currently support memory reallocation." );

	return NULL;
}

void linear_allocator_free( LinearAllocator* allocator, void* ptr) {
	assert( allocator );
	assert( ptr );

	unused( allocator );
	unused( ptr );

	fatal_error(
		"Linear allocators do not free individual memory allocations.  " \
		"They are instead reset hollistically.  " \
		"Maybe you want to call Mem_Reset() instead?\n"
	);
}

u64	linear_allocator_tell( LinearAllocator* allocator ) {
	return allocator->offset;
}

void linear_allocator_rewind( LinearAllocator* allocator, u64 previous_position ) {
	assert( allocator );
	assertf( previous_position <= allocator->offset, "Cannot rewind forwads!" );

	allocator->offset = previous_position;
}

void linear_allocator_reset( LinearAllocator* allocator ) {
	assertf( allocator, "Linear allocator MUST be non-NULL." );

	allocator->offset = 0;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
void linear_allocator_create_generic_interface( Allocator& out_interface ) {
	out_interface.init = cast( allocator_init, &linear_allocator_create );
	out_interface.shutdown = cast( allocator_shutdown, &linear_allocator_destroy );

	out_interface.allocate_aligned = cast( allocator_allocate_aligned, &linear_allocator_alloc );
	out_interface.reallocate_aligned = cast( allocator_reallocate_aligned, &linear_allocator_realloc );

	out_interface.free = cast( allocator_free, &linear_allocator_free );
	out_interface.reset = cast( allocator_reset, &linear_allocator_reset );

	out_interface.data = NULL; // set by calling allocator_intitialize;
}
#pragma clang diagnostic pop