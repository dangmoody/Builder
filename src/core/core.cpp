/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "int_truncation.cpp"

#if defined( _WIN64 )
#include "core.win64.cpp"
#else
#error Unrecognised platform.
#endif

#include "math.cpp"

#include "array.inl"
#include "ring.inl"

#include "cmd_line_args.cpp"
#include "temp_storage.cpp"
#include "allocation_context.cpp"
#include "allocator_generic.cpp"
#include "allocator_linear.cpp"
#include "hashmap.cpp"
#include "string_helpers.cpp"
#include "string_builder.cpp"
#include "random.cpp"

// things we use external libraries for
#include "process.subprocess.cpp"
#include "hash.xxhash.cpp"
#include "profiler.null.cpp"