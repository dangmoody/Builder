#pragma once
#include "memory_units.h"
#include "core_types.h"

constexpr MemoryAlignment DEFAULT_MEMORY_ALIGNMENT = MEMORY_ALIGNMENT_EIGHT;

typedef void*	( *allocator_init )( const u64 max_size );
typedef void	( *allocator_shutdown )( void* allocator_data );
typedef void*	( *allocator_allocate_aligned )( void* allocator_data, const u64 size, const MemoryAlignment alignment );
typedef void*	( *allocator_reallocate_aligned )( void* allocator_data, void* ptr, const u64 new_size, const MemoryAlignment alignment );
typedef void	( *allocator_free )( void* allocator_data, void* ptr );
typedef void	( *allocator_reset )( void* allocator_data );

struct Allocator {
	allocator_init					init;
	allocator_shutdown				shutdown;
	void*							allocate( void* allocator_data, const u64 size ) { return allocate_aligned( allocator_data, size, DEFAULT_MEMORY_ALIGNMENT ); }
	allocator_allocate_aligned		allocate_aligned;
	void*							reallocate( void* allocator_data, void* ptr, const u64 new_size ) { return reallocate_aligned( allocator_data, ptr, new_size, DEFAULT_MEMORY_ALIGNMENT ); }
	allocator_reallocate_aligned	reallocate_aligned;
	allocator_free					free;
	allocator_reset					reset;

	void*							data = nullptr;
};