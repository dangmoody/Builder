#pragma once

#include "core_types.h"
#include "debug.h"

#include <float.h>

/*
================================================================================================

	Typecasting

================================================================================================
*/

// Call these ones!
#define cast( Type, x )				(Type) (x)
#define trunc_cast( Type, x )		trunc_cast_internal<Type>( (x) )


// DO NOT CALL THESE ONES!
template<class OutType, class InType>
OutType trunc_cast_internal( const InType in ) {
	bool8 is_input_floating_point = cast( InType, 0.5 ) != 0;
	bool8 is_output_floating_point = cast( OutType, 0.5 ) != 0;

	bool8 is_input_signed = cast( InType, -1 ) < 0;
	bool8 is_output_signed = cast( OutType, -1 ) < 0;

	OutType min_output_value = 0;
	OutType max_output_value = 0;

	if ( is_output_floating_point ) {
		min_output_value = cast( OutType, -FLT_MAX );
		max_output_value = cast( OutType, FLT_MAX );
	} else {
		if ( is_output_signed ) {
			max_output_value = cast( OutType, ( 1ULL << ( sizeof( OutType ) * 8 - 1 ) ) - 1 );
			min_output_value = cast( OutType, -max_output_value - 1 );
		} else {
			min_output_value = 0;
			max_output_value = cast( OutType, ~0 );
		}
	}

	if ( is_input_signed ) {
		assert( in >= cast( OutType, min_output_value ) );
	}

	assert( in <= cast( OutType, max_output_value ) );

	return cast( OutType, in );
}