/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

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