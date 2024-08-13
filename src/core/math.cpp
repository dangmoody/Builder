#pragma once

#include "math.h"

/*
================================================================================================

	Maths

================================================================================================
*/

s32 get_num_leading_zeros( const u64 number ) {
	return __builtin_clzll( number );
}

s32 get_num_trailing_zeros( const u64 number ) {
	return __builtin_ctzll( number );
}

s32 get_num_set_bits( const u64 number ) {
	return __builtin_popcountll( number );
}

u64	next_po2_up( const u64 number ) {
	return ( number == 1 ) ? 1 : 1 << ( 64 - get_num_leading_zeros( number - 1 ) );
}