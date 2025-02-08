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

#ifdef CORE_MEMORY_TRACKING
#include "memory_tracking.h"
#endif // CORE_MEMORY_TRACKING

#include "allocator.h" //TODO(Tom): Move if temp turns back to linear

struct AllocatorGeneric;
struct AllocatorLinearData;

struct Paths;

//TODO: Maybe you should put in on the stack like shutdown?
void mem_allocator_intitialize( Allocator* allocator, u64 total_size );

//
// implicit context
//
constexpr u32 MAX_ALLOCATOR_STACK_SIZE		= 32;
constexpr u32 MAX_ALLOCATOR_RELATIONSHIPS	= 32;

struct CoreContext {
	Allocator*									allocator_stack[MAX_ALLOCATOR_STACK_SIZE];
	u32											current_stack_size;
	Allocator									temp_storage;

	Paths*										paths;

#ifdef CORE_MEMORY_TRACKING
	MemoryTracking*								memory_tracking;
#endif
};

extern CoreContext*								g_core_ptr;

void											core_init( const u64 allocator_size, const u64 temp_storage_size );
void											core_shutdown( void );

void											core_hook( CoreContext* context );

void											mem_set_allocator( Allocator* allocator, void* allocator_data );

// DO NOT CALL THE INTERNAL FUNCTIONS!
// CALL THE NON _INTERNAL ONES BELOW!
void*											mem_alloc_internal( const u64 size );
void*											mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment );
void*											mem_realloc_internal( void* ptr, const u64 size );
void*											mem_realloc_aligned_internal( void* ptr, const u64 size, const MemoryAlignment alignment );
void											mem_free_internal( void* ptr );

void											mem_reset_allocator_internal( void );
void											mem_shutdown_allocator_internal( void );

void											mem_push_allocator( Allocator* allocator );
void											mem_pop_allocator();

#ifdef CORE_MEMORY_TRACKING

// call these ones!
#define mem_alloc( size )							track_allocation_internal( mem_alloc_internal( (size) ), __FUNCTION__, __LINE__ )
#define mem_alloc_aligned( size, alignment )		track_allocation_internal( mem_alloc_aligned_internal( (size), cast( MemoryAlignment, (alignment) ) ), __FUNCTION__, __LINE__ )
#define mem_realloc( ptr, size )					track_allocation_internal( mem_realloc_internal( (ptr), (size) ), __FUNCTION__, __LINE__ ); track_free_internal( (ptr) )
#define mem_realloc_aligned( ptr, size, alignment )	track_allocation_internal( mem_realloc_aligned_internal( (ptr), (size), cast( MemoryAlignment, (alignment) ) ), __FUNCTION__, __LINE__ ); track_free_internal( (ptr) )
#define mem_free( ptr )								mem_free_internal( (ptr) ); track_free_internal( (ptr) ); ptr = NULL

// Resets the current allocator
#define mem_reset()									mem_reset_allocator_internal(); track_free_whole_allocator_internal( /*stop_tracking*/false )
// Shuts down the current allocator
#define mem_shutdown()								mem_shutdown_allocator_internal(); track_free_whole_allocator_internal( /*stop_tracking*/true )

void 												mem_allow_allocator_nuking( bool allow );

#else

// OR call these ones instead!
#define mem_alloc( size )							mem_alloc_internal( (size) )
#define mem_alloc_aligned( size, alignment )		mem_alloc_aligned_internal( (size), cast( MemoryAlignment, (alignment) ) )
#define mem_realloc( ptr, size )					mem_realloc_internal( (ptr), (size) )
#define mem_realloc_aligned( ptr, size, alignment )	mem_realloc_aligned_internal( (ptr), (size), cast( MemoryAlignment, (alignment) ) )
#define mem_free( ptr )								mem_free_internal( (ptr) ); ptr = nullptr

// Resets the current allocator
#define mem_reset()									mem_reset_allocator_internal()
// Shuts down the current allocator
#define mem_shutdown()								mem_shutdown_allocator_internal()
static void 										mem_allow_allocator_nuking( bool allow ) { unused( allow ); }

#endif

inline Allocator*									mem_get_current_allocator() { return g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1]; }