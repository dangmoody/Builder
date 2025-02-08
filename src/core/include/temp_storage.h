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


void							mem_reset_temp_storage( void );
u64								mem_tell_temp_storage( void );
void							mem_rewind_temp_storage( const u64 position );

// DONT CALL THESE DIRECTLY
void*							mem_temp_alloc_internal( const u64 size );
void*							mem_temp_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment );

// call these instead
#define mem_temp_alloc( size )						mem_temp_alloc_internal( size )
#define mem_temp_alloc_aligned( size, alignment )	mem_temp_alloc_aligned_internal( size, cast( MemoryAlignment, (alignment) ) )