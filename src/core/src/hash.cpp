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

#include <hash.h>

#include <typecast.inl>
#include <core_helpers.h>
#include <linear_allocator.h>
#include <core_string.h>
#include <debug.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define XXH_ASSUME assert
#define XXH_ASSERT assert
#include "xxhash/xxhash.c"
#pragma clang diagnostic pop

/*
================================================================================================

	Hash

================================================================================================
*/

u32 hash32( const void *data, const u64 length, const u32 seed ) {
	assert( data );

	return XXH32( data, length, seed );
}

u64 hash64( const void *data, const u64 length, const u64 seed ) {
	assert( data );

	return XXH64( data, length, seed );
}

u64 hash_string( const char *string, const u64 seed ) {
	assert( string );

	return hash64( string, strlen( string ), seed );
}

u64 hash_string( const String *string, const u64 seed ) {
	assert( string );

	return hash64( string->data, string->count, seed );
}

struct Hasher {
	XXH64_state_t*	state;
};

Hasher *hasher_create( LinearAllocator *allocator, const u64 seed ) {
	assert( allocator );

	Hasher *hasher = cast( Hasher *, linear_allocator_alloc( allocator, sizeof( Hasher ) ) );
	memset( hasher, 0, sizeof( Hasher ) );

	hasher->state = XXH64_createState();

	hasher_reset( hasher, seed );

	return hasher;
}

void hasher_destroy( Hasher *hasher ) {
	assert( hasher );

	XXH64_freeState( hasher->state );
	hasher->state = NULL;
}

void hasher_reset( Hasher *hasher, const u64 seed ) {
	assert( hasher );

	XXH_errorcode result = XXH64_reset( hasher->state, seed );
	assert( result != XXH_ERROR );
	unused( result );
}

void hasher_hash( Hasher *hasher, const void *ptr, const u64 size ) {
	assert( hasher );

	XXH64_update( hasher->state, ptr, size );
}

u64 hasher_get_hash( Hasher *hasher ) {
	assert( hasher );

	return XXH64_digest( hasher->state );
}

