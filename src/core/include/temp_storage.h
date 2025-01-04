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


void							mem_reset_temp_storage( void );

// DONT CALL THIS DIRECTLY
void*							mem_temp_alloc_internal( const u64 size, const char* file, const int line );

// call this one instead
#define mem_temp_alloc( size )	mem_temp_alloc_internal( size, __FILE__, __LINE__ )