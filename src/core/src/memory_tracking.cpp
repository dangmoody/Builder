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

#ifdef CORE_MEMORY_TRACKING

#include <memory_tracking.h>

#include <hashmap.h>
#include <allocation_context.h>
#include <debug.h>
#include <array.inl>
#include <typecast.inl>

#include <new.h>
#include <malloc.h>

ScopedFlags::ScopedFlags( u32 new_flags, u32 remove_flags ) {
	assertf( ( new_flags & remove_flags ) == 0, "Can't both add and remove flags" );

	old_flags = g_core_ptr->memory_tracking->flags;
	g_core_ptr->memory_tracking->flags |= new_flags;
	g_core_ptr->memory_tracking->flags &= ~remove_flags;
}

ScopedFlags::~ScopedFlags() {
	g_core_ptr->memory_tracking->flags = old_flags;
}

void init_memory_tracking() {
	g_core_ptr->memory_tracking = cast( MemoryTracking*, malloc( sizeof( MemoryTracking ) ) );

	assert( g_core_ptr->memory_tracking != NULL );
	assert( sizeof( MemoryTracking ) == sizeof( *g_core_ptr->memory_tracking ) );

	g_core_ptr->memory_tracking->flags = 0;

	ScopedFlags scoped_flags( MTF_IGNORE );

	g_core_ptr->memory_tracking->allocator_tracking_lookup = hashmap_create( 32 );

	new( &g_core_ptr->memory_tracking->allocator_tracking_data ) Array<AllocatorTrackingData>();
}

static void set_memory_tracking_flag( MemoryTrackingFlag flag, bool active ) {
	if ( active ) {
		g_core_ptr->memory_tracking->flags |= flag;
	} else {
		g_core_ptr->memory_tracking->flags &= ~flag;
	}
}

bool is_memeory_tracking_flag_active( MemoryTrackingFlag flag ) {
	return ( g_core_ptr->memory_tracking->flags & flag ) != 0;
}

CORE_API void	check_allocator_is_active(Allocator* allocator)
{
	MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
	if (memory_tracking && !is_memeory_tracking_flag_active(MTF_IGNORE))
	{
		assertf(hashmap_get_value(memory_tracking->allocator_tracking_lookup, cast(u64, allocator)) != HASHMAP_INVALID_VALUE,
			"The allocator needs to to be active. Either it's not been initialized or has been cleaned up");
	}
}

void start_tracking_allocator( Allocator* allocator ) {
	ScopedFlags scoped_flags( MTF_IGNORE );

	MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
	assert( hashmap_get_value( memory_tracking->allocator_tracking_lookup, cast( u64, allocator ) ) == HASHMAP_INVALID_VALUE );

	AllocatorTrackingData tracking;
	tracking.allocation_lookup = hashmap_create( 64 );
	tracking.allocator = allocator;

	new( &tracking.allocations ) Array<Allocation>();

	memory_tracking->allocator_tracking_data.add( tracking );
	u64 index = memory_tracking->allocator_tracking_data.count - 1;
	hashmap_set_value( memory_tracking->allocator_tracking_lookup, cast( u64, allocator ), cast( u32, index ) );
}

static AllocatorTrackingData* get_current_tracking_data() {
	MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;

	Allocator* current_allocator = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];

	u32 index = hashmap_get_value( memory_tracking->allocator_tracking_lookup, cast( u64, current_allocator ) );
	assert( index != HASHMAP_INVALID_VALUE );

	return &memory_tracking->allocator_tracking_data[index];
}

void* track_reallocation_internal(void* new_allocation, void* old_allocation, const char* function, const u32 line_number)
{
	track_allocation_internal(new_allocation, function, line_number);
	track_free_internal(old_allocation);
	return new_allocation;
}

void* track_allocation_internal( void* allocation, const char* function, const u32 line_number ) {
	if ( is_memeory_tracking_flag_active( MTF_IGNORE ) ) {
		return allocation;
	}

	ScopedFlags scoped_flags( MTF_IGNORE );
	AllocatorTrackingData* allocator_data = get_current_tracking_data();

	assert( hashmap_get_value( allocator_data->allocation_lookup, cast( u64, allocation ) ) == HASHMAP_INVALID_VALUE );

	Allocation allocation_data = { function, line_number, allocation, is_memeory_tracking_flag_active( MTF_IS_ALLOCATOR ) };

	allocator_data->allocations.add( allocation_data );
	u64 index = allocator_data->allocations.count - 1;

	hashmap_set_value( allocator_data->allocation_lookup, cast( u64, allocation ), cast( u32, index ) );

	return allocation;
}

static void recursively_track_frees( AllocatorTrackingData* allocator_data, void* allocation ) {
	u32 index = hashmap_get_value( allocator_data->allocation_lookup, cast( u64, allocation ) );
	assertf( index != HASHMAP_INVALID_VALUE, "Pointer freed was never allocated in the first place" );

	if ( allocator_data->allocations[index].is_allocator ) {
		MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
		Allocator* allocator = cast( Allocator*, allocator_data->allocations[index].ptr );

		bool allocator_found = false;

		For ( u32, track_index, 0, memory_tracking->allocator_tracking_data.count ) {
			if ( memory_tracking->allocator_tracking_data[track_index].allocator == allocator ) {
				assertf( is_memeory_tracking_flag_active( MTF_ALLOW_ALLOCATOR_NUKING ), "Not safe to remove this allocator: you need to call mem_allow_allocator_nuking if you're sure you're not leaving dangling allocators" );

				allocator_found = true;

				For ( u32, allocator_index, 0, g_core_ptr->current_stack_size ) {
					assertf( g_core_ptr->allocator_stack[allocator_index] != allocator, "Even if you mem_allow_allocator_nuking you can't leave allocators dangling on the stack." );
				}

				//Recursively check all the allocators allocations
				AllocatorTrackingData* allocator_data_from_child_allocation = &memory_tracking->allocator_tracking_data[track_index];

				For ( u32, allocation_index, 0, allocator_data_from_child_allocation->allocations.count ) {
					recursively_track_frees( allocator_data_from_child_allocation, allocator_data_from_child_allocation->allocations[allocation_index].ptr );
				}

				memory_tracking->allocator_tracking_data.swap_remove_at( track_index );
				hashmap_remove_key( memory_tracking->allocator_tracking_lookup, cast( u64, allocator ) );

				break;
			}
		}

		// Note(Tom): see note in start tracking allocator about assuming an allocator is only made up of one single allocation. May not be true
		assert( allocator_found );
		unused( allocator_found );
	}

	allocator_data->allocations.swap_remove_at( index );
	hashmap_remove_key(allocator_data->allocation_lookup, cast(u64, allocation));
	// Patch up the allocators lookup info that got swaped into index's place
	if ( index < allocator_data->allocations.count ) {
		hashmap_set_value( allocator_data->allocation_lookup, cast( u64, allocator_data->allocations[index].ptr ), index );
	}
}

void track_free_internal( void* free ) {
	if ( is_memeory_tracking_flag_active( MTF_IGNORE ) || free == nullptr) {
		return;
	}

	ScopedFlags scoped_flags( MTF_IGNORE );

	AllocatorTrackingData* allocator_data = get_current_tracking_data();
	recursively_track_frees( allocator_data, free );
}

void mem_allow_allocator_nuking( bool allow ) {
	set_memory_tracking_flag( MTF_ALLOW_ALLOCATOR_NUKING, allow );
}

void track_free_whole_allocator_internal( bool stop_tracking ) {
	if ( is_memeory_tracking_flag_active( MTF_IGNORE ) ) {
		return;
	}

	ScopedFlags scoped_flags( MTF_IGNORE );

	unused( stop_tracking );

	AllocatorTrackingData* allocator_data = get_current_tracking_data();
	assert( allocator_data );

	For ( u32, i, 0, allocator_data->allocations.count ) {
		recursively_track_frees( allocator_data, allocator_data->allocations[i].ptr );
	}

	if ( stop_tracking ) {
		MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
		u32 index = hashmap_get_value( memory_tracking->allocator_tracking_lookup, cast( u64, allocator_data->allocator ) );

		AllocatorTrackingData& data = memory_tracking->allocator_tracking_data[index];

		hashmap_destroy( data.allocation_lookup );
		data.allocations.~Array();

		memory_tracking->allocator_tracking_data.swap_remove_at( index );
		hashmap_remove_key( memory_tracking->allocator_tracking_lookup, cast( u64, allocator_data->allocator ) );

		// Fixup the lookup of the swaped in allocator data in this index pos
		if ( index < memory_tracking->allocator_tracking_data.count ) {
			hashmap_set_value( memory_tracking->allocator_tracking_lookup, cast( u64, memory_tracking->allocator_tracking_data[index].allocator ), index );
		}
	}
}

#endif // CORE_MEMORY_TRACKING