/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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

#pragma once

#include "int_types.h"

struct linearAllocator_t;
template<typename T> struct array_t;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

/*
================================================================================================

	process_t

	OS-agnostic interface for running other processes from your program.

================================================================================================
*/

struct process_t;

enum processFlagBits_t {
	PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR = 1,
};
typedef u32 processFlags_t;


process_t	*Proc_Create( linearAllocator_t *allocator, array_t<const char *> *args, array_t<const char *> *environmentVariables = NULL, const processFlags_t flags = 0 );

bool8		Proc_Destroy( process_t *process );

s32			Proc_Join( process_t *process );

u32			Proc_ReadStdout( process_t *process, char *outBuffer, const u64 count );

#ifdef __clang__
#pragma clang diagnostic pop
#endif
