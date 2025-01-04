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
#include "array.h"

struct Process;


Process*	process_create( Array<const char*>* args, Array<const char*>* environment_variables );

void		process_destroy( Process* process );

s32			process_join( Process** process );

u64			process_read_stdout( Process* process, char* out_buffer, const u32 count );