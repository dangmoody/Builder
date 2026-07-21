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

#include "../subprocess.h"

#include "../linear_allocator.h"
#include "../array.inl"
#include "../typecast.h"
#include "../debug.h"
#include "../defer.h"

#include <stdio.h>
#include <string.h>
#include "../file.h"

#include <unistd.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>

/*
================================================================================================

	Linux process_t implementation

================================================================================================
*/

struct process_t {
	pid_t	pid;
	FILE	*stdout;
	FILE	*stderr;
};

static bool8 Proc_CreatePipeWithFileActions( posix_spawn_file_actions_t *spawnActions, int fileno, int outHandles[2], const char *subprocessName ) {
	if ( pipe( outHandles ) != 0 ) {
		int err = errno;
		FatalError( "Failed to create pipe for fd %d of subprocess %s: %s\n", fileno, subprocessName, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_adddup2( spawnActions, outHandles[1], fileno ) != 0 ) {
		int err = errno;
		FatalError( "Failed to redirect fd %d of subprocess %s: %s\n", fileno, subprocessName, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_addclose( spawnActions, outHandles[0] ) != 0 ) {
		int err = errno;
		FatalError( "Failed to close read end of fd %d pipe in subprocess %s: %s\n", fileno, subprocessName, strerror( err ) );
		return false;
	}

	if ( posix_spawn_file_actions_addclose( spawnActions, outHandles[1] ) != 0 ) {
		int err = errno;
		FatalError( "Failed to close write end of fd %d pipe in subprocess %s: %s\n", fileno, subprocessName, strerror( err ) );
		return false;
	}

	return true;
}

process_t	*Proc_Create( linearAllocator_t *allocator, array_t<const char *> *args, array_t<const char *> *environmentVariables, const processFlags_t flags ) {
	process_t *subprocess = Cast( process_t *, Mem_AllocatorAlloc( allocator, sizeof( process_t ) ) );

	const char *subprocessName = ( *args )[0];

	posix_spawn_file_actions_t spawnActions = {};

	if ( posix_spawn_file_actions_init( &spawnActions ) != 0 ) {
		int err = errno;
		FatalError( "Failed to create posix_spawn_file_actions_t for subprocess %s: %s\n", subprocessName, strerror( err ) );
		return NULL;
	}

	defer {
		if ( posix_spawn_file_actions_destroy( &spawnActions ) != 0 ) {
			int err = errno;
			FatalError( "Failed to destroy posix_spawn_file_actions_t for subprocess %s: %s\n", subprocessName, strerror( err ) );
			// return NULL;
		}
	};

	int stdoutFileHandles[2] = { -1, -1 };
	if ( !Proc_CreatePipeWithFileActions( &spawnActions, STDOUT_FILENO, stdoutFileHandles, subprocessName ) ) {
		return NULL;
	}

	int stderrFileHandles[2] = { -1, -1 };

	if ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) {
		if ( posix_spawn_file_actions_adddup2( &spawnActions, STDOUT_FILENO, STDERR_FILENO ) != 0 ) {
			int err = errno;
			FatalError( "Failed to combine stdout and stderr outputs for subprocess %s: %s\n", subprocessName, strerror( err ) );
			return NULL;
		}
	} else {
		if ( !Proc_CreatePipeWithFileActions( &spawnActions, STDERR_FILENO, stderrFileHandles, subprocessName ) ) {
			return NULL;
		}
	}

	if ( ( *args )[args->count - 1] != NULL ) {
		args->Add( NULL );
	}

	char * const *argsStart = Cast( char * const *, &( *args )[0] );

	char * const *envVarsStart = NULL;
	if ( environmentVariables && environmentVariables->count > 0 ) {
		if ( ( *environmentVariables )[environmentVariables->count - 1] != NULL ) {
			environmentVariables->Add( NULL );
		}

		envVarsStart = Cast( char * const *, &( *environmentVariables )[0] );
	} else {
		envVarsStart = environ;
	}

	if ( posix_spawnp( &subprocess->pid, subprocessName, &spawnActions, NULL, argsStart, envVarsStart ) != 0 ) {
		int err = errno;
		FatalError( "Failed to spawn subprocess %s: %s\n", subprocessName, strerror( err ) );
		return NULL;
	}

	close( stdoutFileHandles[1] );
	subprocess->stdout = fdopen( stdoutFileHandles[0], "rb" );

	if ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) {
		subprocess->stderr = subprocess->stdout;
	} else {
		close( stderrFileHandles[1] );
		subprocess->stderr = fdopen( stderrFileHandles[0], "rb" );
	}

	return subprocess;
}

bool8		Proc_Destroy( process_t *process ) {
	int result = kill( process->pid, SIGKILL );

	if ( result != 0 ) {
		int err = errno;
		if ( err != ESRCH ) {
			FatalError( "Failed to kill subprocess: %d: %s\n", err, strerror( err ) );
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

s32		Proc_Join( process_t *process ) {
	int status = -1;
	if ( waitpid( process->pid, &status, 0 ) != process->pid ) {
		int err = errno;
		FatalError( "Failed to wait for process to finish: %s\n", strerror( err ) );
		return -1;
	}

	if ( WIFEXITED( status ) ) {
		return WEXITSTATUS( status );
	} else {
		return status;
	}
}

u32		Proc_ReadStdout( process_t *process, char *outBuffer, const u64 count ) {
	int fileDesc = fileno( process->stdout );
	s64 bytesRead = read( fileDesc, outBuffer, count );

	return TruncCast( u32, bytesRead );
}

#endif // __linux__
