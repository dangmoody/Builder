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

#define HASHMAP32_UNUSED 0xffffffffUL

struct Hashmap32 {
	u32		count;
	u32*	keys;
	u32*	values;
};

Hashmap32*	hashmap32_create( u32 count );
void		hashmap32_destroy( Hashmap32* map );

void		hashmap32_reset( Hashmap32* map );

// Returns the value associated with the key if the key has a value, otherwise returns 0.
u32			hashmap32_get_value( const Hashmap32* map, const u32 key );

void		hashmap32_set_value( Hashmap32* map, const u32 key, const u32 value );