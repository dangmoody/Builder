/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include <temp_storage.h>
#include <allocation_context.h>
#include <allocator_linear.h>

/*
================================================================================================

	temp storage

================================================================================================
*/

void* g_default_temp_storage_data = NULL;

Allocator g_default_temp_storage = {
	.init				= mem_create_linear,
	.shutdown			= mem_destroy_linear,
	.allocate			= mem_alloc_linear,
	.allocate_aligned	= mem_alloc_linear_aligned,
	.free				= mem_free_linear,
	.reset				= mem_reset_linear,
};

void mem_reset_temp_storage() {
	g_core_context.temp_storage->reset( g_core_context.temp_storage_data );
}

void* mem_temp_alloc_internal( const u64 size, const char* file, const int line ) {
	return g_core_context.temp_storage->allocate( g_core_context.temp_storage_data, size, file, line );
}