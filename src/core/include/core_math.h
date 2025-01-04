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

#define HLML_ASSERT assert
#if 1
#include "3rdparty/hlml/hlml.h"
#else
#include "3rdparty/hlml/bool2.h"
#include "3rdparty/hlml/bool3.h"
#include "3rdparty/hlml/bool3.h"
#include "3rdparty/hlml/int2.h"
#include "3rdparty/hlml/int3.h"
#include "3rdparty/hlml/int3.h"
#include "3rdparty/hlml/uint2.h"
#include "3rdparty/hlml/uint3.h"
#include "3rdparty/hlml/uint3.h"
#include "3rdparty/hlml/float2.h"
#include "3rdparty/hlml/float3.h"
#include "3rdparty/hlml/float3.h"
#include "3rdparty/hlml/double2.h"
#include "3rdparty/hlml/double3.h"
#include "3rdparty/hlml/double3.h"

#include "3rdparty/hlml/bool2x2.h"
#include "3rdparty/hlml/bool2x3.h"
#include "3rdparty/hlml/bool2x4.h"
#include "3rdparty/hlml/bool3x2.h"
#include "3rdparty/hlml/bool3x3.h"
#include "3rdparty/hlml/bool3x4.h"
#include "3rdparty/hlml/bool4x2.h"
#include "3rdparty/hlml/bool4x3.h"
#include "3rdparty/hlml/bool4x4.h"
#include "3rdparty/hlml/int2x2.h"
#include "3rdparty/hlml/int2x3.h"
#include "3rdparty/hlml/int2x4.h"
#include "3rdparty/hlml/int3x2.h"
#include "3rdparty/hlml/int3x3.h"
#include "3rdparty/hlml/int3x4.h"
#include "3rdparty/hlml/int4x2.h"
#include "3rdparty/hlml/int4x3.h"
#include "3rdparty/hlml/int4x4.h"
#include "3rdparty/hlml/uint2x2.h"
#include "3rdparty/hlml/uint2x3.h"
#include "3rdparty/hlml/uint2x4.h"
#include "3rdparty/hlml/uint3x2.h"
#include "3rdparty/hlml/uint3x3.h"
#include "3rdparty/hlml/uint3x4.h"
#include "3rdparty/hlml/uint4x2.h"
#include "3rdparty/hlml/uint4x3.h"
#include "3rdparty/hlml/uint4x4.h"
#include "3rdparty/hlml/float2x2.h"
#include "3rdparty/hlml/float2x3.h"
#include "3rdparty/hlml/float2x4.h"
#include "3rdparty/hlml/float3x2.h"
#include "3rdparty/hlml/float3x3.h"
#include "3rdparty/hlml/float3x4.h"
#include "3rdparty/hlml/float4x2.h"
#include "3rdparty/hlml/float4x3.h"
#include "3rdparty/hlml/float4x4.h"
#include "3rdparty/hlml/double2x2.h"
#include "3rdparty/hlml/double2x3.h"
#include "3rdparty/hlml/double2x4.h"
#include "3rdparty/hlml/double3x2.h"
#include "3rdparty/hlml/double3x3.h"
#include "3rdparty/hlml/double3x4.h"
#include "3rdparty/hlml/double4x2.h"
#include "3rdparty/hlml/double4x3.h"
#include "3rdparty/hlml/double4x4.h"

#include "3rdparty/hlml/hlml_constants.h"
#include "3rdparty/hlml/hlml_constants_sse.h"
#include "3rdparty/hlml/hlml_defines.h"
#include "3rdparty/hlml/hlml_functions_scalar.h"
#include "3rdparty/hlml/hlml_functions_scalar_sse.h"
#include "3rdparty/hlml/hlml_functions_vector.h"
#include "3rdparty/hlml/hlml_functions_vector_sse.h"
#include "3rdparty/hlml/hlml_functions_quaternion.h"
#include "3rdparty/hlml/hlml_functions_matrix.h"
#endif


// Returns the number of zeros on the left hand side of 'number' when viewed in base 2.
s32	get_num_leading_zeros( const u64 number );

// Returns the number of zeros on the right hand side of 'number' when viewed in base 2.
s32	get_num_trailing_zeros( const u64 number );

// Returns the number of bits inside 'number' that are set to 1 when viewed in base 2.
s32	get_num_set_bits( const u64 number );

// Returns the next power of two that is higher than 'number'.
u64	next_power_of_2_up( const u64 number );

// Returns the next multiple of 4 that is higher than or equal to 'number'.
u64 next_multiple_of_4_up( const u64 number );