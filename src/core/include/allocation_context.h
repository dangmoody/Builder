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

struct AllocatorGeneric;
struct AllocatorLinearData;

struct Paths;

struct Allocator {
	void	( *init )( const u64 max_size, void** out_data );
	void	( *shutdown )( void* allocator_data );
	void*	( *allocate )( void* allocator_data, const u64 size, const char* file, const int line );
	void*	( *allocate_aligned )( void* allocator_data, const u64 size, const MemoryAlignment alignment, const char* file, const int line );
	void*	( *reallocate )( void* allocator_data, void* ptr, const u64 new_size, const char* file, const int line );
	void	( *free )( void* allocator_data, void* ptr, const char* file, const int line );
	void	( *reset )( void* allocator_data );
};

// implicit context
struct CoreContext {
	Allocator*									allocator;
	void*										allocator_data;

	Allocator*									temp_storage;
	void*										temp_storage_data;

	Paths*										paths;
};

extern CoreContext								g_core_context;

extern Allocator								g_default_allocator;
extern Allocator								g_default_temp_storage;

extern void*									g_default_allocator_data;
extern void*									g_default_temp_storage_data;

void											core_init( const u64 allocator_size, const u64 temp_storage_size );
void											core_shutdown( void );

void											core_hook( CoreContext* context );

void											mem_set_allocator( Allocator* allocator, void* allocator_data );

// DO NOT CALL THE INTERNAL FUNCTIONS
void*											mem_alloc_internal( const u64 size, const char* file, const int line );
void*											mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment, const char* file, const int line );
void*											mem_realloc_internal( void* ptr, const u64 size, const char* file, const int line );
void											mem_free_internal( void* ptr, const char* file, const int line );

// call these ones instead!
#define mem_alloc( size )						mem_alloc_internal( (size), __FILE__, __LINE__ )
#define mem_alloc_aligned( size, alignment )	mem_alloc_aligned_internal( (size), cast( MemoryAlignment ) (alignment), __FILE__, __LINE__ )
#define mem_realloc( ptr, size )				mem_realloc_internal( (ptr), (size), __FILE__, __LINE__ )
#define mem_free( ptr )							mem_free_internal( (ptr), __FILE__, __LINE__ )