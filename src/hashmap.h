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

/*
================================================================================================

	Hashmap

	TODO(TOM): document this

================================================================================================
*/

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

struct LinearAllocator;

constexpr u64 HASHMAP_UNUSED_BUCKET 	= 0U;
constexpr u64 HASHMAP_TOMBSTONE_BUCKET 	= 0xffffffffffffffffU;
constexpr u32 HASHMAP_INVALID_VALUE 	= 0xffffffffU;

struct HashmapBucket {
	u32		keyHi;
	u32		keyLo;
	u32		value;
};

struct Hashmap {
	u32				capacity;
	u32				usageCount;
	u32 			tombstoneCount;
	u32				lastLinearProbe;
	float32			maxUtilisation;
	bool8			shouldGrow;
	HashmapBucket	*buckets;
};

Hashmap	*HM_Create( LinearAllocator *allocator, u32 startingCapacity, float32 normalizedMaxUtilisation = 0.5f, bool8 shouldGrow = true );

void	HM_Reset( Hashmap *map );

// Returns the value associated with the key if the key has a value, otherwise returns 0.
u32		HM_GetValue( const Hashmap *map, const u64 key );

void	HM_SetValue( Hashmap *map, const u64 key, const u32 value );
void	HM_RemoveKey( Hashmap *map, const u64 key );

u64		HM_InternalCombine( const u32 hi, const u32 lo );
u64		HM_InternalCombineAtIndex( const Hashmap *map, const u32 index );
u32		HM_InternalGetLoPart( const u64 key );
u32		HM_InternalGetHiPart( const u64 key );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
