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

#include <core_types.h>
#include <memory_units.h>
#include <debug.h>

#include <stdlib.h>	// malloc, free

/*
================================================================================================

	linear allocator

================================================================================================
*/

void mem_create_linear( const u64 size_bytes, void** out_allocator_data ) {
	assertf( size_bytes, "Can't create a linear allocator with a size of 0 bytes." );
	assert( out_allocator_data );

	void* memory = malloc( sizeof( AllocatorLinearData ) + size_bytes );

	AllocatorLinearData* allocator = cast( AllocatorLinearData* ) memory;
	allocator->offset = 0;
	allocator->size_bytes = size_bytes;

	*out_allocator_data = allocator;
}

void mem_destroy_linear( void* allocator_data ) {
	assertf( allocator_data, "Linear allocator MUST be non-NULL." );

	AllocatorLinearData* allocator = cast( AllocatorLinearData* ) allocator_data;

	free( allocator );
	allocator = NULL;
}

void* mem_alloc_linear( void* allocator_data, const u64 size_bytes, const char* file, const int line ) {
	return mem_alloc_linear_aligned( allocator_data, size_bytes, MEMORY_ALIGNMENT_EIGHT, file, line );
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

void* mem_alloc_linear_aligned( void* allocator_data, const u64 size_bytes, const MemoryAlignment alignment, const char* file, const int line ) {
	assertf( allocator_data, "Linear allocator handle MUST be non-NULL." );
	assertf( size_bytes, "Not allowed to allocate 0 bytes from linear allocator." );
	assertf( alignment != 0, "Not allowed to allocate memory from allocator with an alignment of 0 bytes." );
	assert( file );
	assert( line > 0 );

	AllocatorLinearData* allocator = cast( AllocatorLinearData* ) allocator_data;

	{
		u64 remaining_bytes = allocator->size_bytes - allocator->offset;

		assertf( allocator->offset + size_bytes <= allocator->size_bytes,
				 "Linear allocator wanted to allocate %d bytes, but it only has %d bytes left.  It needs at least another %d bytes.\n",
				 size_bytes, remaining_bytes, ( size_bytes - remaining_bytes )
		);
	}

	allocator->offset = align_up( allocator->offset, cast( u64 ) alignment );

	void* base_ptr = allocator + sizeof( AllocatorLinearData );
	u8* ptr = cast( u8* ) base_ptr + allocator->offset;

	LinearAllocatorHeader* header = cast( LinearAllocatorHeader* ) ptr;
	header->size_bytes = size_bytes;
	header->line = cast( u32 ) line;
	header->file = file;

	ptr += sizeof( LinearAllocatorHeader );

	allocator->offset += size_bytes + sizeof( LinearAllocatorHeader );

	return ptr;
}

#pragma clang diagnostic pop

void* mem_realloc_linear( void* allocator_data, void* ptr, const u64 newSize, const char* file, const int line ) {
	assert( allocator_data );
	assert( ptr );
	assert( newSize );
	assert( file );
	assert( line );

	unused( allocator_data );
	unused( ptr );
	unused( newSize );
	unused( file );
	unused( line );

	//fatal_error( "Linear allocator does not currently support memory reallocation." );

	return NULL;
}

void mem_free_linear( void* allocator_data, void* ptr, const char* file, const int line ) {
	assert( allocator_data );
	assert( ptr );
	assert( file );
	assert( line );

	unused( allocator_data );
	unused( ptr );
	unused( file );
	unused( line );

	/*fatal_error(
		"Linear allocators do not free individual memory allocations.  " \
		"They are instead reset hollistically.  " \
		"Maybe you want to call Mem_Reset() instead?\n"
	);*/
}

void mem_reset_linear( void* allocator_data ) {
	AllocatorLinearData* allocator = cast( AllocatorLinearData* ) allocator_data;

	assertf( allocator, "Linear allocator MUST be non-NULL." );

	allocator->offset = 0;
}