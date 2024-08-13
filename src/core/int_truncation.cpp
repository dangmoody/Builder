/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "core_types.h"
#include "debug.h"

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