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

#include <core_types.h>
#include <debug.h>

/*
================================================================================================

	int truncation

================================================================================================
*/

s32 TruncS64ToS32( const s64 x ) {
	assert( x < S32_MAX );
	return cast( s32 ) x;
}

s16 TruncS64ToS16( const s64 x ) {
	assert( x < S16_MAX );
	return cast( s16 ) x;
}

s8 TruncS64ToS8( const s64 x ) {
	assert( x < S8_MAX );
	return cast( s8 ) x;
}

s16 TruncS32ToS16( const s32 x ) {
	assert( x < S16_MAX );
	return cast( s16 ) x;
}

s16 TruncS32ToS8( const s32 x ) {
	assert( x < S8_MAX );
	return cast( s8 ) x;
}

s8 TruncS16ToS8( const s16 x ) {
	assert( x < S8_MAX );
	return cast( s8 ) x;
}

u32 TruncU64ToU32( const u64 x ) {
	assert( x < U32_MAX );
	return cast( u32 ) x;
}

u16 TruncU64ToU16( const u64 x ) {
	assert( x < U16_MAX );
	return cast( u16 ) x;
}

u8 TruncU64ToU8( const u64 x ) {
	assert( x < U8_MAX );
	return cast( u8 ) x;
}

s32 TruncU64ToS32( const u64 x ) {
	assert( x < S32_MAX );
	return cast( s32 ) x;
}

s32 TruncU32ToS32( const u32 x ) {
	assert( x < S32_MAX );
	return cast( s32 ) x;
}

u16 TruncU32ToU16( const u32 x ) {
	assert( x < U16_MAX );
	return cast( u16 ) x;
}

u8 TruncU32ToU8( const u32 x ) {
	assert( x < U8_MAX );
	return cast( u8 ) x;
}

u8 TruncU16ToU8( const u16 x ) {
	assert( x < U8_MAX );
	return cast( u8 ) x;
}