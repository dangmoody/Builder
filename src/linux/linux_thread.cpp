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

#ifdef __linux__

#include <core_thread.h>

#include <core_helpers.h>
#include <debug.h>
#include <defer.h>
#include <typecast.inl>
#include <temp_storage.h>
#include <linear_allocator.h>

#include <pthread.h>
#include <errno.h>

#include <string.h>
#include <malloc.h>

struct ThreadBootstrapData {
	ThreadFunc		thread_func;
	void			*data;
	u64				temp_storage_size;
};

static void *Thread_Bootstrap( void *data ) {
	assert( data );

	ThreadBootstrapData *bootstrap_data = cast( ThreadBootstrapData *, data );

	assert( bootstrap_data );

	Mem_InitTempStorage( bootstrap_data->temp_storage_size );
	defer { Mem_ShutdownTempStorage(); };

	s32 exit_code = bootstrap_data->thread_func( bootstrap_data->data );

	free( bootstrap_data );
	bootstrap_data = NULL;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
	s32 *exit_code_ptr = cast( s32 *, exit_code );
#pragma clang diagnostic pop

	return cast( void *, exit_code_ptr );
}

Thread		Thread_Create( ThreadFunc thread_func, void *data ) {
	assert( thread_func );

	pthread_t thread_linux;

	pthread_attr_t attribs = {};
	pthread_attr_init( &attribs );
	defer { pthread_attr_destroy( &attribs ); };

	ThreadBootstrapData *bootstrap_data = cast( ThreadBootstrapData *, malloc( sizeof( ThreadBootstrapData ) ) );
	bootstrap_data->thread_func = thread_func;
	bootstrap_data->data = data;
	bootstrap_data->temp_storage_size = Mem_GetTempStorage()->reservedBytes;

	if ( pthread_create( &thread_linux, &attribs, &Thread_Bootstrap, bootstrap_data ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create thread: %s\n", strerror( err ) );

		return { NULL };
	}

	return { cast( void *, thread_linux ) };
}

void		Thread_Destroy( Thread *thread ) {
	pthread_t pthread_linux = cast( pthread_t, thread->ptr );

	if ( pthread_linux ) {
		if ( pthread_detach( pthread_linux ) != 0 ) {
			int err = errno;
			fatal_error( "Failed to detach thread: %s\n", strerror( err ) );
		}

		thread->ptr = NULL;
	}
}

s32		Thread_Wait( Thread *thread ) {
	pthread_t pthread_linux = cast( pthread_t, thread->ptr );

	s32 *exit_code_ptr;

	if ( pthread_join( pthread_linux, cast( void **, &exit_code_ptr ) ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to join thread: %s\n", strerror( err ) );

		return -1;
	}

	thread->ptr = NULL;

	s64 exit_code_2 = cast( s64, exit_code_ptr );

	return trunc_cast( s32, exit_code_2 );
}

#pragma clang diagnostic push
// TOOD: DM: 04/05/2026: this warning means we are imposing "stronger memory barriers than necessary", but I think we definitely need them?
// the whole point of doing an "atomic" operation is to guarantee syncronization of values between threads in order to safely avoid race conditions, which is exactly what these intrinsics offer
// so we need them?
#pragma clang diagnostic ignored "-Watomic-implicit-seq-cst"

u32		Thread_AtomicIncrement( Atomic32 *atomic ) {
	assert( atomic );
	return __sync_add_and_fetch( &atomic->value, 1 );
}

#pragma clang diagnostic pop

#endif
