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
#ifdef _WIN32

#include <core_thread.h>

#include <debug.h>
#include <typecast.inl>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

struct ThreadBootstrapData {
	ThreadFunc	thread_func;
	void*		data;
};

static DWORD thread_bootstrap( void* data ) {
	assert( data );

	ThreadBootstrapData* bootstrap_data = cast( ThreadBootstrapData*, data );

	assert( bootstrap_data->thread_func );
	assert( bootstrap_data->data );

	s32 thread_return_code = bootstrap_data->thread_func( bootstrap_data->data );

	return cast( DWORD, thread_return_code );
}

Thread thread_create( ThreadFunc thread_func, void* data ) {
	assert( thread_func );
	assert( data );

	HANDLE handle = CreateThread( NULL, 0, thread_bootstrap, data, 0, 0 );

	if ( handle == INVALID_HANDLE_VALUE ) {
		return { NULL };
	}

	return { handle };
}

void thread_destroy( Thread* thread ) {
	assert( thread );
	assert( thread->ptr );

	// TODO(DM): 03/02/2026: is this expected behaviour user-side?
	// do we make users do this themselves?
	thread_wait_for_idle( thread );

	CloseHandle( cast( HANDLE, thread->ptr ) );
	thread->ptr = NULL;
}

void thread_wait_for_idle( Thread* thread ) {
	HANDLE handle = cast( HANDLE, thread->ptr );

	DWORD exit_code = WaitForSingleObjectEx( handle, INFINITE, TRUE );

	assert( exit_code != WAIT_FAILED );
}

void thread_sleep( const float64 seconds ) {
	DWORD ms = cast( DWORD, seconds ) * 1000;

	Sleep( ms );
}

#endif // _WIN32