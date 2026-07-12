/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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

#include "int_types.h"
#include "dll_export.h"

/*
================================================================================================

	Linear Allocator

	Virtual Memory-based linear allocator.

	When the allocator is about to become full, it creates a new page on the end.

	Pages are stored in virtual memory via linked lists, so all allocations are contiguous.

	Uses ideas discussed by:
	* Niklas Gray: https://ruby0x1.github.io/machinery_blog_archive/post/virtual-memory-tricks/index.html
	* Casey Muratori: Handmade Hero
	* Jonathan Blow: Jai Compiler
	* Ryan Fleury: https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator

================================================================================================
*/

struct LinearAllocator {
	u8	*ptr;
	u64	offset;
	u64	reserved_bytes;
	u64	comitted_bytes;
	u64	virtual_memory_page_size;
};

CORE_API LinearAllocator	*linear_allocator_create( const u64 reserved_bytes );

CORE_API void				linear_allocator_destroy( LinearAllocator *allocator );

CORE_API void				*linear_allocator_alloc( LinearAllocator *allocator, const u64 size_bytes, const u32 alignment = 8 );

CORE_API void				linear_allocator_reset( LinearAllocator *allocator );

CORE_API u64				linear_allocator_tell( LinearAllocator *allocator );

CORE_API void				linear_allocator_rewind_to( LinearAllocator *allocator, const u64 offset );

CORE_API void				linear_allocator_rewind_by( LinearAllocator *allocator, const u64 bytes );
