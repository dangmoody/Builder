/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "core_types.h"

// This macro is used by Shaft to turn this:
//
//	u64 myHash = SHAFT_HASH_STATIC( "the hash" );
//
// into this:
//
//	u64 myHash = /*SHAFT_HASH_STATIC( "the hash" )*/1065184032827641794;
//
// If you want to change the hash string and update the value, you will need to:
//	* Revert the code back to just the macro.
//	* Change the string.
//	* Re-run Shaft.
#define SHAFT_HASH_STATIC( str )	0


// Creates a hash based on the given 32 bit 'data' that is 'length' bytes long.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
u32	hash32( const void* data, const u64 length, const u32 seed );

// Creates a hash based on the given 64 bit 'data' that is 'length' bytes long.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
u64	hash64( const void* data, const u64 length, const u64 seed );

// Returns a 64 bit hash based on the given string.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
u64	hash_string( const char* string, const u64 seed );


struct Hasher;

Hasher*		hasher_create( const u64 seed );
void		hasher_destroy( Hasher* hasher );

void		hasher_reset( Hasher* hasher, const u64 seed );

// DM: dont think generalizing this to a pointer here is actually the best thing to do
// lets just have the following functions:
//
//	hasher_hash1
//	hasher_hash2
//	hasher_hash4
//	hasher_hash8
//	hasher_hash16
//	hasher_hash32
//	hasher_hash64
void		hasher_hash( Hasher* hasher, const void* ptr, const u64 size );

u64			hasher_get( Hasher* hasher );