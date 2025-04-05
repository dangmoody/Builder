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
CORE_API s32	get_num_leading_zeros( const u64 number );

// Returns the number of zeros on the right hand side of 'number' when viewed in base 2.
CORE_API s32	get_num_trailing_zeros( const u64 number );

// Returns the number of bits inside 'number' that are set to 1 when viewed in base 2.
CORE_API s32	get_num_set_bits( const u64 number );

// Returns the next power of two that is higher than 'number'.
CORE_API u64	next_power_of_2_up( const u64 number );

// Returns the next multiple of 4 that is higher than or equal to 'number'.
CORE_API u64	next_multiple_of_4_up( const u64 number );