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

#include <hashmap.h>

#include <core_helpers.h>
#include <defer.h>
#include <debug.h>
#include <typecast.inl>
#include <core_math.h>

#include <memory.h>	// memset
#include <malloc.h>

/*
================================================================================================

	Hashmap

================================================================================================
*/

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif

Hashmap* hashmap_create( const u32 starting_capacity, float32 normalized_max_utilisation, bool8 should_grow ) {
	assert( starting_capacity );
	assert( normalized_max_utilisation > 0.0f );
	assert( normalized_max_utilisation <= 1.0f );

	Hashmap* map = cast( Hashmap*, malloc( sizeof( Hashmap ) ) );
	map->capacity = starting_capacity;
	map->buckets = cast( HashmapBucket*, malloc( starting_capacity * sizeof( HashmapBucket ) ) );
	map->should_grow = should_grow;
	map->max_utilisation = normalized_max_utilisation;

	hashmap_reset( map );

	return map;
}

void hashmap_destroy( Hashmap* map ) {
	assert( map );

	free( map->buckets );
	map->buckets = NULL;

	free( map );
	map = NULL;
}

inline void set_key_at_index( Hashmap* map, u32 index, u64 key ) {
	map->buckets[index].key_hi = hashmap_internal_get_hi_part( key );
	map->buckets[index].key_lo = hashmap_internal_get_lo_part( key );
}

void hashmap_reset( Hashmap* map ) {
	For ( u32, i, 0, map->capacity ) {
		set_key_at_index( map, trunc_cast( u32, i ), HASHMAP_UNUSED_BUCKET );
		map->buckets[i].value = HASHMAP_INVALID_VALUE;
	}

	map->usage_count = 0;
	map->tombstone_count = 0;
}

inline u32 try_get_index_of_hash( const Hashmap* map, const u64 key ) {
	u32 i = key % map->capacity;

	// Note(Tom): I think this is a legit use of const cast since it's purely for telemetry
	const_cast<Hashmap*>( map )->last_linear_probe = 0;
	
	u64 recombined_hash = hashmap_internal_combine_at_index( map, i );

	while ( recombined_hash != key && recombined_hash != HASHMAP_UNUSED_BUCKET && map->last_linear_probe < map->capacity ) {
		i = ( i + 1 ) % map->capacity;
		recombined_hash = hashmap_internal_combine_at_index( map, i );
		const_cast<Hashmap*>( map )->last_linear_probe++;
	}

	return i;
}

u32 hashmap_get_value( const Hashmap* map, const u64 key ) {
	assert( key != HASHMAP_UNUSED_BUCKET && "Key cannot equal empty bucket value (0)" );
	assert( key != HASHMAP_TOMBSTONE_BUCKET && "Key cannot equal Tombstone (u32 MAX)" );

	u32 i = try_get_index_of_hash( map, key );

	if ( hashmap_internal_combine_at_index( map, i ) != key ) {
#ifndef HASHMAP_HIDE_MISSING_KEY_WARNING
		warning( "GET: Key %llu not found in hashmap\n", key );
#endif
		return HASHMAP_INVALID_VALUE;
	}

	return map->buckets[i].value;
}

void hashmap_set_value( Hashmap* map, const u64 key, const u32 value ) {
	u32 i = try_get_index_of_hash( map, key );
	u64 key_at_location = hashmap_internal_combine_at_index( map, i );

	if ( key_at_location != key && key_at_location != HASHMAP_UNUSED_BUCKET ) {
		warning( "SET: Key %llu or empty space not found in hashmap\n", key );
		return;
	}

	if ( key_at_location == HASHMAP_UNUSED_BUCKET ) {
		map->usage_count++;

		float32 utilization = cast( float32, map->usage_count + map->tombstone_count ) / cast( float32, map->capacity );
		if ( utilization > map->max_utilisation ) {
			if ( map->should_grow ) {
				HashmapBucket* old_buckets = map->buckets;
				defer { free( old_buckets ); };

				u32 old_capacity = map->capacity;
				map->capacity = max( cast( u32, cast( float32, map->capacity ) * 1.5f ), 2U );
				map->buckets = cast( HashmapBucket*, malloc( map->capacity * sizeof( HashmapBucket ) ) );
				// Note(Tom): I don't love that this isn't a memset anymore. this isn's possible if we keep caring about values of unused buckets: unused value != empty bucket.
				// I suggest we start leaving them untouched. Yes they have stale old data in them, but so long as people are using set that should never be an issue
				// (we're testing this right?)
				// Alternatively we could swap tombstone and unused values round and then unused value == empty bucket and zero the entire damn thing :)
				hashmap_reset( map );

				For ( u32, old_bucket_index, 0, old_capacity ) {
					u64 key_in_bucket = hashmap_internal_combine( old_buckets[old_bucket_index].key_hi, old_buckets[old_bucket_index].key_lo );
					if ( key_in_bucket != HASHMAP_UNUSED_BUCKET && key_in_bucket != HASHMAP_TOMBSTONE_BUCKET ) {
						hashmap_set_value( map, key_in_bucket, old_buckets[old_bucket_index].value );
					}
				}

				// Finally add this one
				hashmap_set_value( map, key, value );
				return;
			} else {
				warning( "Hashmap is above utilization of %f with %u buckets", map->max_utilisation, map->capacity );
			}
		}
	}

	set_key_at_index( map, i, key );
	map->buckets[i].value = value;
}

void hashmap_remove_key( Hashmap* map, const u64 key ) {
	assert( key != HASHMAP_UNUSED_BUCKET && "Key cannot equal empty bucket value (0)" );
	assert( key != HASHMAP_TOMBSTONE_BUCKET && "Key cannot equal Tombstone (u32 MAX)" );

	u32 i = try_get_index_of_hash( map, key );
	u64 key_at_location = hashmap_internal_combine_at_index( map, i );

	if ( key_at_location != key ) {
		warning( "REMOVE: Key %llu not found in hashmap\n", key );
		return;
	}

	u32 next = ( i + 1 ) % map->capacity;
	if ( hashmap_internal_combine_at_index( map, next ) != HASHMAP_UNUSED_BUCKET ) {
		set_key_at_index( map, i, HASHMAP_TOMBSTONE_BUCKET );
		map->tombstone_count++;
	} else {
		set_key_at_index( map, i, HASHMAP_UNUSED_BUCKET );
		i = ( i - 1 ) % map->capacity;

 		while ( hashmap_internal_combine_at_index( map, i ) == HASHMAP_TOMBSTONE_BUCKET ) {
			set_key_at_index( map, i, HASHMAP_UNUSED_BUCKET );
			map->tombstone_count--;
			i = ( i - 1 ) % map->capacity;
		}
	}

	map->usage_count--;
}

u64 hashmap_internal_combine( const u32 hi, const u32 lo ) {
	return ( cast( u64, lo ) << 32 ) | hi;
}

u64 hashmap_internal_combine_at_index( const Hashmap* map, const u32 index ) {
	return hashmap_internal_combine( map->buckets[index].key_hi, map->buckets[index].key_lo );
}

u32 hashmap_internal_get_lo_part( const u64 key ) {
	return trunc_cast( u32, key >> 32 );
}

u32 hashmap_internal_get_hi_part( const u64 key ) {
	return trunc_cast( u32, key & 0xFFFFFFFF );
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
