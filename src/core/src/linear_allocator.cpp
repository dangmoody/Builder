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

#include "typecast.inl"
#include "core_helpers.h"

#include <malloc.h>

LinearAllocator* linear_allocator_create( const u64 size_bytes ) {
	assert( size_bytes );

	LinearAllocator* allocator = cast( LinearAllocator*, malloc( sizeof( LinearAllocator ) + size_bytes ) );

	allocator->offset = 0;
	allocator->size_bytes = 0;
	allocator->ptr = cast( u8*, allocator + sizeof( u64 ) + sizeof( u64 ) );

	return allocator;
}

void linear_allocator_destroy( LinearAllocator* allocator ) {
	assert( allocator );

	free( allocator );
	allocator = NULL;
}

u8* linear_allocator_alloc( LinearAllocator* allocator, const u64 size_bytes, const u32 alignment ) {
	assert( allocator );
	assert( size_bytes );

	allocator->offset = align_up( allocator->offset, alignment );

	u8* ptr = allocator->ptr + allocator->offset;

	allocator->offset += size_bytes;

	return ptr;
}

void linear_allocator_reset( LinearAllocator* allocator ) {
	allocator->offset = 0;
}

u64 linear_allocator_tell( LinearAllocator* allocator ) {
	return allocator->offset;
}

void linear_allocator_rewind_to( LinearAllocator* allocator, const u64 offset ) {
	allocator->offset = offset;
}

void linear_allocator_rewind_by( LinearAllocator* allocator, const u64 bytes ) {
	allocator->offset -= bytes;
}