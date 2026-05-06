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

#include <core_process.h>

#include <typecast.inl>
#include <string_builder.h>
#include <core_array.inl>
#include <core_helpers.h>
#include <linear_allocator.h>
#include <temp_storage.h>
#include <defer.h>

#include <Windows.h>

/*
================================================================================================

	Windows Process implementation

================================================================================================
*/

struct Process {
	PROCESS_INFORMATION	process_info;

	HANDLE				stdout_read;
	HANDLE				event_stdout;

	HANDLE				stderr_read;
	HANDLE				event_stderr;
};

static bool8 close_handle_internal( HANDLE *handle, const char *description ) {
	if ( !*handle ) {
		return true;
	}

	if ( !CloseHandle( *handle ) ) {
		fatal_error( "Failed to close %s handle: Windows error code: 0x%X\n", description, GetLastError() );
		return false;
	}

	*handle = NULL;
	return true;
}

Process* process_create( LinearAllocator *allocator, Array<const char *> *args, Array<const char *> *environment_variables, const ProcessFlags flags ) {
	assert( allocator );
	assert( args );
	assert( args->count > 0 );

	const char *subprocess_name = ( *args )[0];

	Process *process = cast( Process *, linear_allocator_alloc( allocator, sizeof( Process ) ) );

	SECURITY_ATTRIBUTES sec_attr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	HANDLE stdout_write = NULL;
	if ( !CreatePipe( &process->stdout_read, &stdout_write, &sec_attr, 0 ) ) {
		error( "CreatePipe call failed for stdout: 0x%X.\n", GetLastError() );
		return NULL;
	}

	HANDLE stderr_write = NULL;
	if ( !( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) ) {
		if ( !CreatePipe( &process->stderr_read, &stderr_write, &sec_attr, 0 ) ) {
			error( "CreatePipe call failed for stderr: 0x%X.\n", GetLastError() );
			return NULL;
		}
	}

	STARTUPINFO start_info = { sizeof( start_info ) };
	start_info.dwFlags = STARTF_USESTDHANDLES;
	start_info.hStdOutput = stdout_write;
	start_info.hStdError = ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) ? stdout_write : stderr_write;

	char *combined_args = NULL;
	{
		u64 offset = 0;

		u64 combined_args_length = 0;

		For ( u32, arg_index, 0, args->count ) {
			combined_args_length += strlen( ( *args )[arg_index] );
		}
		combined_args_length += args->count - 1;	// one space between each argument
		combined_args_length += 1;					// null terminator

		combined_args = cast( char *, mem_temp_alloc( combined_args_length * sizeof( char ) ) );

		For ( u64, arg_index, 0, args->count ) {
			const char *arg = ( *args )[arg_index];

			u64 arg_len = strlen( arg );

			strncpy( combined_args + offset, arg, arg_len * sizeof( char ) );
			offset += arg_len;

			combined_args[offset] = ' ';
			offset += 1;
		}
		combined_args[combined_args_length - 1] = 0;
	}

	char *combined_env_vars = NULL;
	if ( environment_variables && environment_variables->count ) {
		u64 combined_env_vars_length = 0;
		u64 offset = 0;

		For ( u32, env_var_index, 0, environment_variables->count ) {
			combined_env_vars_length += strlen( ( *environment_variables )[env_var_index] );
			combined_env_vars_length += 1;	// space
		}

		combined_env_vars_length += 1;	// null terminator at the end

		combined_env_vars = cast( char *, mem_temp_alloc( combined_env_vars_length * sizeof( char ) ) );

		For ( u32, env_var_index, 0, environment_variables->count ) {
			const char *env_var = ( *environment_variables )[env_var_index];

			u64 env_var_length = strlen( env_var );
			strncpy( combined_env_vars + offset, env_var, env_var_length * sizeof( char ) );

			offset += env_var_length;

			combined_env_vars[offset] = ' ';

			offset += 1;
		}
		combined_env_vars[combined_env_vars_length - 1] = 0;
	}

	if ( flags & PROCESS_FLAG_ASYNC ) {
		process->event_stdout = CreateEvent( &sec_attr, 1, 1, NULL );
		process->event_stderr = ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR )
			? process->event_stdout
			: CreateEvent( &sec_attr, 1, 1, NULL );
	}

	if ( !CreateProcess(
		NULL,
		const_cast<LPSTR>( combined_args ),
		NULL,
		NULL,
		true,
		CREATE_NO_WINDOW,
		combined_env_vars,
		NULL,
		&start_info,
		&process->process_info
	) ) {
		error( "Failed to create process \"%s\": 0x%X.\n", subprocess_name, GetLastError() );
		return NULL;
	}

	// close the write ends of the pipes on the parent side
	// the child inherited them so they remain open from the child's perspective
	// closing the parents copies ensures ReadFile on the read ends returns EOF once the child exits rather than blocking indefinitely
	if ( !CloseHandle( stdout_write ) ) {
		fatal_error( "Failed to close stdout write handle: 0x%X\n", GetLastError() );
		return NULL;
	}

	if ( stderr_write && !CloseHandle( stderr_write ) ) {
		fatal_error( "Failed to close stderr write handle: 0x%X\n", GetLastError() );
		return NULL;
	}

	return process;
}

bool8 process_destroy( Process* process ) {
	assert( process );

	if ( !close_handle_internal( &process->stdout_read, "subprocess stdout read" ) ) {
		return false;
	}

	if ( !close_handle_internal( &process->stderr_read, "subprocess stderr read" ) ) {
		return false;
	}

	if ( !close_handle_internal( &process->process_info.hProcess, "subprocess process" ) ) {
		return false;
	}

	if ( !close_handle_internal( &process->process_info.hThread, "subprocess thread" ) ) {
		return false;
	}

	if ( process->event_stderr && process->event_stderr != process->event_stdout ) {
		if ( !close_handle_internal( &process->event_stderr, "subprocess stderr event" ) ) {
			return false;
		}
	}

	if ( !close_handle_internal( &process->event_stdout, "subprocess stdout event" ) ) {
		return false;
	}

	return true;
}

s32 process_join( Process* process ) {
	assert( process );

	if ( !close_handle_internal( &process->stdout_read, "subprocess stdout read" ) ) {
		return -1;
	}

	if ( !close_handle_internal( &process->stderr_read, "subprocess stderr read" ) ) {
		return -1;
	}

	if ( WaitForSingleObject( process->process_info.hProcess, INFINITE ) != WAIT_OBJECT_0 ) {
		fatal_error( "Failed to wait for subprocess to finish: 0x%X\n", GetLastError() );
		return -1;
	}

	DWORD exit_code = 0;

	if ( !GetExitCodeProcess( process->process_info.hProcess, &exit_code ) ) {
		fatal_error( "Failed to get exit code of subprocess: 0x%X\n", GetLastError() );
		return -1;
	}

	return trunc_cast( s32, exit_code );
}

static u32 read_from_file_handle( HANDLE file_handle, HANDLE event, char *out_buffer, const u64 count ) {
	assert( file_handle );
	assert( event );
	assert( out_buffer );
	assert( count );

	OVERLAPPED overlapped = {};
	overlapped.hEvent = event;

	DWORD bytes_read = 0;
	BOOL read = ReadFile( file_handle, out_buffer, trunc_cast( u32, count ), &bytes_read, &overlapped );

	if ( !read ) {
		DWORD last_error = GetLastError();

		if ( last_error == ERROR_IO_PENDING ) {
			if ( !GetOverlappedResult( file_handle, &overlapped, &bytes_read, 1 ) ) {
				last_error = GetLastError();

				if ( ( last_error != ERROR_IO_INCOMPLETE ) && ( last_error != ERROR_HANDLE_EOF ) ) {
					error( "Failed to read stdout of subprocess: 0x%X.\n", GetLastError() );
					return 0;
				}
			}
		}
	}

	return bytes_read;
}

u32 process_read_stdout( Process *process, char *out_buffer, const u64 count ) {
	assert( process );
	assert( out_buffer );
	assert( count );

	return read_from_file_handle( process->stdout_read, process->event_stdout, out_buffer, count );
}

u32 process_read_stderr( Process *process, char *out_buffer, const u64 count ) {
	assert( process );
	assert( out_buffer );
	assert( count );

	return read_from_file_handle( process->stderr_read, process->event_stderr, out_buffer, count );
}

#endif // _WIN32
