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
#include "dll_export.h"

/*
================================================================================================

	Thread

	OS-agnostic functions for creating, scheduling, and destroying threads and semaphores.

	Also contains basic data structure and functions for atomics and atomic operations.

================================================================================================
*/

struct Thread {
	void	*ptr;
};

struct Semaphore {
	void	*ptr;
};

struct Atomic32 {
	volatile u32	value;
};

typedef s32 ( *ThreadFunc )( void *data );

// Creates and immediately executes a thread that runs 'thread_func' with 'data' passed through.
CORE_API Thread		thread_create( ThreadFunc thread_func, void *data );

// Waits for the thread to stop running, then destroys it.
CORE_API void		thread_destroy( Thread *thread );

// Waits for the thread to stop running, returning the exit code when it finished.
CORE_API s32		thread_wait( Thread *thread );

// Returns true if the thread was successfully suspended, otherwise returns false.
CORE_API bool8		thread_suspend( Thread *thread );

// Returns true if the thread was successfully resumed, otherwise returns false.
CORE_API bool8		thread_resume( Thread *thread );

// Returns true if the semaphore could be successfully created, otherwise returns false.
CORE_API bool8		semaphore_create( Semaphore *semaphore );

// Returns true if the semaphore could be sucessfully destroyed, otherwise returns false.
CORE_API bool8		semaphore_destroy( Semaphore *semaphore );

// Any threads blocked by this semaphore will become unblocked.
CORE_API void		semaphore_signal( Semaphore *semaphore );

// Blocks the thread until the semaphore "wakes up" via a call to 'semaphore_signal'.
CORE_API s32		semaphore_wait( Semaphore *semaphore );

// Performs an atomic increment.
CORE_API u32		atomic_increment( Atomic32 *atomic );

// Performs an atomic decrement.
CORE_API u32		atomic_decrement( Atomic32* atomic );

// If the value of 'dst' is the same as 'compare', then 'dst' gets set to 'exchange'.
// The value of 'dst' before it got potentially changed gets returned.
CORE_API u32		atomic_compare_exchange( Atomic32* dst, const u32 compare, const u32 exchange );
