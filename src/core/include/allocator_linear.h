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

#pragma once

#include "core_types.h"
#include "memory_units.h"

/*
================================================================================================

	Linear Allocator

	Standard linear allocator that allocates a (usually) large section of memory and then
	returns pointers to addresses that are offset from this pointer when calls to
	mem_alloc_linear() are made.

================================================================================================
*/

struct LinearAllocator {
	u64			offset;
	u64			size_bytes;
	void*		arena;
};

LinearAllocator*	linear_allocator_create( const u64 size_bytes );
void				linear_allocator_destroy( LinearAllocator* allocator );

void*				linear_allocator_alloc( LinearAllocator* allocator, const u64 size_bytes, const MemoryAlignment alignment );

void*				linear_allocator_realloc( LinearAllocator* allocator, void* ptr, const u64 newSize, const MemoryAlignment alignment );

// Not allowed.
void				linear_allocator_free( LinearAllocator* allocator, void* ptr );

u64					linear_allocator_tell( LinearAllocator* allocator );
void				linear_allocator_rewind( LinearAllocator* allocator, u64 previous_position );

// "Resets" the position of the linear allocator back to the start of the memory block.
// All allocations made after calling this will override previously made allocations from this allocator.
void				linear_allocator_reset( LinearAllocator* allocator );

void 				linear_allocator_create_generic_interface( Allocator& out_interface );