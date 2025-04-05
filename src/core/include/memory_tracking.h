#pragma once

#ifdef CORE_MEMORY_TRACKING

#include "core_types.h"
#include "array.h"
#include "dll_export.h"

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

CORE_API void	init_memory_tracking();
CORE_API void	start_tracking_allocator( Allocator* allocator );

CORE_API void*	track_allocation_internal( void* allocation, const char* function, u32 line_number );
CORE_API void*	track_reallocation_internal(void* new_allocation, void* old_allocation, const char* function, const u32 line_number);
CORE_API void	track_free_internal( void* free );
CORE_API void	track_free_whole_allocator_internal( bool stop_tracking );

CORE_API void	check_allocator_is_active(Allocator* allocator);

struct ScopedFlags {
		ScopedFlags( u32 new_flags, u32 remove_flags = 0 );
		~ScopedFlags();

	u32	old_flags;
};

#endif // CORE_MEMORY_TRACKING