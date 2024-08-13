#pragma once

enum MemoryAlignment {
	MEMORY_ALIGNMENT_ONE = 1,
	MEMORY_ALIGNMENT_TWO = 2,
	MEMORY_ALIGNMENT_FOUR = 4,
	MEMORY_ALIGNMENT_EIGHT = 8,
	MEMORY_ALIGNMENT_SIXTEEN = 16
};

// memory conversion helpers
#define MEM_KILOBYTES( x )	( cast( u64 ) (x) * 1000 )
#define MEM_MEGABYTES( x )	( MEM_KILOBYTES( x ) * 1000 )
#define MEM_GIGABYTES( x )	( MEM_MEGABYTES( x ) * 1000 )
