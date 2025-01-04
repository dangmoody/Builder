/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include <hashmap32.h>
#include <hashmap64.h>

#include <debug.h>
#include <allocation_context.h>

#include <memory.h>	// memset

/*
================================================================================================

	Hashmap32

================================================================================================
*/

Hashmap32* hashmap32_create( const u32 count ) {
	assert( count );

	Hashmap32* map = cast( Hashmap32* ) mem_alloc( sizeof( Hashmap32 ) );
	map->count = count;
	map->keys = cast( u32* ) mem_alloc( count * sizeof( u32 ) );
	map->values = cast( u32* ) mem_alloc( count * sizeof( u32 ) );

	hashmap32_reset( map );

	return map;
}

void hashmap32_destroy( Hashmap32* map ) {
	assert( map );

	mem_free( map->values );
	map->values = NULL;

	mem_free( map->keys );
	map->keys = NULL;

	mem_free( map );
	map = NULL;
}

void hashmap32_reset( Hashmap32* map ) {
	memset( map->keys, 0xff, map->count * sizeof( u32 ) );
	memset( map->values, 0xff, map->count * sizeof( u32 ) );
}

u32 hashmap32_get_value( const Hashmap32* map, const u32 key ) {
	u32 i = key % map->count;

	while ( map->keys[i] != key && map->keys[i] != HASHMAP32_UNUSED ) {
		i = ( i + 1 ) % map->count;
	}

	return map->values[i] == HASHMAP32_UNUSED ? 0 : map->values[i];
}

void hashmap32_set_value( Hashmap32* map, const u32 key, const u32 value ) {
	u32 i = key % map->count;

	while ( map->keys[i] != key && map->keys[i] != HASHMAP32_UNUSED ) {
		i = ( i + 1 ) % map->count;
	}

	map->keys[i] = key;
	map->values[i] = value;
}


/*
================================================================================================

	Hashmap64

================================================================================================
*/

Hashmap64* hashmap64_create( u64 count ) {
	assert( count );

	Hashmap64* map = cast( Hashmap64* ) mem_alloc( sizeof( Hashmap64 ) );
	map->count = count;
	map->keys = cast( u64* ) mem_alloc( count * sizeof( u64 ) );
	map->values = cast( u64* ) mem_alloc( count * sizeof( u64 ) );

	hashmap64_reset( map );

	return map;
}

void hashmap64_destroy( Hashmap64* map ) {
	assert( map );

	mem_free( map->values );
	map->values = NULL;

	mem_free( map->keys );
	map->keys = NULL;

	mem_free( map );
	map = NULL;
}

void hashmap64_reset( Hashmap64* map ) {
	memset( map->keys, 0xff, map->count * sizeof( u64 ) );
	memset( map->values, 0xff, map->count * sizeof( u64 ) );
}

u64 hashmap64_get_value( const Hashmap64* map, const u64 key ) {
	assert( key );

	u64 i = key % map->count;

	while ( map->keys[i] != key && map->keys[i] != HASHMAP64_UNUSED ) {
		i = ( i + 1 ) % map->count;
	}

	return map->values[i] == HASHMAP64_UNUSED ? 0 : map->values[i];
}

void hashmap64_set_value( Hashmap64* map, const u64 key, const u64 value ) {
	assert( key );

	u64 i = key % map->count;

	while ( map->keys[i] != key && map->keys[i] != HASHMAP64_UNUSED ) {
		i = ( i + 1 ) % map->count;
	}

	map->keys[i] = key;
	map->values[i] = value;
}