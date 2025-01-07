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

struct AllocatorLinearData {
	u64			offset;
	u64			size_bytes;
};

struct LinearAllocatorHeader {
	u64			size_bytes;
	u32			line;
	const char*	file;
};

void	mem_create_linear( const u64 size_bytes, void** out_allocator_data );
void	mem_destroy_linear( void* allocator_data );

void*	mem_alloc_linear( void* allocator_data, const u64 size_bytes, const char* file, const int line );
void*	mem_alloc_linear_aligned( void* allocator_data, const u64 size_bytes, const MemoryAlignment alignment, const char* file, const int line );

void*	mem_realloc_linear( void* allocator_data, void* ptr, const u64 newSize, const char* file, const int line );

// Not allowed.
void	mem_free_linear( void* allocator_data, void* ptr, const char* file, const int line );

// "Resets" the position of the linear allocator back to the start of the memory block.
// All allocations made after calling this will override previously made allocations from this allocator.
void	mem_reset_linear( void* allocator_data );