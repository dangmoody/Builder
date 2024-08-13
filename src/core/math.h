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
#include "hlml/hlml.h"


// Returns the number of zeros on the left hand side of 'number' when viewed in base 2.
s32	get_num_leading_zeros( const u64 number );

// Returns the number of zeros on the right hand side of 'number' when viewed in base 2.
s32	get_num_trailing_zeros( const u64 number );

// Returns the number of bits inside 'number' that are set to 1 when viewed in base 2.
s32	get_num_set_bits( const u64 number );

// Returns the next power of two that is higher than 'number'.
u64	next_po2_up( const u64 number );