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

#include <core_process.h>

#include <linear_allocator.h>
#include <core_array.inl>
#include <typecast.inl>
#include <debug.h>
#include <defer.h>
#include <core_helpers.h>

#include <stdio.h>
#include <string.h>
#include <file.h>

#include <unistd.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>

/*
================================================================================================

	Linux Process implementation

================================================================================================
*/

struct Process {
	pid_t	pid;
	FILE	*stdout;
	FILE	*stderr;
};

static bool8 Proc_CreatePipeWithFileActions( posix_spawn_file_actions_t *spawn_actions, int fileno, int out_handles[2], const char *subprocess_name ) {
	if ( pipe( out_handles ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create pipe for fd %d of subprocess %s: %s\n", fileno, subprocess_name, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_adddup2( spawn_actions, out_handles[1], fileno ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to redirect fd %d of subprocess %s: %s\n", fileno, subprocess_name, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_addclose( spawn_actions, out_handles[0] ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to close read end of fd %d pipe in subprocess %s: %s\n", fileno, subprocess_name, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_addclose( spawn_actions, out_handles[1] ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to close write end of fd %d pipe in subprocess %s: %s\n", fileno, subprocess_name, strerror( err ) );
		return false;
	}

	return true;
}

Process	*Proc_Create( LinearAllocator *allocator, Array<const char *> *args, Array<const char *> *environment_variables, const ProcessFlags flags ) {
	Process *subprocess = cast( Process *, Mem_AllocatorAlloc( allocator, sizeof( Process ) ) );

	const char *subprocess_name = ( *args )[0];

	posix_spawn_file_actions_t spawn_actions = {};

	if ( posix_spawn_file_actions_init( &spawn_actions ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create posix_spawn_file_actions_t for subprocess %s: %s\n", subprocess_name, strerror( err ) );
		return NULL;
	}

	defer {
		if ( posix_spawn_file_actions_destroy( &spawn_actions ) != 0 ) {
			int err = errno;
			fatal_error( "Failed to destroy posix_spawn_file_actions_t for subprocess %s: %s\n", subprocess_name, strerror( err ) );
			// return NULL;
		}
	};

	int stdout_file_handles[2] = { -1, -1 };
	if ( !Proc_CreatePipeWithFileActions( &spawn_actions, STDOUT_FILENO, stdout_file_handles, subprocess_name ) ) {
		return NULL;
	}

	int stderr_file_handles[2] = { -1, -1 };

	if ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) {
		if ( posix_spawn_file_actions_adddup2( &spawn_actions, STDOUT_FILENO, STDERR_FILENO ) != 0 ) {
			int err = errno;
			fatal_error( "Failed to combine stdout and stderr outputs for subprocess %s: %s\n", subprocess_name, strerror( err ) );
			return NULL;
		}
	} else {
		if ( !Proc_CreatePipeWithFileActions( &spawn_actions, STDERR_FILENO, stderr_file_handles, subprocess_name ) ) {
			return NULL;
		}
	}

	if ( ( *args )[args->count - 1] != NULL ) {
		args->Add( NULL );
	}

	char * const *args_start = cast( char * const *, &( *args )[0] );

	char * const *env_vars_start = NULL;
	if ( environment_variables && environment_variables->count > 0 ) {
		if ( ( *environment_variables )[environment_variables->count - 1] != NULL ) {
			environment_variables->Add( NULL );
		}

		env_vars_start = cast( char * const *, &( *environment_variables )[0] );
	} else {
		env_vars_start = environ;
	}

	if ( posix_spawnp( &subprocess->pid, subprocess_name, &spawn_actions, NULL, args_start, env_vars_start ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to spawn subprocess %s: %s\n", subprocess_name, strerror( err ) );
		return NULL;
	}

	close( stdout_file_handles[1] );
	subprocess->stdout = fdopen( stdout_file_handles[0], "rb" );

	if ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) {
		subprocess->stderr = subprocess->stdout;
	} else {
		close( stderr_file_handles[1] );
		subprocess->stderr = fdopen( stderr_file_handles[0], "rb" );
	}

	return subprocess;
}

bool8		Proc_Destroy( Process *process ) {
	int result = kill( process->pid, SIGKILL );

	if ( result != 0 ) {
		int err = errno;
		if ( err != ESRCH ) {
			fatal_error( "Failed to kill subprocess: %d: %s\n", err, strerror( err ) );
			return false;
		}
	}

	fclose( process->stdout );

	if ( process->stderr != process->stdout ) {
		fclose( process->stderr );
	}

	process->stdout = NULL;
	process->stderr = NULL;

	return true;
}

s32		Proc_Join( Process *process ) {
	int status = -1;
	if ( waitpid( process->pid, &status, 0 ) != process->pid ) {
		int err = errno;
		fatal_error( "Failed to wait for process to finish: %s\n", strerror( err ) );
		return -1;
	}

	if ( WIFEXITED( status ) ) {
		return WEXITSTATUS( status );
	} else {
		return status;
	}
}

u32		Proc_ReadStdout( Process *process, char *out_buffer, const u64 count ) {
	int file_desc = fileno( process->stdout );
	s64 bytes_read = read( file_desc, out_buffer, count );

	return trunc_cast( u32, bytes_read );
}

#endif // __linux__
