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

#include <allocator_generic.h>

#include <core_types.h>
#include <memory_units.h>
#include <debug.h>

#include <stdlib.h>	// malloc, free
#include <string.h>	// strlen file names

/*
================================================================================================

	AllocatorGeneric

================================================================================================
*/

void mem_create_generic( const u64 size, void** out_allocator_data ) {
	assert( size );
	assert( out_allocator_data );

	unused( size );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) malloc( sizeof( AllocatorGeneric ) );
	allocator->tail = NULL;
	allocator->total_alloced_bytes = 0;

	*out_allocator_data = allocator;
}

void mem_destroy_generic( void* data ) {
	assert( data );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) data;

	//mem_generic_check_leaks( allocator );

	//assertf( allocator->tail == NULL, "Not all allocations have been freed." );

	free( allocator );
	allocator = NULL;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

void* mem_alloc_generic( void* data, const u64 size, const char* file, const int line ) {
	assert( data );
	assert( size );
	assert( file );
	assert( line );

	//u64 filename_length = strlen( file );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) data;

	u64 header_size = sizeof( GenericAllocationHeader );

	u64 total_allocation_size = header_size + size;

	u8* ptr = cast( u8* ) malloc( total_allocation_size );

	GenericAllocationHeader* header = cast( GenericAllocationHeader* ) ptr;
	header->ptr = ptr + header_size;
	header->size = size;
	//memcpy( header->file, file, filename_length );
	//header->file[filename_length] = 0;
	header->file = file;
	header->line = cast( u64 ) line;
	header->prev = NULL;
	header->next = NULL;

	if ( allocator->tail ) {
		header->prev = allocator->tail;

		allocator->tail->next = header;
	}

	allocator->tail = header;

	allocator->total_alloced_bytes += total_allocation_size;

	return header->ptr;
}

void* mem_alloc_generic_aligned( void* data, const u64 size, const MemoryAlignment alignment, const char* file, const int line ) {
	assert( data );
	assert( size );
	assert( file );
	assert( line );

	//u64 filename_length = strlen( file );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) data;

	u64 header_size = sizeof( GenericAllocationHeader );

	u8* ptr = cast( u8* ) _aligned_malloc( header_size + size, cast( size_t ) alignment );

	GenericAllocationHeader* header = cast( GenericAllocationHeader* ) ptr;
	header->ptr = ptr + header_size;
	header->size = size;
	//memcpy( header->file, file, filename_length );
	//header->file[filename_length] = 0;
	header->file = file;
	header->line = cast( u64 ) line;
	header->prev = NULL;
	header->next = NULL;

	if ( allocator->tail ) {
		header->prev = allocator->tail;

		allocator->tail->next = header;
	}

	allocator->tail = header;

	allocator->total_alloced_bytes += header_size + size;

	return header->ptr;
}

void* mem_realloc_generic( void* data, void* ptr, const u64 size, const char* file, const int line ) {
	assert( data );
	assert( size );
	assert( file );
	assert( line );

	//u64 filename_length = strlen( file );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) data;

	if ( ptr ) {
		GenericAllocationHeader* header = cast( GenericAllocationHeader* ) ptr - 1;

		bool8 reallocingTail = header == allocator->tail;

		GenericAllocationHeader* old_prev = header->prev;
		GenericAllocationHeader* old_next = header->next;

		u8* base_ptr = cast( u8* ) header;

		u64 header_size = sizeof( GenericAllocationHeader );

		allocator->total_alloced_bytes -= header->size;

		base_ptr = cast( u8* ) realloc( base_ptr, header_size + size );

		header = cast( GenericAllocationHeader* ) base_ptr;
		header->ptr = base_ptr + header_size;
		header->size = size;
		//memcpy( header->file, file, filename_length );
		//header->file[filename_length] = 0;
		header->file = file;
		header->line = cast( u64 ) line;
		header->prev = old_prev;
		header->next = old_next;

		if ( old_prev ) {
			old_prev->next = header;
		}

		if ( old_next ) {
			old_next->prev = header;
		}

		if ( reallocingTail ) {
			allocator->tail = header;
		}

		allocator->total_alloced_bytes += size;

		return header->ptr;
	} else {
		return mem_alloc_generic( data, size, file, line );
	}
}

#pragma clang diagnostic pop

void mem_free_generic( void* data, void* ptr, const char* file, const int line ) {
	assert( data );
	assert( ptr );
	assert( file );
	assert( line );

	// TODO(DM): 1/1/2023: instead of just freeing the thing we could potentially add the header to a separate list of freed allocations
	// and then when core shuts down we just free the whole thing there or something
	// but I cant currently think of a good use case for wanting this
	unused( file );
	unused( line );

	AllocatorGeneric* allocator = cast( AllocatorGeneric* ) data;

	GenericAllocationHeader* header = cast( GenericAllocationHeader* ) ptr - 1;

	if ( header->next ) {
		header->next->prev = header->prev;
	}

	if ( header->prev ) {
		header->prev->next = header->next;
	}

	if ( header == allocator->tail ) {
		allocator->tail = header->prev;
	}

	allocator->total_alloced_bytes -= ( header->size + sizeof( GenericAllocationHeader ) );

	free( header );
	header = NULL;
}

void mem_reset_generic( void* allocator_data ) {
	assert( allocator_data );

	unused( allocator_data );

	fatal_error( "Generic allocators do not support resetting." );
}

void mem_generic_check_leaks( AllocatorGeneric* allocator ) {
	assert( allocator );

	GenericAllocationHeader* current_tail = allocator->tail;

	if ( current_tail ) {
		error( "Not all allocations have been freed!\n" );

		while ( current_tail ) {
			error( "Allocation made at %s line %d has not been freed.\n", current_tail->file, current_tail->line );

			current_tail = current_tail->prev;
		}
	}
}