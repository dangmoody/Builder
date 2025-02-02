#pragma once

#ifdef CORE_MEMORY_TRACKING

#include "core_types.h"
#include "array.h"

struct Hashmap;

enum MemoryTrackingFlag {
	MTF_IGNORE					= 1,
	MTF_IS_ALLOCATOR			= 2,
	MTF_ALLOW_ALLOCATOR_NUKING	= 4
};

struct Allocation {
	const char*	function;
	u32			line;
	void*		ptr;
	bool		is_allocator;
};

struct AllocatorTrackingData {
	Array<Allocation>	allocations;
	Hashmap*			allocation_lookup;
	Allocator*			allocator;
};

struct MemoryTracking {
	Hashmap*						allocator_tracking_lookup;
	Array<AllocatorTrackingData>	allocator_tracking_data;
	u32								flags;
};

void			init_memory_tracking();
void			start_tracking_allocator( Allocator* allocator );

void*			track_allocation_internal( void* allocation, const char* function, u32 line_number );
void			track_free_internal( void* free );
void			track_free_whole_allocator_internal( bool stop_tracking );

struct ScopedFlags {
		ScopedFlags( u32 new_flags, u32 remove_flags = 0 );
		~ScopedFlags();

	u32	old_flags;
};

#endif // CORE_MEMORY_TRACKING