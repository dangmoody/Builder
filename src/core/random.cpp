/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "random.h"

#include <stdlib.h>
#include <time.h>

/*
================================================================================================

	Random

================================================================================================
*/

void random_generate_seed() {
	srand( cast( u32 ) time( NULL ) );
}

float32 random_float32() {
	return random_float32( 0.0f, 1.0f );
}

float32 random_float32( const float32 min, const float32 max ) {
	return min + cast( float32 )( rand() ) / ( cast( float32 ) ( RAND_MAX / ( max - min ) ) );
}