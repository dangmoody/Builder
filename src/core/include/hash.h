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
#include "dll_export.h"

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
CORE_API u32		hash32( const void* data, const u64 length, const u32 seed );

// Creates a hash based on the given 64 bit 'data' that is 'length' bytes long.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
CORE_API u64		hash64( const void* data, const u64 length, const u64 seed );

// Returns a 64 bit hash based on the given string.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
CORE_API u64		hash_string( const char* string, const u64 seed );


struct Hasher;

CORE_API Hasher*	hasher_create( const u64 seed );
CORE_API void		hasher_destroy( Hasher* hasher );

CORE_API void		hasher_reset( Hasher* hasher, const u64 seed );

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
CORE_API void		hasher_hash( Hasher* hasher, const void* ptr, const u64 size );

CORE_API u64		hasher_get( Hasher* hasher );