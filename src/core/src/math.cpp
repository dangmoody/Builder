// this include must come first because in here we define HLML_ASSERT to assert
// otherwise we get a compiler error that assert is undefined
#include <debug.h>

#include <core_math.h>

// #include "3rdparty/hlml/hlml.h"

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

u64	next_power_of_2_up( const u64 number ) {
	return ( number == 1 ) ? 1 : 1 << ( 64 - get_num_leading_zeros( number - 1 ) );
}

u64 next_multiple_of_4_up( const u64 number ) {
	return ( number + 3 ) & -4ULL;
}