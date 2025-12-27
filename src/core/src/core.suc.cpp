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

#if defined( _WIN32 )
	#include "win64/core.win64.suc.cpp"
#elif defined( __linux__ )
	#include "win64/core.linux.suc.cpp"
#else
	#error Unrecognised platform!
#endif

#include "cmd_line_args.cpp"
#include "core_math.cpp"
#include "core_string.cpp"
#include "debug.cpp"
#include "file.cpp"
#include "hash.xxhash.cpp"
#include "hashmap.cpp"
#include "intrinsics.clang.cpp"
#include "linear_allocator.cpp"
#include "paths.cpp"
#include "random.cpp"
#include "string_builder.cpp"
#include "temp_storage.cpp"
