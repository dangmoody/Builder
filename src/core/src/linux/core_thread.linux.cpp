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

#ifdef __linux__

#include <core_thread.h>

#include <core_helpers.h>
#include <debug.h>
#include <defer.h>
#include <typecast.inl>
#include <temp_storage.h>

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

struct ThreadBootstrapData {
	ThreadFunc		thread_func;
	void			*data;
};

static void *thread_bootstrap( void *data ) {
	assert( data );

	ThreadBootstrapData *bootstrap_data = cast( ThreadBootstrapData *, data );

	assert( bootstrap_data );

	mem_init_temp_storage( MEM_KILOBYTES( 64 ) );	// DM!!! this needs to be parameterized
	defer { mem_shutdown_temp_storage(); };

	s32 exit_code = bootstrap_data->thread_func( bootstrap_data->data );

	free( bootstrap_data );
	bootstrap_data = NULL;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
	s32 *exit_code_ptr = cast( s32 *, exit_code );
#pragma clang diagnostic pop

	return cast( void *, exit_code_ptr );
}

Thread		thread_create( ThreadFunc thread_func, void *data ) {
	assert( thread_func );

	pthread_t thread_linux;

	pthread_attr_t attribs = {};
	pthread_attr_init( &attribs );
	defer { pthread_attr_destroy( &attribs ); };

	ThreadBootstrapData *bootstrap_data = cast( ThreadBootstrapData *, malloc( sizeof( ThreadBootstrapData ) ) );
	bootstrap_data->thread_func = thread_func;
	bootstrap_data->data = data;

	if ( pthread_create( &thread_linux, &attribs, &thread_bootstrap, bootstrap_data ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create thread: %s\n", strerror( err ) );

		return { NULL };
	}

	return { cast( void *, thread_linux ) };
}

void		thread_destroy( Thread *thread ) {
	pthread_t pthread_linux = cast( pthread_t, thread->ptr );

	if ( pthread_linux ) {
		if ( pthread_detach( pthread_linux ) != 0 ) {
			int err = errno;
			fatal_error( "Failed to detach thread: %s\n", strerror( err ) );
		}

		thread->ptr = NULL;
	}
}

s32		thread_wait( Thread *thread ) {
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

bool8		thread_suspend( Thread *thread ) {
	unused( thread );
	assert( false && "TODO: DM: this" );
	return false;
}

bool8		thread_resume( Thread *thread ) {
	unused( thread );
	assert( false && "TODO: DM: this" );
	return false;
}

bool8		semaphore_create( Semaphore *semaphore ) {
	assert( semaphore );

	sem_t *sema_linux = cast( sem_t *, malloc( sizeof( sem_t ) ) );

	if ( sem_init( sema_linux, 0, 0 ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create semaphore: %s\n", strerror( err ) );
		return false;
	}

	semaphore->ptr = sema_linux;

	return true;
}

bool8		semaphore_destroy( Semaphore *semaphore ) {
	assert( semaphore );

	sem_t *sema_linux = cast( sem_t *, semaphore->ptr );

	if ( sem_destroy( sema_linux ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to destroy semaphore: %s\n", strerror( err ) );
		return false;
	}

	free( sema_linux );
	sema_linux = NULL;

	semaphore->ptr = NULL;

	return true;
}

void		semaphore_signal( Semaphore *semaphore ) {
	assert( semaphore );

	sem_t *sema_linux = cast( sem_t *, semaphore->ptr );

	if ( sem_post( sema_linux ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to signal semaphore: %s\n", strerror( err ) );
	}
}

s32		semaphore_wait( Semaphore *semaphore ) {
	assert( semaphore );
	assert( semaphore->ptr );

	sem_t *sema_linux = cast( sem_t *, semaphore->ptr );

	if ( sem_wait( sema_linux ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to wait for semaphore: %s\n", strerror( err ) );
		return -1;
	}

	// TODO: DM: 04/05/2026: the only reason we get away with this currently is because we never use the return value anywhere
	return 0;
}

#pragma clang diagnostic push
// TOOD: DM: 04/05/2026: this warning means we are imposing "stronger memory barriers than necessary", but I think we definitely need them?
// the whole point of doing an "atomic" operation is to guarantee syncronization of values between threads in order to safely avoid race conditions, which is exactly what these intrinsics offer
// so we need them?
#pragma clang diagnostic ignored "-Watomic-implicit-seq-cst"

u32		atomic_increment( Atomic32 *atomic ) {
	assert( atomic );
	return __sync_add_and_fetch( &atomic->value, 1 );
}

u32		atomic_decrement( Atomic32* atomic ) {
	assert( atomic );
	return __sync_sub_and_fetch( &atomic->value, 1 );
}

u32		atomic_compare_exchange( Atomic32* dst, const u32 compare, const u32 exchange ) {
	assert( dst );
	return __sync_val_compare_and_swap( &dst->value, compare, exchange );
}

#pragma clang diagnostic pop

#endif
