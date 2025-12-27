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

#include "int_types.h"
#include "dll_export.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

/*
================================================================================================

	Core Math

	Just a collection of math helper functions.

================================================================================================
*/

// Returns true if 'a' is within an epsilon range to 'b', otherwise returns false.
CORE_API bool8	float32_equals( const float32 a, const float32 b );

// Returns true if 'a' is within an epsilon range to 'b', otherwise returns false.
CORE_API bool8	float64_equals( const float64 a, const float64 b );

// Returns whichever value is smallest.
CORE_API u32	min( const u32 a, const u32 b );

// Returns whichever value is largest.
CORE_API u32	max( const u32 a, const u32 b );

// Returns the number of zeros on the left hand side of 'number' when viewed in base 2.
CORE_API s32	get_num_leading_zeros( const u64 number );

// Returns the number of zeros on the right hand side of 'number' when viewed in base 2.
CORE_API s32	get_num_trailing_zeros( const u64 number );

// Returns the number of bits inside 'number' that are set to 1 when viewed in base 2.
CORE_API s32	get_num_set_bits( const u64 number );

// Returns the next power of two that is higher than 'number'.
CORE_API u64	next_power_of_2_up( const u64 number );

// Returns the next multiple of 4 that is higher than or equal to 'number'.
CORE_API u64	next_multiple_of_4_up( const u64 number );
