/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "allocator_linear.h"

#include "core_types.h"
#include "memory_units.h"
#include "debug.h"

#include <stdlib.h>	// malloc, free

/*
================================================================================================

	linear allocator

================================================================================================
*/

struct AllocatorLinearData {
	u64			offset;
	u64			size_bytes;
};

struct LinearAllocatorHeader {
	u64			size_bytes;
	u32			line;
	const char*	file;
};

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

	assertf( allocator->offset + size_bytes <= allocator->size_bytes,
		"Linear allocator wanted to allocate %d bytes, but it only has %d bytes left.  It needs another %d bytes.\n",
		size_bytes, ( allocator->size_bytes - allocator->offset ), ( allocator->size_bytes - size_bytes )
	);

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