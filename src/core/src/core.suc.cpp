/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef CORE_SUC

#include "int_truncation.cpp"

#if defined( _WIN64 )
	#include "core.win64.suc.cpp"
#else
	#error Unrecognised platform.
#endif

#include <array.inl>
#include <ring.inl>
#include "allocation_context.cpp"
#include "allocator_generic.cpp"
#include "allocator_linear.cpp"
#include "cmd_line_args.cpp"
#include "hashmap.cpp"
#include "math.cpp"
#include "random.cpp"
#include "string_helpers.cpp"
#include "string_builder.cpp"
#include "temp_storage.cpp"

// things we use external libraries for
#include "process.subprocess.cpp"
#include "hash.xxhash.cpp"
#include "profiler.null.cpp"

#endif // CORE_SUC