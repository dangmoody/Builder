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

#include <process.h>
#include <debug.h>
#include <allocation_context.h>
#include <defer.h>
#include <array.inl>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "3rdparty/subprocess/subprocess.h"
#pragma clang diagnostic pop

/*
================================================================================================

	Process

================================================================================================
*/

struct Process {
	struct subprocess_s	proc;
};

Process* process_create( Array<const char*>* args, Array<const char*>* environment_variables ) {
	assert( args );
	assertf( args->count >= 1, "When calling \"%s\", the args array MUST have at least one element in it (which is the name of the process you want to call).\n" );
	assert( args->data != NULL );

	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	// dont memset here because kicking off a subprocess is a slow thing to do
	// and we need all the speed wins we can get here, no matter how small
	Process* process = cast( Process* ) mem_alloc( sizeof( Process ) );
	process->proc = {};

	// separate stdout and stderr doesnt work for some reason?
	int options = subprocess_option_combined_stdout_stderr | subprocess_option_enable_async;

	if ( environment_variables == NULL ) {
		options |= subprocess_option_inherit_environment;
	}

	// subprocess.h requires that if we specified an array of environment variables then the array MUST end with 2 x NULLs
	if ( environment_variables && environment_variables->count != 0 && ( *environment_variables )[environment_variables->count - 1] != NULL ) {
		environment_variables->add( NULL );
	}

	// subprocess.h requires that the array of args MUST end with a NULL
	if ( ( *args )[args->count - 1] != NULL ) {
		args->add( NULL );
	}

	int result = subprocess_create_ex( args->data, options, environment_variables ? environment_variables->data : NULL, &process->proc );

	assertf( result == 0, "Failed to create process: 0x%X\n", GetLastError() );
	unused( result );

	return process;
}

void process_destroy( Process* process ) {
	assert( process );

	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];

	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	int result = subprocess_destroy( &process->proc );

	if ( result != 0 ) {
		error( "Failed to destroy process.\n" );
		return;
	}

	mem_free( process );
	process = NULL;
}

s32 process_join( Process** process ) {
	assert( process );
	assert( *process );

	int exit_code = -1;
	int result = subprocess_join( &( *process )->proc, &exit_code );

	assert( result == 0 );
	unused( result );

	return exit_code;
}

u64 process_read_stdout( Process* process, char* out_buffer, const u32 count ) {
	assert( process );
	assert( out_buffer );

	return subprocess_read_stdout( &process->proc, out_buffer, count );
}