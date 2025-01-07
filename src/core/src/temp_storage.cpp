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