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

#include "../thread.h"

#include "../debug.h"
#include "../defer.h"
#include "../typecast.h"
#include "../temp_storage.h"
#include "../linear_allocator.h"

#include <pthread.h>
#include <errno.h>

#include <string.h>
#include <malloc.h>

struct threadBootstrapData_t {
	ThreadFunc		threadFunc;
	void			*data;
	u64				tempStorageSize;
};

static void *Thread_Bootstrap( void *data ) {
	Assert( data );

	threadBootstrapData_t *bootstrapData = Cast( threadBootstrapData_t *, data );

	Assert( bootstrapData );

	Mem_InitTempStorage( bootstrapData->tempStorageSize );
	defer { Mem_ShutdownTempStorage(); };

	s32 exitCode = bootstrapData->threadFunc( bootstrapData->data );

	free( bootstrapData );
	bootstrapData = NULL;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
	s32 *exitCodePtr = Cast( s32 *, exitCode );
#pragma clang diagnostic pop

	return Cast( void *, exitCodePtr );
}

thread_t		Thread_Create( ThreadFunc threadFunc, void *data ) {
	Assert( threadFunc );

	pthread_t threadLinux;

	pthread_attr_t attribs = {};
	pthread_attr_init( &attribs );
	defer { pthread_attr_destroy( &attribs ); };

	threadBootstrapData_t *bootstrapData = Cast( threadBootstrapData_t *, malloc( sizeof( threadBootstrapData_t ) ) );
	bootstrapData->threadFunc = threadFunc;
	bootstrapData->data = data;
	bootstrapData->tempStorageSize = Mem_GetTempStorage()->reservedBytes;

	if ( pthread_create( &threadLinux, &attribs, &Thread_Bootstrap, bootstrapData ) != 0 ) {
		int err = errno;
		FatalError( "Failed to create thread: %s\n", strerror( err ) );

		return { NULL };
	}

	return { Cast( void *, threadLinux ) };
}

void		Thread_Destroy( thread_t *thread ) {
	pthread_t pthreadLinux = Cast( pthread_t, thread->ptr );

	if ( pthreadLinux ) {
		if ( pthread_detach( pthreadLinux ) != 0 ) {
			int err = errno;
			FatalError( "Failed to detach thread: %s\n", strerror( err ) );
		}

		thread->ptr = NULL;
	}
}

s32		Thread_Wait( thread_t *thread ) {
	pthread_t pthreadLinux = Cast( pthread_t, thread->ptr );

	s32 *exitCodePtr;

	if ( pthread_join( pthreadLinux, Cast( void **, &exitCodePtr ) ) != 0 ) {
		int err = errno;
		FatalError( "Failed to join thread: %s\n", strerror( err ) );

		return -1;
	}

	thread->ptr = NULL;

	s64 exitCode2 = Cast( s64, exitCodePtr );

	return TruncCast( s32, exitCode2 );
}

#pragma clang diagnostic push
// TOOD: DM: 04/05/2026: this warning means we are imposing "stronger memory barriers than necessary", but I think we definitely need them?
// the whole point of doing an "atomic" operation is to guarantee syncronization of values between threads in order to safely avoid race conditions, which is exactly what these intrinsics offer
// so we need them?
#pragma clang diagnostic ignored "-Watomic-implicit-seq-cst"

u32		Thread_AtomicIncrement( atomic32_t *atomic ) {
	Assert( atomic );
	return __sync_add_and_fetch( &atomic->value, 1 );
}

#pragma clang diagnostic pop

#endif
