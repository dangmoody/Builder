/*
===========================================================================

Core

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#ifdef CORE_SUC

#if defined( _WIN64 )
	#include "core.win64.suc.cpp"
#elif __linux__
	#include "core.linux.suc.cpp"
#else
	#error Unrecognised platform.
#endif

#include <typecast.inl>
#include <array.inl>
#include <ring.inl>
#include "allocation_context.cpp"
#include "memory_tracking.cpp"
#include "allocator_linear.cpp"
#include "cmd_line_args.cpp"
#include "core_string.cpp"
#include "debug.cpp"
#include "hashmap.cpp"
#include "math.cpp"
#include "random.cpp"
#include "paths.cpp"
#include "string_helpers.cpp"
#include "string_builder.cpp"
#include "temp_storage.cpp"

// things we use external libraries for
#include "core_process.subprocess.cpp"
#include "hash.xxhash.cpp"

#endif // CORE_SUC