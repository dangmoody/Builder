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

#include "hash.h"

#include "typecast.h"
#include "helpers.h"
#include "linear_allocator.h"
#include "string.h"
#include "debug.h"

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

u64 Hash64( const void *data, const u64 length, const u64 seed ) {
	assert( data );

	return XXH64( data, length, seed );
}

u64 HashString( const char *string, const u64 seed ) {
	assert( string );

	return Hash64( string, strlen( string ), seed );
}

u64 HashString( const String *string, const u64 seed ) {
	assert( string );

	return Hash64( string->data, string->count, seed );
}
