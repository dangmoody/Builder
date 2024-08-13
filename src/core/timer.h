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


// returns a timestamp in clock cycles
s64		time_cycles( void );

// returns a timestamp in seconds
float64	time_seconds( void );

// returns a timestamp in milliseconds
float64	time_ms( void );

// returns a timestamp in microseconds
float64	time_us( void );

// returns a timestamp in nanoseconds
float64	time_ns( void );