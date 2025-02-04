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

#pragma once

#include "core_types.h"
#include "memory_units.h"

struct Allocator;

/*
================================================================================================

	Generic Malloc Allocator

	Wrap of malloc/free, this allocator is used as the first allocator on the stack of CoreContext

================================================================================================
*/

void*	malloc_allocator_create( const u64 size );
void	malloc_allocator_destroy( void* allocator_data );

void*	malloc_allocator_alloc( void* allocator_data, const u64 size, const MemoryAlignment alignment );
void*	malloc_allocator_realloc( void* allocator_data, void* ptr, const u64 new_size, const MemoryAlignment alignment );

void	malloc_allocator_free( void* allocator_data, void* ptr );

// Not allowed.
void	malloc_allocator_reset( void* allocator_data );

void 	malloc_allocator_create_generic_interface( Allocator& out_interface );