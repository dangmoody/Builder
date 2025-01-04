/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include <allocation_context.h>
#include <allocator_generic.h>
#include <debug.h>
#include <cmd_line_args.h>

#include "core_local.h"

#include <memory.h>	// memcpy

/*
================================================================================================

	allocation context

================================================================================================
*/

extern void core_init_platform();
extern void core_shutdown_platform();

void* g_default_allocator_data = NULL;

Allocator g_default_allocator = {
	.init				= mem_create_generic,
	.shutdown			= mem_destroy_generic,
	.allocate			= mem_alloc_generic,
	.allocate_aligned	= mem_alloc_generic_aligned,
	.reallocate			= mem_realloc_generic,
	.free				= mem_free_generic,
	.reset				= mem_reset_generic
};

// implicit context
// TODO(DM): 19/1/2023: this should be one per thread
CoreContext g_core_context = {
	.allocator			= &g_default_allocator,
	.temp_storage		= &g_default_temp_storage
};

void core_init( const u64 allocator_size, const u64 temp_storage_size ) {
	assert( allocator_size );
	assert( temp_storage_size );

	// init default allocators
	{
		// TODO(DM): 11/2/2023: still not sure if letting users specify the total allocator size is the right answer
		// or if we want to just let users specify the size of a page and then we make as many as we need etc
		g_default_allocator.init( allocator_size, cast( void** ) &g_default_allocator_data );
		g_default_temp_storage.init( temp_storage_size, cast( void** ) &g_default_temp_storage_data );

		g_core_context.allocator_data = g_default_allocator_data;
		g_core_context.temp_storage_data = g_default_temp_storage_data;
	}

	core_init_platform();
}

void core_shutdown() {
	core_shutdown_platform();

	// shutdown default allocators
	{
		g_default_temp_storage.shutdown( g_default_temp_storage_data );
		g_default_allocator.shutdown( g_default_allocator_data );

		g_default_temp_storage_data = NULL;
		g_default_allocator_data = NULL;
	}
}

void core_hook( CoreContext* context ) {
	assert( context );

	memcpy( &g_core_context, context, sizeof( CoreContext ) );

	g_default_allocator_data = context->allocator_data;
	g_default_temp_storage_data = context->temp_storage_data;
}

void mem_set_allocator( Allocator* allocator, void* allocator_data ) {
	assert( allocator );
	assert( allocator->allocate );
	assert( allocator->allocate_aligned );

	g_core_context.allocator = allocator;
	g_core_context.allocator_data = allocator_data;
}

void* mem_alloc_internal( const u64 size, const char* file, const int line ) {
	return g_core_context.allocator->allocate( g_core_context.allocator_data, size, file, line );
}

void* mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment, const char* file, const int line ) {
	return g_core_context.allocator->allocate_aligned( g_core_context.allocator_data, size, alignment, file, line );
}

void* mem_realloc_internal( void* ptr, const u64 size, const char* file, const int line ) {
	return g_core_context.allocator->reallocate( g_core_context.allocator_data, ptr, size, file, line );
}

void mem_free_internal( void* ptr, const char* file, const int line ) {
	g_core_context.allocator->free( g_core_context.allocator_data, ptr, file, line );
}