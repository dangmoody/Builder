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

struct Allocator;

constexpr u64 HASHMAP_UNUSED_BUCKET 	= 0U;
constexpr u64 HASHMAP_TOMBSTONE_BUCKET 	= 0xffffffffffffffffU;
constexpr u32 HASHMAP_INVALID_VALUE 	= 0xffffffffU;

struct HashmapBucket {
	u32		key_hi;
	u32		key_lo;
	u32		value;
};

struct Hashmap {
	u32				capacity;
	u32				usage_count;
	u32 			tombstone_count;
	u32				last_linear_probe;
	float32			max_utilisation;
	bool8			should_grow;
	HashmapBucket*	buckets;
	Allocator*		allocator;
};

Hashmap*	hashmap_create( u32 starting_capacity, float32 normalized_max_utilisation = 0.5f, bool8 should_grow = true);
void		hashmap_destroy( Hashmap* map );

void		hashmap_reset( Hashmap* map );

// Returns the value associated with the key if the key has a value, otherwise returns 0.
u32			hashmap_get_value( const Hashmap* map, const u64 key );

void		hashmap_set_value( Hashmap* map, const u64 key, const u32 value );
void		hashmap_remove_key( Hashmap* map, const u64 key );

constexpr inline u64 hashmap_internal_combine( u32 hi, u32 lo ) {
	return ( cast( u64, lo ) << 32 ) | hi;
}

inline u64 hashmap_internal_combine_at_index( const Hashmap* map, u32 index ) {
	return hashmap_internal_combine( map->buckets[index].key_hi, map->buckets[index].key_lo );
}

constexpr inline u32 hashmap_internal_get_lo_part( u64 key ) {
	return cast( u32, key >> 32 );
}

constexpr inline u32 hashmap_internal_get_hi_part( u64 key ) {
	return cast( u32, key & 0xFFFFFFFF );
}