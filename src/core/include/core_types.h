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

#include "defer.h"

// #define HLML_ASSERT assert
// #include "hlml/bool2.h"
// #include "hlml/bool3.h"
// #include "hlml/bool4.h"
// #include "hlml/int2.h"
// #include "hlml/int3.h"
// #include "hlml/int4.h"
// #include "hlml/uint2.h"
// #include "hlml/uint3.h"
// #include "hlml/uint4.h"
// #include "hlml/float2.h"
// #include "hlml/float3.h"
// #include "hlml/float4.h"
// #include "hlml/double2.h"
// #include "hlml/double3.h"
// #include "hlml/double4.h"

// #include "hlml/bool2x2.h"
// #include "hlml/bool2x3.h"
// #include "hlml/bool2x4.h"
// #include "hlml/bool3x2.h"
// #include "hlml/bool3x3.h"
// #include "hlml/bool3x4.h"
// #include "hlml/bool4x2.h"
// #include "hlml/bool4x3.h"
// #include "hlml/bool4x4.h"
// #include "hlml/int2x2.h"
// #include "hlml/int2x3.h"
// #include "hlml/int2x4.h"
// #include "hlml/int3x2.h"
// #include "hlml/int3x3.h"
// #include "hlml/int3x4.h"
// #include "hlml/int4x2.h"
// #include "hlml/int4x3.h"
// #include "hlml/int4x4.h"
// #include "hlml/uint2x2.h"
// #include "hlml/uint2x3.h"
// #include "hlml/uint2x4.h"
// #include "hlml/uint3x2.h"
// #include "hlml/uint3x3.h"
// #include "hlml/uint3x4.h"
// #include "hlml/uint4x2.h"
// #include "hlml/uint4x3.h"
// #include "hlml/uint4x4.h"
// #include "hlml/float2x2.h"
// #include "hlml/float2x3.h"
// #include "hlml/float2x4.h"
// #include "hlml/float3x2.h"
// #include "hlml/float3x3.h"
// #include "hlml/float3x4.h"
// #include "hlml/float4x2.h"
// #include "hlml/float4x3.h"
// #include "hlml/float4x4.h"
// #include "hlml/double2x2.h"
// #include "hlml/double2x3.h"
// #include "hlml/double2x4.h"
// #include "hlml/double3x2.h"
// #include "hlml/double3x3.h"
// #include "hlml/double3x4.h"
// #include "hlml/double4x2.h"
// #include "hlml/double4x3.h"
// #include "hlml/double4x4.h"

/*
================================================================================================

	Core Types

================================================================================================
*/

// typecast
#define cast( x )					(x)

// returns bit position 'x'
#define bit( x )					( 1ULL << (x) )

// returns number of elements in static array
#define count_of( x )				( sizeof( (x) ) / sizeof( (x)[0] ) )

// use this to avoid compiler warning about unused variable if you need to keep it
#define unused( x )					( cast( void ) (x) )

// for loop helper macro
// DM: these exist because I'm getting bored of typing the whole thing out every time and it feels like boiler-plate
#define For( T, it, start, count )	for ( T it = (start); it < (count); it++ )

// reverse for loop helper macro
#define RFor( T, it, start, count )	for ( T it = (count); it-- > (start); )

// returns the amount of padding required to align x up to the next aligned address
#define padding_up(x, alignment)	( (alignment) - 1 )  & ~( (alignment) - 1 )

// returns the input 'x' that has been aligned up by 'alignment' to the next largest value, in bytes
#define align_up( x, alignment )	( (x) +  padding_up(x, alignment) )

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

// TODO(DM): these are shit and ugly
// replace these with trunc_cast<T>
s32					TruncS64ToS32( const s64 x );
s16					TruncS64ToS16( const s64 x );
s8					TruncS64ToS8( const s64 x );
s16					TruncS32ToS16( const s32 x );
s8					TruncS32ToS8( const s32 x );
s8					TruncS16ToS8( const s16 x );
u32					TruncU64ToU32( const u64 x );
u16					TruncU64ToU16( const u64 x );
u8					TruncU64ToU8( const u64 x );
s32					TruncU64ToS32( const u64 x );
s32					TruncU32ToS32( const u32 x );
u16					TruncU32ToU16( const u32 x );
u8					TruncU32ToU8( const u32 x );
u8					TruncU16ToU8( const u16 x );