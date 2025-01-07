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

	Generic Allocator

	Generic malloc/free tracker used to help identify which allocations are being made where
	and which ones aren't being cleaned up.

================================================================================================
*/

struct GenericAllocationHeader {
	struct GenericAllocationHeader*	prev;
	struct GenericAllocationHeader*	next;

	void*							ptr;
	u64								size;
	u64								line;
	//char							file[1024];
	const char*						file;
};

struct AllocatorGeneric {
	GenericAllocationHeader*		tail;
	u64								total_alloced_bytes;
};

void	mem_create_generic( const u64 size, void** out_allocator_data );
void	mem_destroy_generic( void* allocator_data );

void*	mem_alloc_generic( void* allocator_data, const u64 size, const char* file, const int line );
void*	mem_alloc_generic_aligned( void* allocator_data, const u64 size, const MemoryAlignment alignment, const char* file, const int line );

void*	mem_realloc_generic( void* allocator_data, void* ptr, const u64 size, const char* file, const int line );

void	mem_free_generic( void* allocator_data, void* ptr, const char* file, const int line );

// Not allowed.
void	mem_reset_generic( void* allocator_data );

void	mem_generic_check_leaks( AllocatorGeneric* allocator );