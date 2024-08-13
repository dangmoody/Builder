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

struct Library {
	void* ptr;
};

Library	library_load( const char* name );
void	library_unload( Library* library );

void*	library_get_proc_address( const Library library, const char* func_name );