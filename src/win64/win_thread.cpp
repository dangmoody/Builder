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
#ifdef _WIN32

#include "../thread.h"

#include "../helpers.h"
#include "../debug.h"
#include "../defer.h"
#include "../typecast.h"
#include "../temp_storage.h"
#include "../linear_allocator.h"

#include <Windows.h>

#include <malloc.h>

struct ThreadBootstrapData {
	ThreadFunc	thread_func;
	void		*data;
	u64			temp_storage_size;
};

static DWORD thread_bootstrap( void *data ) {
	assert( data );

	ThreadBootstrapData *bootstrap_data = cast( ThreadBootstrapData *, data );

	assert( bootstrap_data->thread_func );

	mem_init_temp_storage( bootstrap_data->temp_storage_size );
	defer { mem_shutdown_temp_storage(); };

	s32 exit_code = bootstrap_data->thread_func( bootstrap_data->data );

	free( bootstrap_data );
	bootstrap_data = NULL;

	DWORD exit_code_dword = cast( DWORD, exit_code );

	return exit_code_dword;
}

Thread thread_create( ThreadFunc thread_func, void *data ) {
	assert( thread_func );

	// bootstrap data cant be local
	// could go out of scope by the time the thread actually fires
	// TODO: DM: 24/03/2026: do we just pass allocator here so people can specify what allocator this goes on to?
	// if NULL allocator then just malloc?
	ThreadBootstrapData *bootstrap = cast( ThreadBootstrapData *, malloc( sizeof( ThreadBootstrapData ) ) );
	bootstrap->thread_func = thread_func;
	bootstrap->data = data;
	bootstrap->temp_storage_size = mem_get_temp_storage()->reserved_bytes;

	DWORD creation_flags = 0;

	HANDLE handle = CreateThread( NULL, 0, thread_bootstrap, bootstrap, creation_flags, NULL );

	return { handle };
}

void thread_destroy( Thread *thread ) {
	assert( thread );
	assert( thread->ptr );

	CloseHandle( cast( HANDLE, thread->ptr ) );
	thread->ptr = NULL;
}

s32 thread_wait( Thread *thread ) {
	assert( thread );
	assert( thread->ptr );

	HANDLE handle = cast( HANDLE, thread->ptr );

	DWORD result = WaitForSingleObject( handle, INFINITE );

	assert( result != WAIT_FAILED );
	unused( result );

	DWORD exit_code = S32_MAX;
	if ( !GetExitCodeThread( handle, &exit_code ) ) {
		// TODO: DM: 24/03/2026: handle errors etc.
	}

	return trunc_cast( s32, exit_code );
}

u32	atomic_increment( Atomic32 *atomic ) {
	return InterlockedIncrement( &atomic->value );
}

#endif // _WIN32
