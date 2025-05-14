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

#if defined( _WIN32 ) && !defined( CORE_USE_SUBPROCESS )

#include <core_process.h>

#include <allocation_context.h>
#include <typecast.inl>
#include <string_builder.h>
#include <array.inl>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
================================================================================================

	Process

================================================================================================
*/

struct Process {
	PROCESS_INFORMATION	process_info;
	HANDLE				stdout_read;
	HANDLE				stdout_write;
};

Process* process_create( Array<const char*>* args, Array<const char*>* environment_variables ) {
	assert( args );
	assert( args->count > 0 );

	unused( environment_variables );

	//TODO(TOM): Figure out how to configure the file IO allocator
	Allocator* platform_allocator = g_core_ptr->allocator_stack[0];
	mem_push_allocator( platform_allocator );
	defer( mem_pop_allocator() );

	Process* process = cast( Process*, mem_alloc( sizeof( Process ) ) );

	SECURITY_ATTRIBUTES sec_attr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	// stdout
	if ( !CreatePipe( &process->stdout_read, &process->stdout_write, &sec_attr, 0 ) ) {
		fatal_error( "CreatePipe call failed: 0x%X.\n", GetLastError() );
	}

	STARTUPINFO start_info = { sizeof( start_info ) };
	start_info.dwFlags = STARTF_USESTDHANDLES;
	start_info.hStdOutput = process->stdout_write;
	start_info.hStdError = NULL;//process->write_handle;

	char* combined_args = NULL;
	{
		u64 offset = 0;

		u64 combined_args_length = 0;

		For ( u64, arg_index, 0, args->count ) {
			combined_args_length += strlen( ( *args )[arg_index] );
		}
		combined_args_length += args->count - 1;	// one space between each argument
		combined_args_length += 1;					// null terminator

		combined_args = cast( char*, mem_alloc( combined_args_length * sizeof( char ) ) );

		For ( u64, arg_index, 0, args->count ) {
			const char* arg = ( *args )[arg_index];

			u64 arg_len = strlen( arg );

			strncpy( combined_args + offset, arg, arg_len * sizeof( char ) );
			offset += arg_len;

			combined_args[offset] = ' ';
			offset += 1;
		}
		combined_args[combined_args_length - 1] = 0;
	}
	defer( mem_free( combined_args ) );

	BOOL created = CreateProcess(
		NULL,
		const_cast<LPSTR>( combined_args ),
		NULL,
		NULL,
		true,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&start_info,
		&process->process_info
	);

	if ( !created ) {
		fatal_error( "CreateProcess() failed: 0x%X.\n", GetLastError() );
	}

	CloseHandle( start_info.hStdOutput );

	return process;
}

void process_destroy( Process* process ) {
	CloseHandle( process->stdout_read );
	CloseHandle( process->process_info.hProcess );
	CloseHandle( process->process_info.hThread );
}

s32 process_join( Process* process ) {
	CloseHandle( process->stdout_read );
	WaitForSingleObject( process->process_info.hProcess, INFINITE );

	DWORD exit_code = 0;
	BOOL got_exit_code = GetExitCodeProcess( process->process_info.hProcess, &exit_code );

	assert( got_exit_code );

	return trunc_cast( s32, exit_code );
}

u32 process_read_stdout( Process* process, char* out_buffer, const u32 count ) {
	//OVERLAPPED overlapped = {};
	//overlapped.hEvent = process->output_event;

	DWORD bytes_read = 0;
	BOOL read = ReadFile( process->stdout_read, out_buffer, count, &bytes_read, /*&overlapped*/NULL );

	return read ? bytes_read : 0;
}

#endif // defined( _WIN32 ) && !defined( CORE_USE_SUBPROCESS )