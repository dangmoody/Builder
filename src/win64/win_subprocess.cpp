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

#include "../subprocess.h"

#include "../typecast.h"
#include "../string_builder.h"
#include "../array.inl"
#include "../helpers.h"
#include "../linear_allocator.h"
#include "../temp_storage.h"
#include "../defer.h"

#include <Windows.h>

/*
================================================================================================

	Windows process_t implementation

================================================================================================
*/

struct process_t {
	PROCESS_INFORMATION	processInfo;

	HANDLE				stdoutRead;
	HANDLE				eventStdout;

	HANDLE				stderrRead;
	HANDLE				eventStderr;
};

static bool8 Proc_CloseHandleInternal( HANDLE *handle, const char *description ) {
	if ( !*handle ) {
		return true;
	}

	if ( !CloseHandle( *handle ) ) {
		FatalError( "Failed to close %s handle: Windows error code: 0x%X\n", description, GetLastError() );
		return false;
	}

	*handle = NULL;
	return true;
}

process_t* Proc_Create( linearAllocator_t *allocator, array_t<const char *> *args, array_t<const char *> *environmentVariables, const ProcessFlags flags ) {
	Assert( allocator );
	Assert( args );
	Assert( args->count > 0 );

	const char *subprocessName = ( *args )[0];

	process_t *process = Cast( process_t *, Mem_AllocatorAlloc( allocator, sizeof( process_t ) ) );
	memset( process, 0, sizeof( process_t ) );

	SECURITY_ATTRIBUTES secAttr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	HANDLE stdoutWrite = NULL;
	if ( !CreatePipe( &process->stdoutRead, &stdoutWrite, &secAttr, 0 ) ) {
		Error( "CreatePipe call failed for stdout: 0x%X.\n", GetLastError() );
		return NULL;
	}

	HANDLE stderrWrite = NULL;
	if ( !( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) ) {
		if ( !CreatePipe( &process->stderrRead, &stderrWrite, &secAttr, 0 ) ) {
			Error( "CreatePipe call failed for stderr: 0x%X.\n", GetLastError() );
			return NULL;
		}
	}

	STARTUPINFO startInfo = { sizeof( startInfo ) };
	startInfo.dwFlags = STARTF_USESTDHANDLES;
	startInfo.hStdOutput = stdoutWrite;
	startInfo.hStdError = ( flags & PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) ? stdoutWrite : stderrWrite;

	char *combinedArgs = NULL;
	{
		u64 offset = 0;

		u64 combinedArgsLength = 0;

		For ( u32, argIndex, 0, args->count ) {
			const char *arg = ( *args )[argIndex];
			u64 argLen = strlen( arg );

			combinedArgsLength += argLen;

			bool8 alreadyQuoted = ( argLen >= 2 ) && ( arg[0] == '"' ) && ( arg[argLen - 1] == '"' );

			if ( !alreadyQuoted && strchr( arg, ' ' ) ) {
				combinedArgsLength += 2;	// quotes around args that contain spaces
			}
		}
		combinedArgsLength += args->count - 1;	// one space between each argument
		combinedArgsLength += 1;					// null terminator

		combinedArgs = Cast( char *, Mem_TempAlloc( combinedArgsLength * sizeof( char ) ) );

		For ( u64, argIndex, 0, args->count ) {
			const char *arg = ( *args )[argIndex];

			u64 argLen = strlen( arg );

			bool8 alreadyQuoted = ( argLen >= 2 ) && ( arg[0] == '"' ) && ( arg[argLen - 1] == '"' );
			bool8 needsQuotes = !alreadyQuoted && strchr( arg, ' ' ) != NULL;

			if ( needsQuotes ) {
				combinedArgs[offset++] = '"';
			}

			strncpy( combinedArgs + offset, arg, argLen * sizeof( char ) );

			// CreateProcess doesnt like forward slashes
			// make sure they are back slashes
			if ( argIndex == 0 ) {
				for ( u64 charIndex = 0; charIndex < argLen; charIndex++ ) {
					if ( combinedArgs[offset + charIndex] == '/' ) {
						combinedArgs[offset + charIndex] = '\\';
					}
				}
			}

			offset += argLen;

			if ( needsQuotes ) {
				combinedArgs[offset++] = '"';
			}

			combinedArgs[offset] = ' ';
			offset += 1;
		}
		combinedArgs[combinedArgsLength - 1] = 0;
	}

	char *combinedEnvVars = NULL;
	if ( environmentVariables && environmentVariables->count ) {
		u64 combinedEnvVarsLength = 0;
		u64 offset = 0;

		For ( u32, envVarIndex, 0, environmentVariables->count ) {
			combinedEnvVarsLength += strlen( ( *environmentVariables )[envVarIndex] );
			combinedEnvVarsLength += 1;	// space
		}

		combinedEnvVarsLength += 1;	// null terminator at the end

		combinedEnvVars = Cast( char *, Mem_TempAlloc( combinedEnvVarsLength * sizeof( char ) ) );

		For ( u32, envVarIndex, 0, environmentVariables->count ) {
			const char *envVar = ( *environmentVariables )[envVarIndex];

			u64 envVarLength = strlen( envVar );
			strncpy( combinedEnvVars + offset, envVar, envVarLength * sizeof( char ) );

			offset += envVarLength;

			combinedEnvVars[offset] = ' ';

			offset += 1;
		}
		combinedEnvVars[combinedEnvVarsLength - 1] = 0;
	}

	if ( !CreateProcess(
		NULL,
		const_cast<LPSTR>( combinedArgs ),
		NULL,
		NULL,
		true,
		CREATE_NO_WINDOW,
		combinedEnvVars,
		NULL,
		&startInfo,
		&process->processInfo
	) ) {
		Error( "Failed to create process \"%s\": 0x%X.\n", subprocessName, GetLastError() );
		return NULL;
	}

	// close the write ends of the pipes on the parent side
	// the child inherited them so they remain open from the child's perspective
	// closing the parents copies ensures ReadFile on the read ends returns EOF once the child exits rather than blocking indefinitely
	if ( !CloseHandle( stdoutWrite ) ) {
		FatalError( "Failed to close stdout write handle: 0x%X\n", GetLastError() );
		return NULL;
	}

	if ( stderrWrite && !CloseHandle( stderrWrite ) ) {
		FatalError( "Failed to close stderr write handle: 0x%X\n", GetLastError() );
		return NULL;
	}

	return process;
}

bool8 Proc_Destroy( process_t* process ) {
	Assert( process );

	if ( !Proc_CloseHandleInternal( &process->stdoutRead, "subprocess stdout read" ) ) {
		return false;
	}

	if ( !Proc_CloseHandleInternal( &process->stderrRead, "subprocess stderr read" ) ) {
		return false;
	}

	if ( !Proc_CloseHandleInternal( &process->processInfo.hProcess, "subprocess process" ) ) {
		return false;
	}

	if ( !Proc_CloseHandleInternal( &process->processInfo.hThread, "subprocess thread" ) ) {
		return false;
	}

	if ( process->eventStderr && process->eventStderr != process->eventStdout ) {
		if ( !Proc_CloseHandleInternal( &process->eventStderr, "subprocess stderr event" ) ) {
			return false;
		}
	}

	if ( !Proc_CloseHandleInternal( &process->eventStdout, "subprocess stdout event" ) ) {
		return false;
	}

	return true;
}

s32 Proc_Join( process_t* process ) {
	Assert( process );

	if ( !Proc_CloseHandleInternal( &process->stdoutRead, "subprocess stdout read" ) ) {
		return -1;
	}

	if ( !Proc_CloseHandleInternal( &process->stderrRead, "subprocess stderr read" ) ) {
		return -1;
	}

	if ( WaitForSingleObject( process->processInfo.hProcess, INFINITE ) != WAIT_OBJECT_0 ) {
		FatalError( "Failed to wait for subprocess to finish: 0x%X\n", GetLastError() );
		return -1;
	}

	DWORD exitCode = 0;

	if ( !GetExitCodeProcess( process->processInfo.hProcess, &exitCode ) ) {
		FatalError( "Failed to get exit code of subprocess: 0x%X\n", GetLastError() );
		return -1;
	}

	return TruncCast( s32, exitCode );
}

static u32 Proc_ReadFromFileHandle( HANDLE fileHandle, HANDLE event, char *outBuffer, const u64 count ) {
	Assert( fileHandle );
	Assert( outBuffer );
	Assert( count );

	OVERLAPPED overlapped = {};
	overlapped.hEvent = event;

	DWORD bytesRead = 0;
	BOOL read = ReadFile( fileHandle, outBuffer, TruncCast( u32, count ), &bytesRead, &overlapped );

	if ( !read ) {
		DWORD lastError = GetLastError();

		if ( lastError == ERROR_IO_PENDING ) {
			if ( !GetOverlappedResult( fileHandle, &overlapped, &bytesRead, 1 ) ) {
				lastError = GetLastError();

				if ( ( lastError != ERROR_IO_INCOMPLETE ) && ( lastError != ERROR_HANDLE_EOF ) ) {
					Error( "Failed to read stdout of subprocess: 0x%X.\n", GetLastError() );
					return 0;
				}
			}
		}
	}

	return bytesRead;
}

u32 Proc_ReadStdout( process_t *process, char *outBuffer, const u64 count ) {
	Assert( process );
	Assert( outBuffer );
	Assert( count );

	return Proc_ReadFromFileHandle( process->stdoutRead, process->eventStdout, outBuffer, count );
}

#endif // _WIN32
