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

#include <linear_allocator.h>
#include <typecast.inl>

#include <debug.h>

/*
================================================================================================

	temp storage

================================================================================================
*/

LinearAllocator *g_temp_storage = NULL;

void mem_init_temp_storage( const u64 size_bytes ) {
	g_temp_storage = linear_allocator_create( size_bytes );
}

void mem_shutdown_temp_storage() {
	linear_allocator_destroy( g_temp_storage );
	g_temp_storage = NULL;
}

void* mem_temp_alloc( const u64 size_bytes, const u32 alignment ) {
	return linear_allocator_alloc( g_temp_storage, size_bytes, alignment );
}

void mem_reset_temp_storage() {
	linear_allocator_reset( g_temp_storage );
}
