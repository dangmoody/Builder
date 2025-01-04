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