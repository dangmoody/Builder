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

// Generates a seed for random number generation based on time.
void		random_generate_seed();

// Generates a random float32 inclusive between 0 and 1.
float32		random_float32();

// Generates a random float32 inclusive between 'min' and 'max'.
float32		random_float32( const float32 min, const float32 max );