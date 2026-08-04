#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdint.h>

typedef enum BinaryType {
	BINARY_TYPE_EXE	= 0,
	BINARY_TYPE_DYNAMIC_LIBRARY,
	BINARY_TYPE_STATIC_LIBRARY,
} BinaryType;

typedef struct BuildConfig {
	BinaryType	binaryType;
	const char	*name;
	const char	*binaryName;
	const char	**sourceFiles;
	const char	**defines;
	const char	**additionalIncludes;
	const char	**additionalLibPaths;
	const char	**additionalLibs;
} BuildConfig;

typedef struct BuilderOptions {
	BuildConfig	*configs;
	uint32_t	configsCount;

	int			argc;
	char		**argv;
} BuilderOptions;

typedef struct stringBuilderBuffer_t {
	uint32_t						length;
	char							*data;
	struct stringBuilderBuffer_t	*next;
} stringBuilderBuffer_t;


typedef struct stringBuilder_t {
	stringBuilderBuffer_t	*head;
	stringBuilderBuffer_t	*tail;
} stringBuilder_t;

void		StringBuilder_Appendf( stringBuilder_t *builder, const char *fmt, ... );
const char	*StringBuilder_ToString( stringBuilder_t *builder );


#ifdef BUILDER_IMPLEMENTATION

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#elif defined( __linux__ )
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef BUILDER_ASSERT
#include <assert.h>
#define BUILDER_ASSERT assert
#endif

enum {
	BUILDER_VERSION_MAJOR	= 0,
	BUILDER_VERSION_MINOR	= 15,
	BUILDER_VERSION_PATCH	= 0,
};

#define ARG_HELP_SHORT	"-h"
#define ARG_HELP_LONG	"--help"
#define ARG_CONFIG		"--config="

static bool StringStartsWith( const char *str, const char *prefix ) {
	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

static void BuilderWarning( const char *fmt, ... ) {
	printf( "WARNING: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

static void BuilderError( const char *fmt, ... ) {
	printf( "ERROR: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

// TODO: DM: 30/07/2026: replace malloc calls with a custom "Alloc()" function ptr that users can override themselves
void StringBuilder_Appendf( stringBuilder_t *builder, const char *fmt, ... ) {
	BUILDER_ASSERT( builder );
	BUILDER_ASSERT( fmt );

	va_list args;
	va_start( args, fmt );

	stringBuilderBuffer_t *buffer = (stringBuilderBuffer_t *) malloc( sizeof( stringBuilderBuffer_t ) );
	memset( buffer, 0, sizeof( stringBuilderBuffer_t ) );

	va_list argsCopy;
	va_copy( argsCopy, args );

	buffer->length = (uint32_t) vsnprintf( NULL, 0, fmt, args );

	buffer->data = (char *) malloc( buffer->length + 1 );
	vsnprintf( buffer->data, buffer->length + 1, fmt, argsCopy );
	va_end( argsCopy );
	buffer->data[buffer->length] = 0;

	// if no head then this is the first element
	if ( !builder->head ) {
		builder->head = buffer;
	} else {
		builder->tail->next = buffer;
	}

	builder->tail = buffer;

	va_end( args );
}

const char *StringBuilder_ToString( stringBuilder_t *builder ) {
	char *result = NULL;
	uint64_t totalLength = 0;
	uint64_t offset = 0;

	stringBuilderBuffer_t *current = builder->head;

	if ( !current ) {
		return NULL;
	}

	while ( current ) {
		totalLength += current->length;

		current = current->next;
	}

	totalLength += 1;

	result = (char *) malloc( totalLength * sizeof( char ) );

	current = builder->head;

	while ( current ) {
		memcpy( result + offset, current->data, current->length );

		offset += current->length;

		current = current->next;
	}

	result[totalLength - 1] = 0;

	return result;
}

static void SetCmdLineArgs( BuilderOptions *options, const int argc, char **argv ) {
	options->argc = argc;
	options->argv = argv;
}

static bool HasCommandLineArg( BuilderOptions *options, const char *arg ) {
	for ( int argIndex = 0; argIndex < options->argc; argIndex++ ) {
		if ( strcmp( options->argv[argIndex], arg ) == 0 ) {
			return true;
		}
	}

	return false;
}

static void AddBuildConfig( BuilderOptions *options, BuildConfig *config ) {
	options->configsCount++;
	options->configs = (BuildConfig *) realloc( options->configs, options->configsCount * sizeof( BuildConfig ) );

	BuildConfig *dst = &options->configs[options->configsCount - 1];

	memcpy( dst, config, sizeof( BuildConfig ) );
}

static int32_t RunProcess( const char *processAndArgs ) {
#if defined( _WIN32 )
	SECURITY_ATTRIBUTES secAttr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	PROCESS_INFORMATION	processInfo;
	HANDLE				stdoutRead;

	HANDLE stdoutWrite = NULL;
	if ( !CreatePipe( &stdoutRead, &stdoutWrite, &secAttr, 0 ) ) {
		BuilderError( "CreatePipe call failed for stdout: 0x%X.\n", GetLastError() );
		return 1;
	}

	STARTUPINFO startInfo = { sizeof( startInfo ) };
	startInfo.dwFlags = STARTF_USESTDHANDLES;
	startInfo.hStdOutput = stdoutWrite;
	startInfo.hStdError = stdoutWrite;

	char *combinedEnvVars = NULL;

	if ( !CreateProcess(
		NULL,
		(LPSTR) processAndArgs,
		NULL,
		NULL,
		true,
		CREATE_NO_WINDOW,
		combinedEnvVars,
		NULL,
		&startInfo,
		&processInfo
	) ) {
		BuilderError( "Failed to create process: 0x%X.\n", GetLastError() );
		return 1;
	}

	// close the write ends of the pipes on the parent side
	// the child inherited its own copies so this must happen before we start reading
	// otherwise the parents dangling copy keeps the pipe "open" and ReadFile below blocks forever once the child exits
	if ( !CloseHandle( stdoutWrite ) ) {
		BuilderError( "Failed to close stdout write handle: 0x%X\n", GetLastError() );
		return 1;
	}

	char buffer[1024] = {};
	DWORD bytesRead = 0;
	BOOL read = true;

	while ( ( read = ReadFile( stdoutRead, buffer, sizeof( buffer ) - 1, &bytesRead, NULL ) ) && bytesRead != 0 ) {
		buffer[bytesRead] = 0;
		printf( "%s", buffer );
	}

	if ( !read ) {
		DWORD lastError = GetLastError();

		// the child closing its end of the pipe (e.g. on exit) surfaces as ERROR_BROKEN_PIPE here - that's expected EOF, not a real failure
		if ( lastError != ERROR_BROKEN_PIPE ) {
			BuilderError( "Failed to read stdout of subprocess: 0x%X.\n", lastError );
		}
	}

	// wait for process to finish
	if ( !CloseHandle( stdoutRead ) ) {
		BuilderError( "Failed to close stdout read handle: Windows error code: 0x%X\n", GetLastError() );
		return false;
	}
	stdoutRead = NULL;

	if ( WaitForSingleObject( processInfo.hProcess, INFINITE ) != WAIT_OBJECT_0 ) {
		BuilderError( "Failed to wait for subprocess to finish: 0x%X\n", GetLastError() );
		return -1;
	}

	DWORD exitCode = 0;

	if ( !GetExitCodeProcess( processInfo.hProcess, &exitCode ) ) {
		BuilderError( "Failed to get exit code of subprocess: 0x%X\n", GetLastError() );
		return -1;
	}

	return (int32_t) exitCode;
#elif defined( __linux__ )
	int stdoutPipe[2];

	if ( pipe( stdoutPipe ) != 0 ) {
		BuilderError( "Failed to create pipe for subprocess stdout: %s.\n", strerror( errno ) );
		return -1;
	}

	const pid_t pid = fork();

	if ( pid < 0 ) {
		BuilderError( "Failed to fork subprocess: %s.\n", strerror( errno ) );
		return -1;
	}

	if ( pid == 0 ) {
		// child: fold stdout and stderr into the write end of the pipe so the parent sees combined output, then exec
		close( stdoutPipe[0] );

		dup2( stdoutPipe[1], STDOUT_FILENO );
		dup2( stdoutPipe[1], STDERR_FILENO );

		close( stdoutPipe[1] );

		execl( "/bin/sh", "sh", "-c", processAndArgs, (char *) NULL );

		// only reachable if execl failed
		fprintf( stderr, "Failed to exec subprocess: %s.\n", strerror( errno ) );
		_exit( 127 );
	}

	// parent: close the write end on our side so read() sees EOF once the child (and any of its children) close theirs
	close( stdoutPipe[1] );

	char buffer[1024] = {};
	ssize_t bytesRead = 0;

	while ( ( bytesRead = read( stdoutPipe[0], buffer, sizeof( buffer ) - 1 ) ) > 0 ) {
		buffer[bytesRead] = 0;
		printf( "%s", buffer );
	}

	close( stdoutPipe[0] );

	int status = 0;

	if ( waitpid( pid, &status, 0 ) < 0 ) {
		BuilderError( "Failed to wait for subprocess: %s.\n", strerror( errno ) );
		return -1;
	}

	if ( WIFEXITED( status ) ) {
		return WEXITSTATUS( status );
	}

	if ( WIFSIGNALED( status ) ) {
		BuilderError( "Subprocess was terminated by signal %d.\n", WTERMSIG( status ) );
		return -1;
	}

	return -1;
#else
#error Unrecognised platform.
#endif
}

static int ShowUsage( const int exitCode ) {
	// TODO: DM: 30/07/2026: write the usage/help text here
	printf(
		"Usage:\n"
		"\n"
		"\n"
	);

	return exitCode;
}

// TODO: DM: 30/07/2026: add unix support
static const char *GetFileExtensionFromBinaryType( const BinaryType binaryType ) {
#if defined( _WIN32 )
	switch ( binaryType ) {
		case BINARY_TYPE_EXE:				return ".exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".lib";
	}
#else
	switch ( binaryType ) {
		case BINARY_TYPE_EXE:				return "";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".so";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".a";
	}
#endif

	BUILDER_ASSERT( false && "Bad BinaryType.\n" );

	return NULL;
}

static int Build( BuilderOptions *options ) {
	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	const char *inputConfigName = NULL;

	for ( int argIndex = 0; argIndex < options->argc; argIndex++ ) {
		const char *arg = options->argv[argIndex];

		if ( StringStartsWith( arg, ARG_HELP_SHORT ) || StringStartsWith( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( StringStartsWith( arg, ARG_CONFIG ) ) {
			const char *equals = strchr( arg, '=' );

			if ( !equals ) {
				BuilderError( "I detected that you want to set a config, but you never gave me the equals (=) immediately after it.  You need to do that.\n" );

				return 1;
			}

			const char *configName = equals + 1;

			if ( strlen( configName ) < 1 ) {
				BuilderError( "You specified the start of the config arg, but you never actually gave me a name for the config.  I need that.\n" );

				return 1;
			}

			inputConfigName = configName;

			continue;
		}
	}

	BuildConfig *configToBuild = NULL;

	// validate input args
	{
		if ( options->configsCount == 1 ) {
			configToBuild = &options->configs[0];
		} else {
			if ( !inputConfigName ) {
				BuilderError( "No input config specified.  You must specify at least one!\n" );
				return 1;
			}
		}
	}

	printf( "Building config:\n" );

	// build the config
	{
		// compilation step
		{
			const char **sourceFile = configToBuild->sourceFiles;

			while ( *sourceFile ) {
				stringBuilder_t compileArgs = {};
				StringBuilder_Appendf( &compileArgs, "clang " );
				StringBuilder_Appendf( &compileArgs, "-c " );
				StringBuilder_Appendf( &compileArgs, "-o %s.o ", *sourceFile );
				StringBuilder_Appendf( &compileArgs, "%s ", *sourceFile );

				const char *args = StringBuilder_ToString( &compileArgs );

				printf( "%s\n", args );

				if ( RunProcess( args ) != 0 ) {
					BuilderError( "Build failed.\n" );
					return 1;
				}

				sourceFile++;
			}
		}

		// link step
		{
			stringBuilder_t linkerArgs = {};
#if defined( _WIN32 )
			// TODO: DM: 30/07/2026: remove hardcoded path
			if ( configToBuild->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
				StringBuilder_Appendf( &linkerArgs, "lib.exe " );
			} else {
				StringBuilder_Appendf( &linkerArgs, "link.exe " );
			}

			StringBuilder_Appendf( &linkerArgs, "/OUT:%s%s ", configToBuild->binaryName, GetFileExtensionFromBinaryType( configToBuild->binaryType ) );

			if ( configToBuild->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
				StringBuilder_Appendf( &linkerArgs, "/shared " );
			}

			const char **sourceFile = configToBuild->sourceFiles;
			while ( *sourceFile ) {
				StringBuilder_Appendf( &linkerArgs, "%s.o ", *sourceFile );

				sourceFile++;
			}

			const char **additionalLib = configToBuild->additionalLibs;
			while ( additionalLib && *additionalLib ) {
				StringBuilder_Appendf( &linkerArgs, "%s ", *additionalLib );

				additionalLib++;
			}
#elif defined( __linux__ )
			if ( configToBuild->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
				StringBuilder_Appendf( &linkerArgs, "ar rcs " );
				StringBuilder_Appendf( &linkerArgs, "%s%s ", configToBuild->binaryName, GetFileExtensionFromBinaryType( configToBuild->binaryType ) );

				const char **sourceFile = configToBuild->sourceFiles;
				while ( *sourceFile ) {
					StringBuilder_Appendf( &linkerArgs, "%s.o ", *sourceFile );

					sourceFile++;
				}
			} else {
				StringBuilder_Appendf( &linkerArgs, "clang " );

				if ( configToBuild->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
					StringBuilder_Appendf( &linkerArgs, "-shared " );
				}

				StringBuilder_Appendf( &linkerArgs, "-o %s%s ", configToBuild->binaryName, GetFileExtensionFromBinaryType( configToBuild->binaryType ) );

				const char **sourceFile = configToBuild->sourceFiles;
				while ( *sourceFile ) {
					StringBuilder_Appendf( &linkerArgs, "%s.o ", *sourceFile );

					sourceFile++;
				}

				const char **additionalLib = configToBuild->additionalLibs;
				while ( additionalLib && *additionalLib ) {
					StringBuilder_Appendf( &linkerArgs, "%s ", *additionalLib );

					additionalLib++;
				}
			}
#endif

			const char *args = StringBuilder_ToString( &linkerArgs );

			printf( "%s\n", args );

			if ( RunProcess( args ) != 0 ) {
				BuilderError( "Link failed.\n" );
				return 1;
			}
		}
	}

	printf( "Done\n" );

	return 0;
}

#endif // BUILDER_IMPLEMENTATION

#pragma clang diagnostic pop

#ifdef __cplusplus
}
#endif
