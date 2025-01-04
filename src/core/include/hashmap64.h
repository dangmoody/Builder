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

#define HASHMAP64_UNUSED 0xffffffffffffffffULL

struct Hashmap64 {
	u64			count;
	u64*		keys;
	u64*		values;
};

Hashmap64*		hashmap64_create( u64 count );
void			hashmap64_destroy( Hashmap64* map );

void			hashmap64_reset( Hashmap64* map );

// Returns the value associated with the key if the key has a value, otherwise returns 0.
u64				hashmap64_get_value( const Hashmap64* map, const u64 key );

void			hashmap64_set_value( Hashmap64* map, const u64 key, const u64 value );