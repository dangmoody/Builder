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

#include <stdint.h>
#include <float.h>

#include "defer.h"

/*
================================================================================================

	Core Types

================================================================================================
*/

// returns bit position 'x'
#define bit( x )						( 1ULL << (x) )

// returns number of elements in static array
#define count_of( x )					( sizeof( (x) ) / sizeof( (x)[0] ) )

// use this to avoid compiler warning about unused variable if you need to keep it
#define unused( x )						( (void) (x) )

// for loop helper macro
// DM: these exist because I'm getting bored of typing the whole thing out every time and it feels like boiler-plate
#define For( Type, it, start, count )	for ( Type it = (start); it < (count); it++ )

// reverse for loop helper macro
#define RFor( Type, it, start, count )	for ( Type it = (count); it-- > (start); )

// returns the amount of padding required to align x up to the next aligned address
#define padding_up( x, alignment )		( (alignment) - 1 ) & ~( (alignment) - 1 )

// returns the input 'x' that has been aligned up by 'alignment' to the next largest value, in bytes
#define align_up( x, alignment )		( (x) + padding_up( x, alignment ) )

typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;
typedef int64_t		s64;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef float		float32;
typedef double		float64;

typedef u8			bool8;

#define S8_MIN		INT8_MIN
#define S8_MAX		INT8_MAX
#define S16_MIN		INT16_MIN
#define S16_MAX		INT16_MAX
#define S32_MIN		INT32_MIN
#define S32_MAX		INT32_MAX
#define S64_MIN		INT64_MIN
#define S64_MAX		INT64_MAX

#define U8_MAX		UINT8_MAX
#define U16_MAX		UINT16_MAX
#define U32_MAX		UINT32_MAX
#define U64_MAX		UINT64_MAX

#define FLOAT32_MIN	FLT_MIN
#define FLOAT32_MAX	FLT_MAX

#define FLOAT64_MIN	DBL_MIN
#define FLOAT64_MAX	DBL_MAX