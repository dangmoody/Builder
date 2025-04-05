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

#include <allocation_context.h>
#include <allocator.h>
#include <allocator_malloc.h>
#include <allocator_linear.h>
#include <cmd_line_args.h>
#include <debug.h>
#include <hashmap.h>
#include <typecast.inl>

#include "core_local.h"

#include <memory.h>	// memcpy

/*
================================================================================================

	allocation context

================================================================================================
*/

extern void core_init_platform();
extern void core_shutdown_platform();

// implicit context
// TODO(DM): 19/1/2023: this should be one per thread
static CoreContext g_core_context = {};
CoreContext* g_core_ptr = nullptr;

void mem_allocator_intitialize( Allocator* allocator, u64 total_size ) {
	//TODO(Tom): Base allocator doesn't have data, it writes nullptr in INIT, so it could have init run many times, so perhaps a specific flag for initted
	assert( !allocator->data );
	{
		/*Big Note(TOM): This tracking assumes two things. One reasonable, one maybe not so
			- There's only one allocation made to create an allocator; the entire mem space is allocated in one continous block
				- I feel like for complex allocators that feature different strategies for different sizes this may not be true.
			- The data returned from init is also the location of the allocation. What I mean by this is there's no sneaky header at the beginning like
			a stb array. x[header]*[arena] where x is the allocation but * is the thing returned to the user. I can't think of a good reason with our interface that would make sense
		*/ 

#ifdef CORE_MEMORY_TRACKING
		ScopedFlags scoped_flags( MTF_IS_ALLOCATOR );
#endif
		allocator->data = allocator->init( total_size );
	}

#ifdef CORE_MEMORY_TRACKING
	start_tracking_allocator( allocator );
#endif
}

static Allocator get_bottom_allocator() {
	Allocator malloc_allocator;
	// Note(Tom): Unlike other allocators malloc is stateless and doesn't need initting
	malloc_allocator_create_generic_interface( malloc_allocator );
	return malloc_allocator;
}

void core_init() {
	constexpr u64 default_temp_storage_size = MEM_MEGABYTES( 64 );

	core_init( default_temp_storage_size );
}

void core_init( const u64 temp_storage_size ) {
	assert( temp_storage_size > 0 );

	g_core_ptr = &g_core_context;

	g_core_context.current_stack_size = 0;
	For ( u32, i, 0, MAX_ALLOCATOR_STACK_SIZE ) {
		g_core_context.allocator_stack[i] = nullptr;
	}

	static Allocator s_bottom_allocator = get_bottom_allocator();
	mem_push_allocator( &s_bottom_allocator );
	
#ifdef CORE_MEMORY_TRACKING
	init_memory_tracking();
#endif

	mem_allocator_intitialize( &s_bottom_allocator, 0 );

	linear_allocator_create_generic_interface( g_core_context.temp_storage );
	mem_allocator_intitialize( &g_core_context.temp_storage, temp_storage_size );

	core_init_platform();
}

void mem_push_allocator( Allocator* allocator ) {
	assert( g_core_ptr );

#ifdef CORE_MEMORY_TRACKING
	check_allocator_is_active(allocator);
#endif

	assert( g_core_ptr->current_stack_size + 1 < MAX_ALLOCATOR_STACK_SIZE );
	g_core_ptr->allocator_stack[g_core_ptr->current_stack_size++] = allocator;
}

void mem_pop_allocator() {
	assert( g_core_ptr );
	assertf( g_core_ptr->current_stack_size > 1, "Cannot pop passed the bottom allocator" );
	g_core_ptr->current_stack_size--;
	g_core_ptr->allocator_stack[g_core_ptr->current_stack_size] = nullptr;
}

Allocator* mem_get_current_allocator() {
	return g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
}

void core_shutdown() {
	assert( g_core_ptr );

	core_shutdown_platform();
}

void core_hook( CoreContext* context ) {
	assert( context );

	g_core_ptr = context;
}

void* mem_alloc_internal( const u64 size ) {
	assert( g_core_ptr );
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->allocate( current->data, size );
}

void* mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment ) {
	assert( g_core_ptr );
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size-1];
	return current->allocate_aligned( current->data, size, alignment );
}

void* mem_realloc_internal( void* ptr, const u64 size ) {
	assert( g_core_ptr );
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->reallocate( current->data, ptr, size );
}

void* mem_realloc_aligned_internal( void* ptr, const u64 size, const MemoryAlignment alignment ) {
	assert( g_core_ptr );
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->reallocate_aligned( current->data, ptr, size, alignment );
}

void mem_free_internal( void* ptr ) {
	assert( g_core_ptr );
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->free( current->data, ptr );
}

void mem_reset_allocator_internal() {
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	current->reset( current->data );
}

void mem_shutdown_allocator_internal() {
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	current->shutdown( current->data );
}