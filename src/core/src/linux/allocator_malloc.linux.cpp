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

#include <allocator_malloc.h>

#include <allocation_context.h>
#include <core_types.h>
#include <cstdlib>
#include <memory_units.h>
#include <debug.h>
#include <typecast.inl>

#include <stdlib.h>	// malloc, free
#include <string.h>	// memcpy

static void* aligned_malloc_internal( const size_t size, const MemoryAlignment alignment ) {
	unused( alignment );
	return malloc(size);
}

static void* aligned_realloc_internal( void* ptr, const size_t size, MemoryAlignment alignment ) {
	unused( alignment );
	return realloc(ptr, size);
}

/*
================================================================================================

	Malloc Allocator: Linux implementations for wrapping malloc and free

================================================================================================
*/

void* malloc_allocator_create( const u64 size) {
	unused( size );

	// info( "It's unneccessary to create a malloc_allocator. It is stateless, therefore needs no creation." );

	return nullptr;
}

void malloc_allocator_destroy( void* data ) {
	assertf( !data, "Nothing is destroyed because malloc_allocator is stateless" );

	unused( data );
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

void* malloc_allocator_alloc( void* allocator_data, const u64 size, const MemoryAlignment alignment ) {
	assertf( !allocator_data, "Malloc_allocator is stateless" );
	assert( size );

	unused( allocator_data );

	void* ptr = aligned_malloc_internal( size, alignment );

	if ( !ptr ) {
		error( "malloc failed to allocate" );
	}

	return ptr;
}

void* malloc_allocator_realloc( void* allocator_data, void* ptr, const u64 new_size, const MemoryAlignment alignment ) {
	assertf( !allocator_data, "Malloc_allocator is stateless" );

	unused( allocator_data );
	unused(alignment);

	// if ( !ptr ) {
	// 	warning( "Null ptr cannot be realloced" );
	// 	return nullptr;
	// }

	return aligned_realloc_internal( ptr, new_size, alignment );
}

#pragma clang diagnostic pop

void malloc_allocator_free( void* allocator_data, void* ptr ){
	assertf( !allocator_data, "Malloc_allocator is stateless" );

	unused( allocator_data );

	free( ptr );
}

void malloc_allocator_reset( void* allocator_data ) {
	assertf( !allocator_data, "Malloc_allocator is stateless" );

	unused( allocator_data );

	fatal_error( "Generic allocators do not support resetting." );
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
void malloc_allocator_create_generic_interface( Allocator& out_interface ) {
	out_interface.init = cast( allocator_init, &malloc_allocator_create );
	out_interface.shutdown = cast( allocator_shutdown, &malloc_allocator_destroy );

	out_interface.allocate_aligned = cast( allocator_allocate_aligned, &malloc_allocator_alloc );
	out_interface.reallocate_aligned = cast( allocator_reallocate_aligned, &malloc_allocator_realloc );

	out_interface.free = cast( allocator_free, &malloc_allocator_free );
	out_interface.reset = cast( allocator_reset, &malloc_allocator_reset );

	// Malloc wrapper has no book keeping
	out_interface.data = NULL;
}
#pragma clang diagnostic pop
