/*
===========================================================================

Builder

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
#ifdef _WIN32

#include "../thread.h"

#include "../debug.h"
#include "../defer.h"
#include "../typecast.h"
#include "../temp_storage.h"
#include "../linear_allocator.h"

#include <Windows.h>

#include <malloc.h>

struct threadBootstrapData_t {
	ThreadFunc	threadFunc;
	void		*data;
	u64			tempStorageSize;
};

static DWORD Thread_Bootstrap( void *data ) {
	Assert( data );

	threadBootstrapData_t *bootstrapData = Cast( threadBootstrapData_t *, data );

	Assert( bootstrapData->threadFunc );

	Mem_InitTempStorage( bootstrapData->tempStorageSize );
	defer { Mem_ShutdownTempStorage(); };

	s32 exitCode = bootstrapData->threadFunc( bootstrapData->data );

	free( bootstrapData );
	bootstrapData = NULL;

	DWORD exitCodeDword = Cast( DWORD, exitCode );

	return exitCodeDword;
}

thread_t Thread_Create( ThreadFunc threadFunc, void *data ) {
	Assert( threadFunc );

	// bootstrap data cant be local
	// could go out of scope by the time the thread actually fires
	// TODO: DM: 24/03/2026: do we just pass allocator here so people can specify what allocator this goes on to?
	// if NULL allocator then just malloc?
	threadBootstrapData_t *bootstrap = Cast( threadBootstrapData_t *, malloc( sizeof( threadBootstrapData_t ) ) );
	bootstrap->threadFunc = threadFunc;
	bootstrap->data = data;
	bootstrap->tempStorageSize = Mem_GetTempStorage()->reservedBytes;

	DWORD creationFlags = 0;

	HANDLE handle = CreateThread( NULL, 0, Thread_Bootstrap, bootstrap, creationFlags, NULL );

	return { handle };
}

void Thread_Destroy( thread_t *thread ) {
	Assert( thread );
	Assert( thread->ptr );

	CloseHandle( Cast( HANDLE, thread->ptr ) );
	thread->ptr = NULL;
}

s32 Thread_Wait( thread_t *thread ) {
	Assert( thread );
	Assert( thread->ptr );

	HANDLE handle = Cast( HANDLE, thread->ptr );

	DWORD result = WaitForSingleObject( handle, INFINITE );

	Assert( result != WAIT_FAILED );
	UNUSED( result );

	DWORD exitCode = S32_MAX;
	if ( !GetExitCodeThread( handle, &exitCode ) ) {
		// TODO: DM: 24/03/2026: handle errors etc.
	}

	return TruncCast( s32, exitCode );
}

u32	Thread_AtomicIncrement( atomic32_t *atomic ) {
	return InterlockedIncrement( &atomic->value );
}

#endif // _WIN32
