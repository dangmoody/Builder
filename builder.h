/*
===========================================================================

Builder

Copyright (c) 2026 Dan Moody

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

TODO: DM: 07/08/2026: table of contents (intro, installation, quick start guide, etc)

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef enum BinaryType {
	BINARY_TYPE_EXE	= 0,
	BINARY_TYPE_DYNAMIC_LIBRARY,
	BINARY_TYPE_STATIC_LIBRARY,
} BinaryType;

typedef enum LanguageVersion {
	LANGUAGE_VERSION_UNSET	= 0,
	LANGUAGE_VERSION_C89,
	LANGUAGE_VERSION_C99,
	LANGUAGE_VERSION_C11,
	LANGUAGE_VERSION_C17,
	LANGUAGE_VERSION_C23,
	LANGUAGE_VERSION_CPP11,
	LANGUAGE_VERSION_CPP14,
	LANGUAGE_VERSION_CPP17,
	LANGUAGE_VERSION_CPP20,
	LANGUAGE_VERSION_CPP23,
} LanguageVersion;

typedef enum Optimization {
	OPTIMIZATION_DISABLED	= 0,
	OPTIMIZATION_PROGRAM_SIZE,
	OPTIMIZATION_PROGRAM_SPEED,
} Optimization;

typedef struct BuildConfig {
	const char			*name;
	// Other BuildConfigs that need to be built before this one.  NULL-terminated array.
	// You only need to call AddBuildConfig() on the top-level config - its dependencies are registered and built automatically.
	// Every BuildConfig you put in here must have BuildConfig::name set.
	struct BuildConfig	**dependsOn;
	const char			*binaryName;
	// The folder the binary is placed into, relative to the file you pass into Builder.
	// If this folder doesn't exist then Builder will create it for you.
	// Leave NULL to put the binary alongside the source file.
	const char			*binaryFolder;
	// The folder that intermediate build files (object files) are placed into, relative to binaryFolder.
	// If this folder doesn't exist then Builder will create it for you.
	// Leave NULL to put intermediate files alongside the binary.
	const char			*intermediateFolder;
	const char			**sourceFiles;
	const char			**defines;
	const char			**additionalIncludes;
	const char			**additionalLibPaths;
	const char			**additionalLibs;
	const char			**warningLevels;
	const char			**ignoreWarnings;
	const char			**additionalLinkerArguments;
	BinaryType			binaryType;
	LanguageVersion		languageVersion;
	Optimization		optimization;
	bool				removeSymbols;
	bool				warningsAsErrors;
	void				( *OnPreBuild )( struct BuildConfig *config );
	void				( *OnPostBuild )( struct BuildConfig *config );
} BuildConfig;

typedef struct BuilderOptions {
	// The path to the compiler you want to build with.
	// Leave NULL to use "clang" and assume it's on your PATH.
	// On Windows, you can set this to "cl" or "cl.exe" to build with MSVC instead - Builder will locate your MSVC install automatically.
	const char	*compilerPath;

	// What version of your compiler are you expecting to build with, if any?
	// If the compiler Builder ends up using doesn't match this, Builder will log a warning but carry on building anyway.
	// Leave NULL to skip this check entirely.
	const char	*compilerVersion;

	// If no config is specified at the command line via --config=, what config do you want Builder to build by default?
	BuildConfig	*defaultConfig;

	// The command line args that come in from main().
	// You can and should set these.
	int			argc;
	char		**argv;

	// The list of configs that gets populated when calling AddBuildConfig().
	// Don't write to this directly unless you know what you're doing.
	BuildConfig	*configs;
	uint32_t	configsCount;
} BuilderOptions;

void	AddBuildConfig( BuilderOptions *options, BuildConfig *config );
int		Build( BuilderOptions *options );


#ifdef BUILDER_IMPLEMENTATION

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>
#if defined( _MSC_VER )
#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )
#pragma comment( lib, "advapi32.lib" )
#endif
#elif defined( __linux__ )
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#else
#error Unrecognised platform.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef __linux__
#include <ctype.h>
#endif

#ifndef BUILDER_ASSERT
#include <assert.h>
#define BUILDER_ASSERT assert
#endif

#ifndef BUILDER_COUNT_OF
#define BUILDER_COUNT_OF( array )	( sizeof( array ) / sizeof( array[0] ) )
#endif

#if defined( _WIN32 )
#define BUILDER_PATH_SEPARATOR '\\'
#elif defined( __linux__ )
#define BUILDER_PATH_SEPARATOR '/'
#endif

enum {
	BUILDER_VERSION_MAJOR	= 1,
	BUILDER_VERSION_MINOR	= 0,
	BUILDER_VERSION_PATCH	= 0,
};

#define ARG_HELP_SHORT	"-h"
#define ARG_HELP_LONG	"--help"
#define ARG_CONFIG		"--config="

static bool Builder_StringEquals( const char *a, const char *b ) {
	return strcmp( a, b ) == 0;
}

static bool Builder_StringStartsWith( const char *str, const char *prefix ) {
	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

static bool Builder_StringContains( const char *str, const char *substring ) {
	return strstr( str, substring ) != NULL;
}

static bool Builder_PathHasFileExtension( const char *path, const char *extension ) {
	uint64_t pathLen = strlen( path );
	uint64_t extensionLen = strlen( extension );

	if ( pathLen < extensionLen ) {
		return false;
	}

#if defined( _WIN32 )
	// filenames are case-insensitive on Windows, so ".exe" and ".EXE" must be treated as the same extension
	return _strnicmp( path + pathLen - extensionLen, extension, extensionLen ) == 0;
#elif defined( __linux__ )
	return strncmp( path + pathLen - extensionLen, extension, extensionLen ) == 0;
#else
#error Unrecognised platform.
#endif
}

// TODO: DM: 07/08/2026: replace malloc calls with a custom "Alloc()" function ptr that users can override themselves
static char *Builder_FormatString( const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );

	va_list argsCopy;
	va_copy( argsCopy, args );

	int length = vsnprintf( NULL, 0, fmt, args );
	va_end( args );

	char *result = (char *) malloc( (size_t) length + 1 );
	vsnprintf( result, (size_t) length + 1, fmt, argsCopy );
	va_end( argsCopy );

	return result;
}

static void Builder_Warning( const char *fmt, ... ) {
	printf( "WARNING: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

static void Builder_Error( const char *fmt, ... ) {
	printf( "ERROR: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

// name of the BuildConfig the user wants built: whatever --config=<name> says, or BuilderOptions::defaultConfig if that arg wasn't given
// requires BuilderOptions::argc/argv to already be set (e.g. via BuilderOptions{ .argc = argc, .argv = argv }) before this is called
static const char *Builder_GetNameOfConfigToBuild( const BuilderOptions *options ) {
	for ( int argIndex = 0; argIndex < options->argc; argIndex++ ) {
		if ( Builder_StringStartsWith( options->argv[argIndex], ARG_CONFIG ) ) {
			return options->argv[argIndex] + strlen( ARG_CONFIG );
		}
	}

	return options->defaultConfig ? options->defaultConfig->name : NULL;
}

static bool Builder_IsWarningLevelAllowed_Clang( const char *warningLevel ) {
	static const char *allowedWarningLevels[] = { "-Wall", "-Weverything", "-Wextra", "-Wpedantic" };

	for ( size_t warningLevelIndex = 0; warningLevelIndex < BUILDER_COUNT_OF( allowedWarningLevels ); warningLevelIndex++ ) {
		// TODO: DM: 06/08/2026: string checking like this is slow
		// do we hash the input string and keep a list of hashes of warning level strings and check those instead?
		if ( Builder_StringEquals( warningLevel, allowedWarningLevels[warningLevelIndex] ) ) {
			return true;
		}
	}

	return false;
}

static bool Builder_IsWarningLevelAllowed_MSVC( const char *warningLevel ) {
	static const char *allowedWarningLevels[] = { "/W0", "/W1", "/W2", "/W3", "/W4", "/Wall" };

	for ( size_t warningLevelIndex = 0; warningLevelIndex < BUILDER_COUNT_OF( allowedWarningLevels ); warningLevelIndex++ ) {
		if ( Builder_StringEquals( warningLevel, allowedWarningLevels[warningLevelIndex] ) ) {
			return true;
		}
	}

	return false;
}

typedef struct stringBuilderBuffer_t {
	uint32_t						length;
	char							*data;
	struct stringBuilderBuffer_t	*next;
} stringBuilderBuffer_t;

typedef struct stringBuilder_t {
	stringBuilderBuffer_t	*head;
	stringBuilderBuffer_t	*tail;
} stringBuilder_t;

static void StringBuilder_Destroy( stringBuilder_t *builder ) {
	stringBuilderBuffer_t *current = builder->head;

	while ( current ) {
		stringBuilderBuffer_t *next = current->next;

		free( current->data );
		free( current );

		current = next;
	}

	builder->head = NULL;
	builder->tail = NULL;
}

// TODO: DM: 30/07/2026: replace malloc calls with a custom "Alloc()" function ptr that users can override themselves
static void StringBuilder_Appendf( stringBuilder_t *builder, const char *fmt, ... ) {
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

static char *StringBuilder_ToString( stringBuilder_t *builder ) {
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

static bool Builder_GetFileLastWriteTime( const char *path, uint64_t *outTime ) {
	BUILDER_ASSERT( path );
	BUILDER_ASSERT( outTime );

#if defined( _WIN32 )
	WIN32_FILE_ATTRIBUTE_DATA attributeData;

	if ( !GetFileAttributesEx( path, GetFileExInfoStandard, &attributeData ) ) {
		return false;
	}

	*outTime = ( (uint64_t) attributeData.ftLastWriteTime.dwHighDateTime << 32 ) | attributeData.ftLastWriteTime.dwLowDateTime;

	return true;
#elif defined( __linux__ )
	struct stat fileStat;

	if ( stat( path, &fileStat ) != 0 ) {
		return false;
	}

	*outTime = (uint64_t) fileStat.st_mtime;

	return true;
#else
#error Unrecognised platform.
#endif
}

static bool Builder_FolderExists( const char *path ) {
#if defined( _WIN32 )
	DWORD attributes = GetFileAttributesA( path );

	return attributes != INVALID_FILE_ATTRIBUTES && ( attributes & FILE_ATTRIBUTE_DIRECTORY );
#elif defined( __linux__ )
	struct stat fileStat;

	return stat( path, &fileStat ) == 0 && S_ISDIR( fileStat.st_mode );
#else
#error Unrecognised platform.
#endif
}

// creates every folder in a relative path that doesn't already exist, including any missing parent folders
// e.g. "bin/win64" will create "bin" first if needed, then "bin/win64"
static bool Builder_CreateFolderIfItDoesntExist( const char *path ) {
	BUILDER_ASSERT( path );

	if ( Builder_FolderExists( path ) ) {
		return true;
	}

	size_t pathLength = strlen( path );

	char *pathCopy = (char *) malloc( pathLength + 1 );
	memcpy( pathCopy, path, pathLength + 1 );

	bool success = true;

	// temporarily truncate the copy at each path separator so parent folders get created before their children
	for ( size_t charIndex = 1; charIndex <= pathLength && success; charIndex++ ) {
		const char c = pathCopy[charIndex];

		if ( c != '/' && c != '\\' && c != 0 ) {
			continue;
		}

		pathCopy[charIndex] = 0;

		if ( !Builder_FolderExists( pathCopy ) ) {
#if defined( _WIN32 )
			success = CreateDirectoryA( pathCopy, NULL ) != 0;
#elif defined( __linux__ )
			success = mkdir( pathCopy, 0755 ) == 0;
			int err = errno;

			if ( !success ) {
				Builder_Error( "Failed to create folder \"%s\": %s\n", pathCopy, strerror( err ) );
			}
#endif
		}

		pathCopy[charIndex] = c;
	}

	free( pathCopy );

	return success;
}

static bool Builder_WriteEntireFile( const char *filename, const char *content ) {
	FILE *file = fopen( filename, "wb" );

	size_t result = fwrite( content, strlen( content ), 1, file );

	if ( !result ) {
		Builder_Error( "Failed to write to file \"%s\".\n", filename );
		fclose( file );
		return false;
	}

	fclose( file );

	return true;
}

static void SetCmdLineArgs( BuilderOptions *options, const int argc, char **argv ) {
	options->argc = argc;
	options->argv = argv;
}

static bool HasCommandLineArg( BuilderOptions *options, const char *arg ) {
	for ( int argIndex = 0; argIndex < options->argc; argIndex++ ) {
		if ( Builder_StringEquals( options->argv[argIndex], arg ) ) {
			return true;
		}
	}

	return false;
}

// growable stack of the configs currently being registered (config -> config->dependsOn[i] -> ...), used to detect cycles
// bundling the pointer/count/capacity together means only a single pointer to this struct needs to be threaded through the
// recursion below - growing builderConfigAncestry_t::items is then just a field mutation every recursive call already sees
// TODO: DM: 08/08/2026: Tom's growable array
typedef struct builderConfigAncestry_t {
	BuildConfig	**items;
	uint32_t	count;
	uint32_t	capacity;
} builderConfigAncestry_t;

static void Builder_ConfigStackPush( builderConfigAncestry_t *stack, BuildConfig *config ) {
	if ( stack->count == stack->capacity ) {
		stack->capacity = stack->capacity ? stack->capacity * 2 : 8;
		stack->items = (BuildConfig **) realloc( stack->items, stack->capacity * sizeof( BuildConfig * ) );
	}

	stack->items[stack->count++] = config;
}

static void AddBuildConfigInternal( BuilderOptions *options, BuildConfig *config, builderConfigAncestry_t *ancestry, BuildConfig **outConfigs, uint32_t *outConfigsCount ) {
	// if no ancestry then dont do circular dependency checking
	if ( ancestry ) {
		for ( uint32_t ancestorIndex = 0; ancestorIndex < ancestry->count; ancestorIndex++ ) {
			if ( ancestry->items[ancestorIndex] == config ) {
				stringBuilder_t cycle = {};

				for ( uint32_t cycleIndex = ancestorIndex; cycleIndex < ancestry->count; cycleIndex++ ) {
					const char *cycleConfigName = ancestry->items[cycleIndex]->name;
					StringBuilder_Appendf( &cycle, "%s -> ", cycleConfigName ? cycleConfigName : "(unnamed config)" );
				}

				StringBuilder_Appendf( &cycle, "%s", config->name ? config->name : "(unnamed config)" );

				char *cycleString = StringBuilder_ToString( &cycle );
				Builder_Error( "Cyclic BuildConfig::dependsOn detected: %s\n", cycleString );
				free( cycleString );

				StringBuilder_Destroy( &cycle );

				exit( 1 );
			}
		}

		Builder_ConfigStackPush( ancestry, config );
	}

	// register dependencies first so they show up (and get built) ahead of the config that needs them
	{
		BuildConfig **dependency = config->dependsOn;

		while ( dependency && *dependency ) {
			AddBuildConfigInternal( options, *dependency, ancestry, outConfigs, outConfigsCount );

			dependency++;
		}
	}

	if ( ancestry ) {
		ancestry->count--;
	}

	// multiple configs can rely on the same config (e.g. configs A and B may both rely on config C)
	// so we still need this duplicate check anyway
	for ( uint32_t configIndex = 0; configIndex < *outConfigsCount; configIndex++ ) {
		if ( ( *outConfigs )[configIndex].name && config->name && Builder_StringEquals( ( *outConfigs )[configIndex].name, config->name ) ) {
			return;
		}
	}

	*outConfigs = (BuildConfig *) realloc( *outConfigs, ++(*outConfigsCount) * sizeof( BuildConfig ) );

	BuildConfig *dst = &( *outConfigs )[(*outConfigsCount) - 1];

	memcpy( dst, config, sizeof( BuildConfig ) );
}

void AddBuildConfig( BuilderOptions *options, BuildConfig *config ) {
	builderConfigAncestry_t ancestry = {};

	AddBuildConfigInternal( options, config, &ancestry, &options->configs, &options->configsCount );

	free( ancestry.items );
}

static int32_t Builder_RunProcess( const char *processAndArgs, char **outCapturedOutput ) {
#if defined( _WIN32 )
	SECURITY_ATTRIBUTES secAttr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	PROCESS_INFORMATION	processInfo;
	HANDLE				stdoutRead;

	HANDLE stdoutWrite = NULL;
	if ( !CreatePipe( &stdoutRead, &stdoutWrite, &secAttr, 0 ) ) {
		Builder_Error( "CreatePipe call failed for stdout: 0x%X.\n", GetLastError() );
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
		Builder_Error( "Failed to create process: 0x%X.\n", GetLastError() );
		return 1;
	}

	// close the write ends of the pipes on the parent side
	// the child inherited its own copies so this must happen before we start reading
	// otherwise the parents dangling copy keeps the pipe "open" and ReadFile below blocks forever once the child exits
	if ( !CloseHandle( stdoutWrite ) ) {
		Builder_Error( "Failed to close stdout write handle: 0x%X\n", GetLastError() );
		return 1;
	}

	char buffer[1024] = {};
	DWORD bytesRead = 0;
	BOOL read = true;

	stringBuilder_t capturedOutput = {};

	while ( ( read = ReadFile( stdoutRead, buffer, sizeof( buffer ) - 1, &bytesRead, NULL ) ) && bytesRead != 0 ) {
		buffer[bytesRead] = 0;

		if ( outCapturedOutput ) {
			StringBuilder_Appendf( &capturedOutput, "%s", buffer );
		} else {
			printf( "%s", buffer );
		}
	}

	if ( outCapturedOutput ) {
		*outCapturedOutput = StringBuilder_ToString( &capturedOutput );
	}

	StringBuilder_Destroy( &capturedOutput );

	if ( !read ) {
		DWORD lastError = GetLastError();

		// the child closing its end of the pipe (e.g. on exit) surfaces as ERROR_BROKEN_PIPE here - that's expected EOF, not a real failure
		if ( lastError != ERROR_BROKEN_PIPE ) {
			Builder_Error( "Failed to read stdout of subprocess: 0x%X.\n", lastError );
		}
	}

	// wait for process to finish
	if ( !CloseHandle( stdoutRead ) ) {
		Builder_Error( "Failed to close stdout read handle: Windows error code: 0x%X\n", GetLastError() );
		return false;
	}
	stdoutRead = NULL;

	if ( WaitForSingleObject( processInfo.hProcess, INFINITE ) != WAIT_OBJECT_0 ) {
		Builder_Error( "Failed to wait for subprocess to finish: 0x%X\n", GetLastError() );
		return -1;
	}

	DWORD exitCode = 0;

	if ( !GetExitCodeProcess( processInfo.hProcess, &exitCode ) ) {
		Builder_Error( "Failed to get exit code of subprocess: 0x%X\n", GetLastError() );
		return -1;
	}

	return (int32_t) exitCode;
#elif defined( __linux__ )
	int stdoutPipe[2];

	if ( pipe( stdoutPipe ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to create pipe for subprocess stdout: %s.\n", strerror( err ) );
		return -1;
	}

	pid_t pid = fork();
	int err = errno;

	if ( pid < 0 ) {
		Builder_Error( "Failed to fork subprocess: %s.\n", strerror( err ) );
		return -1;
	}

	if ( pid == 0 ) {
		// child: fold stdout and stderr into the write end of the pipe so the parent sees combined output, then exec
		if ( close( stdoutPipe[0] ) == -1 ) {
			err = errno;
			Builder_Error( "Failed to close stdout pipe: %s\n", strerror( err ) );
		}

		if ( dup2( stdoutPipe[1], STDOUT_FILENO ) == -1 ) {
			err = errno;
			Builder_Error( "Failed to duplicate stdout pipe: %s\n", strerror( err ) );
		}

		if ( dup2( stdoutPipe[1], STDERR_FILENO ) == -1 ) {
			err = errno;
			Builder_Error( "Failed to duplicate stderr pipe: %s\n", strerror( err ) );
		}

		if ( close( stdoutPipe[1] ) == -1 ) {
			err = errno;
			Builder_Error( "Failed to close stderr pipe: %s\n", strerror( err ) );
		}

		if ( execl( "/bin/sh", "sh", "-c", processAndArgs, (char *) NULL ) == -1 ) {
			err = errno;
			Builder_Error( "Failed to exec subprocess: %s\n", strerror( err ) );
			_exit( 127 );
		}
	}

	// parent: close the write end on our side so read() sees EOF once the child (and any of its children) close theirs
	if ( close( stdoutPipe[1] ) == -1 ) {
		err = errno;
		Builder_Error( "Failed to close stdout pipe: %s\n", strerror( err ) );
		return -1;
	}

	char buffer[1024] = {};
	ssize_t bytesRead = 0;

	stringBuilder_t capturedOutput = {};

	while ( ( bytesRead = read( stdoutPipe[0], buffer, sizeof( buffer ) - 1 ) ) > 0 ) {
		buffer[bytesRead] = 0;

		if ( outCapturedOutput ) {
			StringBuilder_Appendf( &capturedOutput, "%s", buffer );
		} else {
			printf( "%s", buffer );
		}
	}

	err = errno;

	// a negative return here means read() itself failed - distinct from bytesRead == 0, which is just normal EOF once the child closes the pipe
	if ( bytesRead < 0 ) {
		Builder_Error( "Failed to read stdout of subprocess: %s.\n", strerror( err ) );
	}

	if ( outCapturedOutput ) {
		*outCapturedOutput = StringBuilder_ToString( &capturedOutput );
	}

	StringBuilder_Destroy( &capturedOutput );

	if ( close( stdoutPipe[0] ) == -1 ) {
		err = errno;
		Builder_Error( "Failed to close stdout pipe: %s\n", strerror( err ) );
		return -1;
	}

	int status = 0;

	if ( waitpid( pid, &status, 0 ) < 0 ) {
		err = errno;
		Builder_Error( "Failed to wait for subprocess: %s.\n", strerror( err ) );
		return -1;
	}

	if ( WIFEXITED( status ) ) {
		return WEXITSTATUS( status );
	}

	if ( WIFSIGNALED( status ) ) {
		Builder_Error( "Subprocess was terminated by signal %d.\n", WTERMSIG( status ) );
		return -1;
	}

	return -1;
#else
#error Unrecognised platform.
#endif
}

#define Builder_RebuildSelf( argc, argv ) Builder_RebuildSelfInternal( (argc), (argv), __FILE__ )

// DO NOT CALL THIS DIRECTLY
// CALL THE MACRO VERSION INSTEAD
static void Builder_RebuildSelfInternal( int argc, char **argv, const char *sourceFile ) {
	const char *binaryPath = argv[0];

	// we need the exe filename on windows to end with ".exe"
	// otherwise the file will fail to be found
#ifdef _WIN32
	if ( !Builder_PathHasFileExtension( binaryPath, ".exe" ) ) {
		binaryPath = Builder_FormatString( "%s.exe", argv[0] );
	}

	// if the old binary was left around from the previous build, clean it up now
	{
		char *oldBackupPath = Builder_FormatString( "%s.rebuild.old", binaryPath );
		DeleteFile( oldBackupPath );
		free( oldBackupPath );
	}
#endif

	uint64_t sourceTime = 0;
	uint64_t binaryTime = 0;

	if ( !Builder_GetFileLastWriteTime( sourceFile, &sourceTime ) ) {
		Builder_Error( "Couldn't stat source file '%s'.\n", sourceFile );
		exit( 1 );
	}

	// binary missing/unstatable, treat as "always rebuild" rather than erroring
	bool binaryExists = Builder_GetFileLastWriteTime( binaryPath, &binaryTime );

	if ( binaryExists && binaryTime >= sourceTime ) {
		// already up to date, fall through and let main() continue as normal
		return;
	}

	printf( "'%s' is stale, rebuilding...\n", binaryPath );

	stringBuilder_t tempPathBuilder = {};
	StringBuilder_Appendf( &tempPathBuilder, "%s.rebuild.tmp", binaryPath );
	const char *tempBinaryPath = StringBuilder_ToString( &tempPathBuilder );

	stringBuilder_t compileArgs = {};
	StringBuilder_Appendf( &compileArgs, "clang " );
	StringBuilder_Appendf( &compileArgs, "-o %s ", tempBinaryPath );
	StringBuilder_Appendf( &compileArgs, "%s ", sourceFile );

	const char *compileCmd = StringBuilder_ToString( &compileArgs );

	printf( "%s\n", compileCmd );

	if ( Builder_RunProcess( compileCmd, NULL ) != 0 ) {
		Builder_Error( "failed to rebuild '%s'.\n", binaryPath );

#if defined( _WIN32 )
		DeleteFile( tempBinaryPath );
#elif defined( __linux__ )
		if ( unlink( tempBinaryPath ) == -1 ) {
			int err = errno;
			Builder_Error( "Failed to unlink binary: %s\n", strerror( err ) );
		}
#endif

		exit( 1 );
	}

	// atomically swap the freshly built binary into place
	// never overwrite binaryPath in place since writing directly into a currently-executing image fails
#if defined( _WIN32 )
	// a currently-running process cant MoveFileEx-replace its own on-disk image directly (fails with ERROR_ACCESS_DENIED)
	// rename it out of the way first, then move the freshly built binary into the now-vacated name
	// the running image stays mapped and executing under its backup name until this process re-execs below
	stringBuilder_t backupPathBuilder = {};
	StringBuilder_Appendf( &backupPathBuilder, "%s.rebuild.old", binaryPath );
	const char *backupBinaryPath = StringBuilder_ToString( &backupPathBuilder );

	if ( !MoveFileEx( binaryPath, backupBinaryPath, MOVEFILE_REPLACE_EXISTING ) ) {
		Builder_Error( "Failed to move currently-running '%s' out of the way: 0x%X\n", binaryPath, GetLastError() );
		exit( 1 );
	}

	if ( !MoveFileEx( tempBinaryPath, binaryPath, MOVEFILE_REPLACE_EXISTING ) ) {
		Builder_Error( "Failed to replace '%s' with rebuilt binary: 0x%X\n", binaryPath, GetLastError() );
		exit( 1 );
	}
#elif defined( __linux__ )
	if ( rename( tempBinaryPath, binaryPath ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to replace '%s' with rebuilt binary: %s\n", binaryPath, strerror( err ) );
		exit( 1 );
	}
#endif

	// re-exec the freshly rebuilt binary with the original argv
	// never fall through to running the (now stale-in-memory) code of the process currently executing
#if defined( _WIN32 )
	stringBuilder_t execArgs = {};
	StringBuilder_Appendf( &execArgs, "\"%s\" ", binaryPath );

	for ( int argIndex = 1; argIndex < argc; argIndex++ ) {
		StringBuilder_Appendf( &execArgs, "\"%s\" ", argv[argIndex] );
	}

	const char *execCmd = StringBuilder_ToString( &execArgs );

	int32_t exitCode = Builder_RunProcess( execCmd, NULL );

	free( (void*)execCmd );

	exit( exitCode );
#elif defined( __linux__ )
	if ( execv( binaryPath, argv ) == -1 ) {
		int err = errno;
		Builder_Error( "Failed to re-exec '%s': %s\n", binaryPath, strerror( err ) );

		exit( 1 );
	}
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

static const char *Builder_GetFileExtensionFromBinaryType( const BinaryType binaryType ) {
#if defined( _WIN32 )
	switch ( binaryType ) {
		case BINARY_TYPE_EXE:				return ".exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".lib";
	}
#elif defined( __linux__ )
	switch ( binaryType ) {
		case BINARY_TYPE_EXE:				return "";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".so";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".a";
	}
#else
#error Unrecognised platform.
#endif

	BUILDER_ASSERT( false && "Bad BinaryType.\n" );

	return NULL;
}

static void Builder_AppendBinaryPath( stringBuilder_t *sb, const BuildConfig *config ) {
	if ( config->binaryFolder ) {
		StringBuilder_Appendf( sb, "%s%c", config->binaryFolder, BUILDER_PATH_SEPARATOR );
	}

	StringBuilder_Appendf( sb, "%s%s ", config->binaryName, Builder_GetFileExtensionFromBinaryType( config->binaryType ) );
}

// intermediateFolder may be NULL, in which case the intermediate file is placed alongside sourceFile instead
static void Builder_AppendIntermediateFilePath( stringBuilder_t *sb, const char *intermediateFolder, const char *sourceFile ) {
	if ( !intermediateFolder ) {
		StringBuilder_Appendf( sb, "%s.o ", sourceFile );
		return;
	}

	const char *fileName = sourceFile;

	for ( const char *c = sourceFile; *c; c++ ) {
		if ( *c == '/' || *c == '\\' ) {
			fileName = c + 1;
		}
	}

	const char *extension = strrchr( fileName, '.' );
	size_t fileNameLength = extension ? (size_t) ( extension - fileName ) : strlen( fileName );

	StringBuilder_Appendf( sb, "%s%c%.*s.o ", intermediateFolder, BUILDER_PATH_SEPARATOR, (int) fileNameLength, fileName );
}

typedef struct {
	uint64_t	sizeBytes;
	uint64_t	lastWriteTime;
	bool		isDirectory;
	const char	*filename;
	const char	*fullFilename;
} fileInfo_t;

typedef void ( *builderFileVisitCallback_t )( fileInfo_t *fileInfo, void *data );

typedef enum {
	BUILDER_FILE_VISIT_FILES		= 1 << 0,
	BUILDER_FILE_VISIT_FOLDERS		= 1 << 1,
	BUILDER_FILE_VISIT_RECURSIVE	= 1 << 2,
} builderFileVisitFlagBits_t;
typedef uint32_t builderFileVisitFlags_t;

static bool Builder_VisitFiles( const char *path, const builderFileVisitFlags_t visitFlags, builderFileVisitCallback_t callback, void *data ) {
	BUILDER_ASSERT( path );
	BUILDER_ASSERT( callback );

	// TODO: DM: 05/08/2026: Tom's chunked array
	uint32_t directoriesCount = 0;
	const char **directories = (const char **)malloc( 1 * sizeof( char * ) );
	directories[directoriesCount++] = path;

	uint32_t dirIndex = 0;

	while ( dirIndex < directoriesCount ) {
		const char *dir = directories[dirIndex];

		dirIndex += 1;

		size_t dirLength = strlen( dir );
		bool dirHasTrailingSeparator = dirLength > 0 && ( dir[dirLength - 1] == '\\' || dir[dirLength - 1] == '/' );

#if defined( _WIN32 )
		char *searchPath = Builder_FormatString( dirHasTrailingSeparator ? "%s*" : "%s\\*", dir );

		WIN32_FIND_DATA findData = {};
		HANDLE handle = FindFirstFile( searchPath, &findData );

		if ( handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		while ( 1 ) {
			fileInfo_t fileInfo = {
				.sizeBytes		= ( (uint64_t) findData.nFileSizeHigh << 32 ) | findData.nFileSizeLow,
				.lastWriteTime	= ( (uint64_t) findData.ftLastWriteTime.dwHighDateTime << 32 ) | findData.ftLastWriteTime.dwLowDateTime,
				.isDirectory	= (bool) ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ),
				.filename		= findData.cFileName,
				.fullFilename	= Builder_FormatString( dirHasTrailingSeparator ? "%s%s" : "%s\\%s", dir, findData.cFileName ),
			};

			if ( fileInfo.isDirectory ) {
				if ( !Builder_StringEquals( findData.cFileName, "." ) && !Builder_StringEquals( findData.cFileName, ".." ) ) {
					if ( visitFlags & BUILDER_FILE_VISIT_FOLDERS ) {
						callback( &fileInfo, data );
					}

					if ( visitFlags & BUILDER_FILE_VISIT_RECURSIVE ) {
						directories = (const char **)realloc( directories, ++directoriesCount * sizeof( char * ) );
						directories[directoriesCount - 1] = fileInfo.fullFilename;
					}
				}
			} else if ( visitFlags & BUILDER_FILE_VISIT_FILES ) {
				callback( &fileInfo, data );
			}

			if ( !FindNextFile( handle, &findData ) ) {
				break;
			}
		}

		if ( !FindClose( handle ) ) {
			return false;
		}
#elif defined( __linux__ )
		DIR *handle = opendir( dir );
		int err = errno;

		if ( !handle ) {
			Builder_Error( "Failed to open folder \"%s\": %s\n", dir, strerror( err ) );
			return false;
		}

		struct dirent *entry = NULL;

		while ( ( entry = readdir( handle ) ) != NULL ) {
			if ( Builder_StringEquals( entry->d_name, "." ) || Builder_StringEquals( entry->d_name, ".." ) ) {
				continue;
			}

			char *fullFilename = Builder_FormatString( dirHasTrailingSeparator ? "%s%s" : "%s/%s", dir, entry->d_name );

			struct stat fileStat = {};

			if ( stat( fullFilename, &fileStat ) != 0 ) {
				err = errno;
				Builder_Error( "Failed to stat \"%s\": %s\n", fullFilename, strerror( err ) );
				return false;
			}

			fileInfo_t fileInfo = {
				.sizeBytes		= (uint64_t) fileStat.st_size,
				.lastWriteTime	= (uint64_t) fileStat.st_mtime,
				.isDirectory	= (bool) S_ISDIR( fileStat.st_mode ),
				.filename		= entry->d_name,
				.fullFilename	= fullFilename,
			};

			if ( fileInfo.isDirectory ) {
				if ( visitFlags & BUILDER_FILE_VISIT_FOLDERS ) {
					callback( &fileInfo, data );
				}

				if ( visitFlags & BUILDER_FILE_VISIT_RECURSIVE ) {
					directories = realloc( directories, ++directoriesCount * sizeof( char * ) );
					directories[directoriesCount - 1] = fileInfo.fullFilename;
				}
			} else if ( visitFlags & BUILDER_FILE_VISIT_FILES ) {
				callback( &fileInfo, data );
			}
		}

		if ( closedir( handle ) != 0 ) {
			err = errno;
			Builder_Error( "Failed to close folder \"%s\": %s\n", dir, strerror( err ) );
			return false;
		}
#endif
	}

	free( directories );
	directories = NULL;

	return true;
}

typedef struct {
	const char **files;
	uint32_t count;
	uint32_t capacity;
} builderFileGlobResult_t;

// There could be an optimisation here if we store whether there is an asterisk
// As slice comparison can early out if count is not equal for the price of more memory?
// We should wait for some more consistent performance benchmarks to test the difference
typedef struct {
	const char *begin;
	uint32_t count;
} builderStringSlice_t;

typedef struct {
	builderStringSlice_t *data;
	uint32_t count;
} builderStringSliceArray_t;

typedef struct {
	const builderStringSliceArray_t patternSlices;
	const uint32_t searchPathLen;
	builderFileGlobResult_t *globResults;
} builderGlobVisitCallbackData_t;

static void Builder_AddGlobbedFile( builderFileGlobResult_t *fileResults, const char *file ) {
	if ( fileResults->count == fileResults->capacity ) {
		fileResults->capacity = fileResults->capacity ? fileResults->capacity * 2 : 8;
		fileResults->files = (const char **) realloc( fileResults->files, fileResults->capacity * sizeof( const char * ) );
	}

	fileResults->files[fileResults->count++] = file;
}

// remember to free sliceArray.data
static builderStringSliceArray_t Builder_SliceFilePath( const char *filePath ) {
	const char *pathIterator = filePath; 
	builderStringSliceArray_t sliceArray = {
		.data = NULL,
		.count = 0
	};

	// we currently iterate over it twice as it fits with the allocation strategy, this is not the best way
	// another contender for chunked array?
	uint32_t numSlices = (*pathIterator != '\\' && *pathIterator != '/') ? 1 : 0; // the first bit is also a slice that could be missed if no slash at front
	while( *pathIterator != '\0' ) {
		// since this is supposed to be a file it shouldn't end in a slash so each time we hit a slash
		// there is a slice between the current slashes and the next slash / end of full filename
		if ( *pathIterator == '\\' || *pathIterator == '/' ) {
			while ( *pathIterator == '\\' || *pathIterator == '/' ) {
				++pathIterator;
				if ( *pathIterator == '\0' ) {
					printf( "Error: Tried to slice path that ended in a slash \"%s\", this shouldn't be happening.\n", filePath );
					return sliceArray;
				}
			}
			++numSlices;
		} else {
			++pathIterator;
		}
	}

	// I don't like that we are somewhat hiding this allocation
	sliceArray.data = (builderStringSlice_t *) malloc( numSlices * sizeof( builderStringSlice_t ) );
	sliceArray.count = numSlices;

	pathIterator = filePath;
	for (uint32_t slice = 0; slice < numSlices; ++slice) {
		while ( *pathIterator == '\\' || *pathIterator == '/' ) {
			++pathIterator;
		}

		sliceArray.data[slice].begin = pathIterator;
		sliceArray.data[slice].count = 0;
		while ( *pathIterator != '\\' && *pathIterator != '/' && *pathIterator != '\0' ) {
			++sliceArray.data[slice].count;
			++pathIterator;
		}
	}

	return sliceArray;
}

static bool Builder_SliceMatchesPattern( const builderStringSlice_t *patternSlice, const builderStringSlice_t *pathSlice ) {
	if ( pathSlice->count == 0 ) {
		return false;
	}

	uint32_t afterLastWildcard = 0;
	uint32_t patternIndex = 0;
	for (uint32_t pathIndex = 0; pathIndex < pathSlice->count; ++pathIndex ) {
		// there are more characters that haven't been matched
		if ( patternIndex == patternSlice->count ) {
			return false;
		}

		// keep consuming wildcards to reach next match
		while ( patternSlice->begin[patternIndex] == '*' ) {
			afterLastWildcard = ++patternIndex;
			if ( patternIndex == patternSlice->count ) {
				return true; // rest of the characters are matched via wildcard
			}
		}

		// consume match resetting if match fails (TODO: AK: 11/08/2026: explain this better?)
		if ( patternSlice->begin[patternIndex++] != pathSlice->begin[pathIndex] ) {
			patternIndex = afterLastWildcard;
		}
	}

	// since we consumed all characters in match and filename it's a match
	return patternIndex == patternSlice->count;
}

static bool Builder_PathMatchesPattern( const builderStringSliceArray_t *patternSliceArray, const builderStringSliceArray_t *pathSliceArray ) {
	if ( pathSliceArray == NULL || patternSliceArray == NULL || 
		pathSliceArray->count == 0 || patternSliceArray->count == 0 ) {
		return false;
	}

	bool inRecursiveGlob = false;
	uint32_t patternIndex = 0;
	uint32_t pathIndex = 0;
	while ( patternIndex < patternSliceArray->count ) {
		const builderStringSlice_t *patternSlice = &patternSliceArray->data[patternIndex++];

		if ( patternSlice->count == 1 && patternSlice->begin[0] == '*' ) { // case of /*/ - we should just check this in Builder_SliceMatchesPattern
			if ( pathIndex++ == pathSliceArray->count ) {
				return false; // no more folders to consume
			}
		} else if ( patternSlice->count == 2 && patternSlice->begin[0] == '*' &&  patternSlice->begin[1] == '*' ) { // case of /**/
			inRecursiveGlob = true;
		} else {
			bool foundMatch = false;
			while ( pathIndex < pathSliceArray->count ) {
				const builderStringSlice_t *pathSlice = &pathSliceArray->data[pathIndex++];

				if ( Builder_SliceMatchesPattern( patternSlice, pathSlice) ) {
					foundMatch = true;
					break;
				} else if ( !inRecursiveGlob ) { // we can't fail a match if not globbing recursively
					return false;
				}
			}

			// there was something to match and we didn't match it
			if ( !foundMatch ) {
				return false;
			}

			// if we have reached the end then we actually matched everything and we are done
			if (pathIndex == pathSliceArray->count && patternIndex == patternSliceArray->count) {
				return true;
			}
		}
	}

	// if the pattern ended in a recursive glob then
	// we can match anything remaining in the path
	return inRecursiveGlob;
}

static void Builder_GlobVisitCallback( fileInfo_t *fileInfo, void *data ) {
	builderGlobVisitCallbackData_t *callbackData = (builderGlobVisitCallbackData_t *)data;

	uint32_t filenameLen = strnlen( fileInfo->filename, 255 ); // filename length maxes here I think?
	uint32_t fullFilenameLen = strnlen( fileInfo->fullFilename, MAX_PATH + filenameLen );

	if ( fullFilenameLen - filenameLen < callbackData->searchPathLen ) {
		printf( "Error: Search path length was longer than full file path for file %s\n", fileInfo->fullFilename );
		return;
	}

	// we could do a heavyweight verification that the search path matches here,
	// but really it should match from the call sites
	const char* matchStart = fileInfo->fullFilename + callbackData->searchPathLen;
	const builderStringSliceArray_t pathSlices = Builder_SliceFilePath( matchStart );

	if ( Builder_PathMatchesPattern( &callbackData->patternSlices, &pathSlices ) ) {
		Builder_AddGlobbedFile( callbackData->globResults, fileInfo->fullFilename ); // Do we need to copy the filename string?
	}

	free( pathSlices.data );
}

static builderFileGlobResult_t Builder_GlobFiles( const char **globPatterns ) {
	builderFileGlobResult_t globResult = {
		.files = NULL,
		.count = 0,
		.capacity = 0
	};
	
	// setup re-usable search path allocation
	char *const searchPath = (char *const) malloc( ( MAX_PATH + 1 ) * sizeof( const char ) );
	while ( globPatterns && *globPatterns) {
		// start by copying and seeking to find if there is an asterisk, and then rewind to just after the preceding slash (or start if it is say **/*.cpp)
		const char* pattern = *globPatterns;
		while ( *pattern != '\0' )  {  // we could also early out if they put any erroneous characters
			if ( *(pattern++) == '*' ) {
				while ( --pattern != *globPatterns ) {
					if ( *pattern == '\\' || *pattern == '/' ) {
						++pattern;
						break;
					}
				}
				break;
			}
		}

		if ( *pattern == '\0' ) {
			// should I just let the compile step handle the empty strings?
			if ( pattern != *globPatterns ) {
				Builder_AddGlobbedFile( &globResult, *globPatterns ); // yes I realise this wasn't globbed \_O_O_/
				++globPatterns;
			}
			continue;
		}

		const uint32_t globPathLen = pattern - *globPatterns;
		if ( globPathLen != 0 ) {
			if ( globPathLen > MAX_PATH ) {
				printf( "Warning: Skipping pattern %s, path was larger than max path.\n", *globPatterns );
				continue;
			}
			memcpy( searchPath, *globPatterns, globPathLen );	
		}
		searchPath[globPathLen] = '\0';
		
		builderGlobVisitCallbackData_t callbackData = {
			.patternSlices = Builder_SliceFilePath(pattern),
			.globResults = &globResult,
			.searchPathLen = globPathLen
		};

		builderFileVisitFlags_t visitFlags = BUILDER_FILE_VISIT_FILES;
		if (callbackData.patternSlices.count > 1) { // we are matching folders too
			visitFlags |= BUILDER_FILE_VISIT_RECURSIVE;
		}
		if ( !Builder_VisitFiles( searchPath, visitFlags, &Builder_GlobVisitCallback, &callbackData ) ) {
			printf( "Warning: Found no matches for pattern %s, at search path %s.\n", *globPatterns, searchPath );
		}

		free( callbackData.patternSlices.data );
		++globPatterns;
	}

	free( searchPath );
	return globResult;
}

#ifdef _WIN32
// the Windows SDK only ships these Setup.Configuration interfaces as C++ (STDMETHOD/DECLSPEC_UUID expand to virtual methods via inheritance)
// so for plain C we declare the vtables by hand instead
// same memory layout, called as This->vtable->Method( This, ... )
typedef struct ISetupInstance ISetupInstance;
typedef struct {
	HRESULT	( STDMETHODCALLTYPE *QueryInterface )( ISetupInstance *This, REFIID riid, void **ppvObject );
	ULONG	( STDMETHODCALLTYPE *AddRef )( ISetupInstance *This );
	ULONG	( STDMETHODCALLTYPE *Release )( ISetupInstance *This );
	HRESULT	( STDMETHODCALLTYPE *GetInstanceId )( ISetupInstance *This, BSTR *pbstrInstanceId );
	HRESULT	( STDMETHODCALLTYPE *GetInstallDate )( ISetupInstance *This, LPFILETIME pInstallDate );
	HRESULT	( STDMETHODCALLTYPE *GetInstallationName )( ISetupInstance *This, BSTR *pbstrInstallationName );
	HRESULT	( STDMETHODCALLTYPE *GetInstallationPath )( ISetupInstance *This, BSTR *pbstrInstallationPath );
	HRESULT	( STDMETHODCALLTYPE *GetInstallationVersion )( ISetupInstance *This, BSTR *pbstrInstallationVersion );
	HRESULT	( STDMETHODCALLTYPE *GetDisplayName )( ISetupInstance *This, LCID lcid, BSTR *pbstrDisplayName );
	HRESULT	( STDMETHODCALLTYPE *GetDescription )( ISetupInstance *This, LCID lcid, BSTR *pbstrDescription );
	HRESULT	( STDMETHODCALLTYPE *ResolvePath )( ISetupInstance *This, LPCOLESTR pwszRelativePath, BSTR *pbstrAbsolutePath );
} ISetupInstanceVTable;

struct ISetupInstance {
	ISetupInstanceVTable	*vtable;
};

typedef struct IEnumSetupInstances IEnumSetupInstances;
typedef struct {
	HRESULT	( STDMETHODCALLTYPE *QueryInterface )( IEnumSetupInstances *This, REFIID riid, void **ppvObject );
	ULONG	( STDMETHODCALLTYPE *AddRef )( IEnumSetupInstances *This );
	ULONG	( STDMETHODCALLTYPE *Release )( IEnumSetupInstances *This );
	HRESULT	( STDMETHODCALLTYPE *Next )( IEnumSetupInstances *This, ULONG celt, ISetupInstance **rgelt, ULONG *pceltFetched );
	HRESULT	( STDMETHODCALLTYPE *Skip )( IEnumSetupInstances *This, ULONG celt );
	HRESULT	( STDMETHODCALLTYPE *Reset )( IEnumSetupInstances *This );
	HRESULT	( STDMETHODCALLTYPE *Clone )( IEnumSetupInstances *This, IEnumSetupInstances **ppenum );
} IEnumSetupInstancesVTable;

struct IEnumSetupInstances {
	IEnumSetupInstancesVTable	*vtable;
};

typedef struct ISetupConfiguration ISetupConfiguration;
typedef struct {
	HRESULT	( STDMETHODCALLTYPE *QueryInterface )( ISetupConfiguration *This, REFIID riid, void **ppvObject );
	ULONG	( STDMETHODCALLTYPE *AddRef )( ISetupConfiguration *This );
	ULONG	( STDMETHODCALLTYPE *Release )( ISetupConfiguration *This );
	HRESULT	( STDMETHODCALLTYPE *EnumInstances )( ISetupConfiguration *This, IEnumSetupInstances **ppEnumInstances );
	HRESULT	( STDMETHODCALLTYPE *GetInstanceForCurrentProcess )( ISetupConfiguration *This, ISetupInstance **ppInstance );
	HRESULT	( STDMETHODCALLTYPE *GetInstanceForPath )( ISetupConfiguration *This, LPCWSTR wzPath, ISetupInstance **ppInstance );
} ISetupConfigurationVTable;

struct ISetupConfiguration {
	ISetupConfigurationVTable	*vtable;
};

typedef struct {
	int32_t	v0, v1, v2;
} builderMSVCVersion_t;

typedef struct {
	const char				*rootFolder;
	const char				*includePath;
	const char				*libPath;

	builderMSVCVersion_t	version;
} builderMSVCInstall_t;

// TODO: DM: 05/08/2026: Tom's chunked array
typedef struct {
	builderMSVCInstall_t	*installs;
	uint32_t				installsCount;
} builderFoundMSVCInstallData_t;

typedef struct {
	int32_t	v0, v1, v2, v3;
} builderWindowsSDKVersion_t;

typedef struct {
	const char					*rootFolder;
	const char					*ucrtIncludePath;
	const char					*umIncludePath;
	const char					*sharedIncludePath;
	const char					*ucrtLibPath;
	const char					*umLibPath;

	builderWindowsSDKVersion_t	version;
} builderWindowsSDKInstall_t;

// TODO: DM: 05/08/2026: Tom's chunked array
typedef struct {
	builderWindowsSDKVersion_t	*versions;
	uint32_t					versionsCount;
} builderFoundWindowsSDKVersionData_t;

static void OnWindowsSDKVersionFound( fileInfo_t *fileInfo, void *data ) {
	builderFoundWindowsSDKVersionData_t *foundData = (builderFoundWindowsSDKVersionData_t *) data;

	builderWindowsSDKVersion_t version = {};

	if ( sscanf( fileInfo->filename, "%d.%d.%d.%d", &version.v0, &version.v1, &version.v2, &version.v3 ) != 4 ) {
		return;
	}

	foundData->versions = (builderWindowsSDKVersion_t *) realloc( foundData->versions, ( ++foundData->versionsCount ) * sizeof( builderWindowsSDKVersion_t ) );
	foundData->versions[foundData->versionsCount - 1] = version;
}

static int Builder_CompareWindowsSDKVersions( const void *a, const void *b ) {
	const builderWindowsSDKVersion_t *versionA = (const builderWindowsSDKVersion_t *) a;
	const builderWindowsSDKVersion_t *versionB = (const builderWindowsSDKVersion_t *) b;

	if ( versionA->v0 != versionB->v0 ) return versionB->v0 - versionA->v0;
	if ( versionA->v1 != versionB->v1 ) return versionB->v1 - versionA->v1;
	if ( versionA->v2 != versionB->v2 ) return versionB->v2 - versionA->v2;

	return versionB->v3 - versionA->v3;
}

static bool Builder_GetWindowsSDKInstall( builderWindowsSDKInstall_t *outSDK ) {
	BUILDER_ASSERT( outSDK );

	bool success = false;
	HKEY key = NULL;
	const char *windowsSDKRoot = NULL;
	builderWindowsSDKVersion_t *versions = NULL;
	uint32_t versionsCount = 0;
	bool found = false;

	const char *winSDKRegPath = "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots";
	LSTATUS status = RegOpenKeyExA( HKEY_LOCAL_MACHINE, winSDKRegPath, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &key );

	if ( status != ERROR_SUCCESS ) {
		Builder_Error(
			"Failed to get Windows SDK installation directory from your Windows registry.  The registry path \"%s\" doesn't seem to exist on your machine.\n"
			"This likely means you don't have the Windows SDK installed on your machine.\n"
			"In order to build using MSVC (which you asked me to do) then you will need to install a version of the Windows SDK on your PC.\n"
			, winSDKRegPath
		);

		goto cleanup;
	}

	const char *winSDKRegKey = "KitsRoot10";

	DWORD windowsSDKRootLength = 0;
	status = RegQueryValueExA( key, winSDKRegKey, NULL, NULL, NULL, &windowsSDKRootLength );

	if ( status == ERROR_SUCCESS ) {
		// valueStrLength from RegQueryValueExA includes the null terminator for REG_SZ strings
		char *windowsSDKRootStr = (char *) malloc( windowsSDKRootLength * sizeof( char ) );

		DWORD windowsSDKRootType = 0;

		status = RegQueryValueExA( key, winSDKRegKey, NULL, &windowsSDKRootType, (LPBYTE) windowsSDKRootStr, &windowsSDKRootLength );

		if ( status == ERROR_SUCCESS && windowsSDKRootType == REG_SZ ) {
			windowsSDKRoot = windowsSDKRootStr;
		} else {
			free( windowsSDKRootStr );
		}
	}

	if ( !windowsSDKRoot ) {
		Builder_Error(
			"Failed to get Windows SDK installation directory from your Windows registry.  The registry key \"%s\" couldn't be queried from the registry path: \"%s\"\n"
			"This likely means you don't have the Windows SDK installed on your machine.\n"
			"In order to build using MSVC (which you asked me to do) then you will need to install a version of the Windows SDK on your PC.\n"
			, winSDKRegKey
			, winSDKRegPath
		);

		goto cleanup;
	}

	{
		char *windowsSDKLibFolder = Builder_FormatString( "%sLib", windowsSDKRoot );

		builderFoundWindowsSDKVersionData_t foundData = {
			.versions		= versions,
			.versionsCount	= versionsCount,
		};

		bool visited = Builder_VisitFiles( windowsSDKLibFolder, BUILDER_FILE_VISIT_FOLDERS, OnWindowsSDKVersionFound, &foundData );

		free( windowsSDKLibFolder );

		versions = foundData.versions;
		versionsCount = foundData.versionsCount;

		if ( !visited ) {
			Builder_Error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have at least one version of the Windows SDK installed?\n" );
			goto cleanup;
		}
	}

	if ( versionsCount == 0 ) {
		Builder_Error( "Failed to find any versions of the Windows SDK installed under \"%s\".\n", windowsSDKRoot );
		goto cleanup;
	}

	// newest version first
	qsort( versions, versionsCount, sizeof( builderWindowsSDKVersion_t ), Builder_CompareWindowsSDKVersions );

	// find the first windows SDK folder that isnt malformed
	for ( uint32_t versionIndex = 0; versionIndex < versionsCount; versionIndex++ ) {
		builderWindowsSDKVersion_t *version = &versions[versionIndex];

		char *ucrtIncludeFolder = Builder_FormatString( "%sinclude\\%d.%d.%d.%d\\ucrt", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *umIncludeFolder = Builder_FormatString( "%sinclude\\%d.%d.%d.%d\\um", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *sharedIncludeFolder = Builder_FormatString( "%sinclude\\%d.%d.%d.%d\\shared", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *ucrtLibFolder = Builder_FormatString( "%sLib\\%d.%d.%d.%d\\ucrt\\x64", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *umLibFolder = Builder_FormatString( "%sLib\\%d.%d.%d.%d\\um\\x64", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );

		uint32_t missingFoldersCount = 0;
		const char *missingFolders[5] = {};

		if ( !Builder_FolderExists( ucrtIncludeFolder ) ) {
			missingFolders[missingFoldersCount++] = ucrtIncludeFolder;
		}

		if ( !Builder_FolderExists( umIncludeFolder ) ) {
			missingFolders[missingFoldersCount++] = umIncludeFolder;
		}

		if ( !Builder_FolderExists( sharedIncludeFolder ) ) {
			missingFolders[missingFoldersCount++] = sharedIncludeFolder;
		}

		if ( !Builder_FolderExists( ucrtLibFolder ) ) {
			missingFolders[missingFoldersCount++] = ucrtLibFolder;
		}

		if ( !Builder_FolderExists( umLibFolder ) ) {
			missingFolders[missingFoldersCount++] = umLibFolder;
		}

		if ( missingFoldersCount > 0 ) {
			stringBuilder_t sb = {};
			StringBuilder_Appendf( &sb, "Version %d.%d.%d.%d of your Windows SDK installation is malformed because the following folder(s) could not be found:\n", version->v0, version->v1, version->v2, version->v3 );

			for ( uint32_t missingFolderIndex = 0; missingFolderIndex < missingFoldersCount; missingFolderIndex++ ) {
				StringBuilder_Appendf( &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}

			StringBuilder_Appendf( &sb, "You must have the following folders in your Windows SDK install:\n"
				"    include/<version>/ucrt\n"
				"    include/<version>/um\n"
				"    include/<version>/shared\n"
				"    Lib/<version>/ucrt/x64\n"
				"    Lib/<version>/um/x64\n"
			);

			StringBuilder_Appendf( &sb, "If you want to use this version of the Windows SDK specifically, you will need to fix this yourself.\n" );

			char *message = StringBuilder_ToString( &sb );
			Builder_Warning( "%s", message );
			free( message );

			StringBuilder_Destroy( &sb );

			free( ucrtIncludeFolder );
			free( umIncludeFolder );
			free( sharedIncludeFolder );
			free( ucrtLibFolder );
			free( umLibFolder );

			continue;
		}

		outSDK->rootFolder			= windowsSDKRoot;
		outSDK->ucrtIncludePath		= ucrtIncludeFolder;
		outSDK->umIncludePath		= umIncludeFolder;
		outSDK->sharedIncludePath	= sharedIncludeFolder;
		outSDK->ucrtLibPath			= ucrtLibFolder;
		outSDK->umLibPath			= umLibFolder;
		outSDK->version				= *version;

		found = true;

		break;
	}

	if ( !found ) {
		Builder_Error(
			"Failed to find a valid installation of the Windows SDK on your machine.\n"
			"You have %u versions of the Windows SDK installed on your machine, and somehow all of them appear to be malformed.\n"
			"You need to install a version through the Visual Studio Installer, or via the separate Build Tools installer from Microsoft.\n"
			, versionsCount
		);

		goto cleanup;
	}

	printf( "Using latest valid Windows SDK version that was found, which was: %d.%d.%d.%d\n", outSDK->version.v0, outSDK->version.v1, outSDK->version.v2, outSDK->version.v3 );

	success = true;

cleanup:
	if ( key ) {
		RegCloseKey( key );
	}

	return success;
}

// MSVC toolset folders are named like "14.44.35207" - that's the only part of each entry we need to parse ourselves
static void Builder_OnMSVCInstallFound( fileInfo_t *fileInfo, void *data ) {
	builderFoundMSVCInstallData_t *foundData = (builderFoundMSVCInstallData_t *) data;

	builderMSVCVersion_t version = {};

	if ( sscanf( fileInfo->filename, "%d.%d.%d", &version.v0, &version.v1, &version.v2 ) != 3 ) {
		return;
	}

	builderMSVCInstall_t install = {
		.rootFolder		= Builder_FormatString( "%s", fileInfo->fullFilename ),
		.includePath	= Builder_FormatString( "%s\\include", fileInfo->fullFilename ),
		.libPath		= Builder_FormatString( "%s\\lib\\x64", fileInfo->fullFilename ),
		.version		= version,
	};

	foundData->installs = (builderMSVCInstall_t *) realloc( foundData->installs, ( ++foundData->installsCount ) * sizeof( builderMSVCInstall_t ) );
	foundData->installs[foundData->installsCount - 1] = install;
}

static bool Builder_MSVCNotInstalled( void ) {
	Builder_Error( "No valid MSVC installation found on your PC.  You need to install one through either the Visual Studio Installer or through the MS Build Tools.\n" );
	return false;
}

static int Builder_CompareMSVCInstallVersions( const void *a, const void *b ) {
	const builderMSVCInstall_t *installA = (const builderMSVCInstall_t *) a;
	const builderMSVCInstall_t *installB = (const builderMSVCInstall_t *) b;

	if ( installA->version.v0 != installB->version.v0 ) {
		return installB->version.v0 - installA->version.v0;
	}

	if ( installA->version.v1 != installB->version.v1 ) {
		return installB->version.v1 - installA->version.v1;
	}

	return installB->version.v2 - installA->version.v2;
}

// get all versions of MSVC
// thanks to Microsoft we will be doing that in the most retarded way possible
static bool Builder_GetMSVCInstall( builderMSVCInstall_t *outInstall ) {
	BUILDER_ASSERT( outInstall );

	bool success = false;

	// these are fixed, documented GUIDs for Microsoft.VisualStudio.Setup.Configuration.Native - not something we get to choose
	const GUID IID_ISetupConfiguration	= { 0x42843719, 0xDB4C, 0x46C2, { 0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B } };
	const GUID CLSID_SetupConfiguration	= { 0x177F0C4A, 0x1CD3, 0x4DE7, { 0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D } };

	HRESULT hr = S_OK;

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if ( FAILED( hr ) ) {
		Builder_Error( "CoInitializeEx() call failed: 0x%X\n", hr );
		return false;
	}

	ISetupConfiguration *setupConfig = NULL;

#ifdef __cplusplus
    hr = CoCreateInstance( CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, IID_ISetupConfiguration, (void **) &setupConfig );
#else
    hr = CoCreateInstance( &CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, &IID_ISetupConfiguration, (void **) &setupConfig );
#endif

	if ( hr == REGDB_E_CLASSNOTREG ) {
		success = Builder_MSVCNotInstalled();
		goto cleanup;
	}

	if ( FAILED( hr ) ) {
		Builder_Error( "CoCreateInstance() call failed: 0x%X\n", hr );
		goto cleanup;
	}

	IEnumSetupInstances *instances = NULL;
	ISetupInstance *instance = NULL;

	hr = setupConfig->vtable->EnumInstances( setupConfig, &instances );

	if ( FAILED( hr ) ) {
		Builder_Error( "setupConfig->EnumInstances() call failed: 0x%X\n", hr );
		goto cleanup;
	}

	if ( !instances ) {
		Builder_Error( "setupConfig->EnumInstances() returned no instances.  Bailing...\n" );
		goto cleanup;
	}

	ULONG foundInstance = 0;
	hr = instances->vtable->Next( instances, 1, &instance, &foundInstance );

	builderMSVCInstall_t *foundMSVCInstalls = NULL;
	uint32_t foundMSVCInstallsCount = 0;

	while ( foundInstance ) {
		BSTR visualStudioInstallationPathWide = NULL;
		hr = instance->vtable->GetInstallationPath( instance, &visualStudioInstallationPathWide );

		if ( FAILED( hr ) ) {
			Builder_Error( "instance->GetInstallationPath() call failed: 0x%X\n", hr );
			instance->vtable->Release( instance );
			goto cleanup;
		}

		char *visualStudioInstallationPath = NULL;

		{
			UINT wideLength = SysStringLen( visualStudioInstallationPathWide );

			int utf8Length = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, (int) wideLength, NULL, 0, NULL, NULL );

			if ( utf8Length <= 0 ) {
				Builder_Error( "First WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				SysFreeString( visualStudioInstallationPathWide );
				instance->vtable->Release( instance );
				goto cleanup;
			}

			visualStudioInstallationPath = (char *) malloc( ( (size_t) utf8Length + 1 ) * sizeof( char ) );

			int converted = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, (int) wideLength, visualStudioInstallationPath, utf8Length, NULL, NULL );

			if ( !converted ) {
				Builder_Error( "Second WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				SysFreeString( visualStudioInstallationPathWide );
				instance->vtable->Release( instance );
				goto cleanup;
			}

			visualStudioInstallationPath[utf8Length] = 0;
		}

		SysFreeString( visualStudioInstallationPathWide );

		char *msvcRootFolder = Builder_FormatString( "%s\\VC\\Tools\\MSVC", visualStudioInstallationPath );

		free( visualStudioInstallationPath );

		builderFoundMSVCInstallData_t foundData = {
			.installs		= foundMSVCInstalls,
			.installsCount	= foundMSVCInstallsCount,
		};

		if ( !Builder_VisitFiles( msvcRootFolder, BUILDER_FILE_VISIT_FOLDERS, Builder_OnMSVCInstallFound, &foundData ) ) {
			Builder_Error( "Failed to query for MSVC installation folders under \"%s\".\n", msvcRootFolder );
			free( msvcRootFolder );
			instance->vtable->Release( instance );
			goto cleanup;
		}

		foundMSVCInstalls = foundData.installs;
		foundMSVCInstallsCount = foundData.installsCount;

		free( msvcRootFolder );

		instance->vtable->Release( instance );

		hr = instances->vtable->Next( instances, 1, &instance, &foundInstance );
	}

	if ( foundMSVCInstallsCount == 0 ) {
		success = Builder_MSVCNotInstalled();
		goto cleanup;
	}

	// newest version first
	qsort( foundMSVCInstalls, foundMSVCInstallsCount, sizeof( builderMSVCInstall_t ), Builder_CompareMSVCInstallVersions );

	bool found = false;
	uint32_t useVersionIndex = 0;

	for ( uint32_t versionIndex = 0; versionIndex < foundMSVCInstallsCount; versionIndex++ ) {
		builderMSVCInstall_t *install = &foundMSVCInstalls[versionIndex];

		uint32_t missingFoldersCount = 0;
		const char *missingFolders[2] = {};

		if ( !Builder_FolderExists( install->includePath ) ) {
			missingFolders[missingFoldersCount++] = install->includePath;
		}

		if ( !Builder_FolderExists( install->libPath ) ) {
			missingFolders[missingFoldersCount++] = install->libPath;
		}

		if ( missingFoldersCount > 0 ) {
			stringBuilder_t sb = {};
			StringBuilder_Appendf( &sb, "Version %d.%d.%d of your MSVC installation is malformed because the following folder(s) could not be found:\n", install->version.v0, install->version.v1, install->version.v2 );

			for ( uint32_t missingFolderIndex = 0; missingFolderIndex < missingFoldersCount; missingFolderIndex++ ) {
				StringBuilder_Appendf( &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}

			StringBuilder_Appendf( &sb, "You must have the following folders in your MSVC install:\n"
				"    %s\n"
				"    %s\n"
				, install->includePath
				, install->libPath
			);

			StringBuilder_Appendf( &sb, "If you want to use this version of MSVC specifically, you will need to fix this yourself.\n" );

			char *message = StringBuilder_ToString( &sb );
			Builder_Warning( "%s", message );
			free( message );

			StringBuilder_Destroy( &sb );

			continue;
		}

		useVersionIndex = versionIndex;
		found = true;

		break;
	}

	if ( !found ) {
		success = Builder_MSVCNotInstalled();
		goto cleanup;
	}

	*outInstall = foundMSVCInstalls[useVersionIndex];

	printf( "Using latest valid MSVC version that was found, which was: %d.%d.%d\n", outInstall->version.v0, outInstall->version.v1, outInstall->version.v2 );

	success = true;

cleanup:
	if ( instances ) {
		instances->vtable->Release( instances );
	}

	if ( setupConfig ) {
		setupConfig->vtable->Release( setupConfig );
	}

	CoUninitialize();

	return success;
}
#endif // _WIN32

static const char *GetLanguageVersionString( const LanguageVersion version ) {
	switch ( version ) {
		case LANGUAGE_VERSION_UNSET:	return NULL;
		case LANGUAGE_VERSION_C89:		return "c89";
		case LANGUAGE_VERSION_C99:		return "c99";
		case LANGUAGE_VERSION_C11:		return "c11";
		case LANGUAGE_VERSION_C17:		return "c17";
		case LANGUAGE_VERSION_C23:		return "c23";
		case LANGUAGE_VERSION_CPP11:	return "c++11";
		case LANGUAGE_VERSION_CPP14:	return "c++14";
		case LANGUAGE_VERSION_CPP17:	return "c++17";
		case LANGUAGE_VERSION_CPP20:	return "c++20";
		case LANGUAGE_VERSION_CPP23:	return "c++23";
	}

	BUILDER_ASSERT( "Unrecognised language version specified!\n" );

	return NULL;
}

static const char *Builder_GetOptimizationString_Clang( const Optimization optimization ) {
	switch ( optimization ) {
		case OPTIMIZATION_DISABLED:			return "-O0";
		case OPTIMIZATION_PROGRAM_SIZE:		return "-O2";
		case OPTIMIZATION_PROGRAM_SPEED:	return "-O3";
	}

	BUILDER_ASSERT( "Unrecognised optimization mode specified!\n" );

	return NULL;
}

static const char *Builder_GetOptimizationString_MSVC( const Optimization optimization ) {
	switch ( optimization ) {
		case OPTIMIZATION_DISABLED:			return "/Od";
		case OPTIMIZATION_PROGRAM_SIZE:		return "/O1";
		case OPTIMIZATION_PROGRAM_SPEED:	return "/O2";
	}

	BUILDER_ASSERT( "Unrecognised optimization mode specified!\n" );

	return NULL;
}

static char *Builder_ExtractVersionNumber( const char *text ) {
	for ( const char *c = text; *c; c++ ) {
		if ( !isdigit( (unsigned char) *c ) ) {
			continue;
		}

		const char *start = c;
		const char *end = c;
		bool sawDot = false;

		while ( *end && ( isdigit( (unsigned char) *end ) || *end == '.' ) ) {
			if ( *end == '.' ) {
				sawDot = true;
			}

			end++;
		}

		while ( end > start && *( end - 1 ) == '.' ) {
			end--;
		}

		if ( sawDot ) {
			return Builder_FormatString( "%.*s", (int) ( end - start ), start );
		}

		c = end - 1;
	}

	return NULL;
}

static double Builder_TimeMS( void ) {
#if defined( _WIN32 )
	static LARGE_INTEGER frequency = { 0 };
	static bool haveFrequency = false;

	if ( !haveFrequency ) {
		QueryPerformanceFrequency( &frequency );
		haveFrequency = true;
	}

	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );

	return ( (double) counter.QuadPart * 1000.0 ) / (double) frequency.QuadPart;
#elif defined( __linux__ )
	struct timespec ts;
	if ( clock_gettime( CLOCK_MONOTONIC, &ts ) == -1 ) {
		int err = errno;
		Builder_Error( "Failed to get time: %s\n", strerror( err ) );
		return 0.0;
	}

	return ( (double) ts.tv_sec * 1000.0 ) + ( (double) ts.tv_nsec / 1000000.0 );
#endif
}

static uint32_t Builder_GetNumCPUCores( void ) {
#if defined( _WIN32 )
	SYSTEM_INFO sysInfo;
	GetSystemInfo( &sysInfo );

	return (uint32_t) sysInfo.dwNumberOfProcessors;
#elif defined( __linux__ )
	long numCores = sysconf( _SC_NPROCESSORS_ONLN );
	int err = errno;

	if ( numCores < 0 ) {
		Builder_Error( "Failed to query CPU for number of available cores: %s\n", strerror( err ) );
		return 1;
	}

	return (uint32_t) numCores;
#endif
}

#if defined( _WIN32 )
typedef volatile LONG	builderAtomic32_t;
typedef HANDLE			builderThread_t;
#elif defined( __linux__ )
typedef volatile int32_t	builderAtomic32_t;
typedef pthread_t			builderThread_t;
#endif

static uint32_t Builder_AtomicIncrement( builderAtomic32_t *value ) {
#if defined( _WIN32 )
	return (uint32_t) InterlockedIncrement( value );
#elif defined( __linux__ )
	return (uint32_t) __sync_add_and_fetch( value, 1 );
#endif
}

typedef struct builderCompileContext_t {
	BuildConfig	*config;
	const char	*compilerPath;
	const char	*intermediateFolder;
	bool		useMSVC;
#if defined( _WIN32 )
	builderMSVCInstall_t		*msvcInstall;
	builderWindowsSDKInstall_t	*windowsSDKInstall;
#endif
} builderCompileContext_t;

static bool Builder_CompileSourceFile( const builderCompileContext_t *context, const char *sourceFile ) {
	BuildConfig *config = context->config;

	stringBuilder_t compileArgs = {};
	StringBuilder_Appendf( &compileArgs, "\"%s\" ", context->compilerPath );

	if ( context->useMSVC ) {
#if defined( _WIN32 )
		builderMSVCInstall_t *msvcInstall = context->msvcInstall;
		builderWindowsSDKInstall_t *windowsSDKInstall = context->windowsSDKInstall;

		StringBuilder_Appendf( &compileArgs, "/nologo " );	// disable MSVC spamming its copyright banner for every compilation unit
		StringBuilder_Appendf( &compileArgs, "/c " );

		if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
			StringBuilder_Appendf( &compileArgs, "/std:%s ", GetLanguageVersionString( config->languageVersion ) );
		}

		if ( !config->removeSymbols ) {
			StringBuilder_Appendf( &compileArgs, "/Z7 " );
		}

		StringBuilder_Appendf( &compileArgs, "%s ", Builder_GetOptimizationString_MSVC( config->optimization ) );

		StringBuilder_Appendf( &compileArgs, "/Fo" );
		Builder_AppendIntermediateFilePath( &compileArgs, context->intermediateFolder, sourceFile );
		StringBuilder_Appendf( &compileArgs, "%s ", sourceFile );

		const char **define = config->defines;
		while ( define && *define ) {
			StringBuilder_Appendf( &compileArgs, "/D%s ", *define );

			define++;
		}

		// cl.exe doesn't know where the CRT/Windows SDK headers live unless you're in a Developer Command Prompt, so point it there ourselves
		StringBuilder_Appendf( &compileArgs, "/I\"%s\" /I\"%s\" /I\"%s\" /I\"%s\" "
			, msvcInstall->includePath
			, windowsSDKInstall->ucrtIncludePath
			, windowsSDKInstall->umIncludePath
			, windowsSDKInstall->sharedIncludePath );

		const char **additionalInclude = config->additionalIncludes;
		while ( additionalInclude && *additionalInclude ) {
			StringBuilder_Appendf( &compileArgs, "/I%s ", *additionalInclude );

			additionalInclude++;
		}

		if ( config->warningsAsErrors ) {
			StringBuilder_Appendf( &compileArgs, "/WX " );
		}

		bool sawWarningLevel = false;
		const char **warningLevel = config->warningLevels;
		while ( warningLevel && *warningLevel ) {
			if ( sawWarningLevel ) {
				Builder_Error( "MSVC only allows one warning level to be set at a time, but you specified more than one.\n" );
				StringBuilder_Destroy( &compileArgs );
				return false;
			}

			if ( !Builder_IsWarningLevelAllowed_MSVC( *warningLevel ) ) {
				Builder_Error(
					"Warning level \"%s\" is not a valid one.  Allowed warning levels are:\n"
					"    /W0\n"
					"    /W1\n"
					"    /W2\n"
					"    /W3\n"
					"    /W4\n"
					"    /Wall\n"
					, *warningLevel
				);

				StringBuilder_Destroy( &compileArgs );
				return false;
			}

			StringBuilder_Appendf( &compileArgs, "%s ", *warningLevel );

			sawWarningLevel = true;
			warningLevel++;
		}
#endif
	} else {
		if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
			StringBuilder_Appendf( &compileArgs, "-std=%s ", GetLanguageVersionString( config->languageVersion ) );
		}

		if ( !config->removeSymbols ) {
			StringBuilder_Appendf( &compileArgs, "-g " );
		}

		StringBuilder_Appendf( &compileArgs, "%s ", Builder_GetOptimizationString_Clang( config->optimization ) );

		StringBuilder_Appendf( &compileArgs, "-c " );
		StringBuilder_Appendf( &compileArgs, "-o " );
		Builder_AppendIntermediateFilePath( &compileArgs, context->intermediateFolder, sourceFile );
		StringBuilder_Appendf( &compileArgs, "%s ", sourceFile );

		const char **define = config->defines;
		while ( define && *define ) {
			StringBuilder_Appendf( &compileArgs, "-D%s ", *define );

			define++;
		}

		const char **additionalInclude = config->additionalIncludes;
		while ( additionalInclude && *additionalInclude ) {
			StringBuilder_Appendf( &compileArgs, "-I%s ", *additionalInclude );

			additionalInclude++;
		}

		if ( config->warningsAsErrors ) {
			StringBuilder_Appendf( &compileArgs, "-Werror " );
		}

		const char **warningLevel = config->warningLevels;
		while ( warningLevel && *warningLevel ) {
			if ( !Builder_IsWarningLevelAllowed_Clang( *warningLevel ) ) {
				Builder_Error(
					"Warning level \"%s\" is not a valid one.  Allowed warning levels are:\n"
					"    -Wall\n"
					"    -Weverything\n"
					"    -Wextra\n"
					"    -Wpedantic\n"
					, *warningLevel
				);

				StringBuilder_Destroy( &compileArgs );
				return false;
			}

			StringBuilder_Appendf( &compileArgs, "%s ", *warningLevel );

			warningLevel++;
		}
	}

	const char **ignoreWarning = config->ignoreWarnings;
	while ( ignoreWarning && *ignoreWarning ) {
		StringBuilder_Appendf( &compileArgs, "%s ", *ignoreWarning );

		ignoreWarning++;
	}

	char *args = StringBuilder_ToString( &compileArgs );

	StringBuilder_Destroy( &compileArgs );

	printf( "%s\n", args );

	int32_t compileResult = Builder_RunProcess( args, NULL );

	free( args );

	return compileResult == 0;
}

typedef struct builderCompileJobPool_t {
	const builderCompileContext_t	*context;
	const char						**sourceFiles;
	uint32_t						threadIndex;
	uint32_t						sourceFilesCount;
	builderAtomic32_t				nextSourceFileIndex;
	builderAtomic32_t				numFailed;
} builderCompileJobPool_t;

static void Builder_RunCompileJobPool( builderCompileJobPool_t *pool ) {
	while ( 1 ) {
		uint32_t sourceFileIndex = Builder_AtomicIncrement( &pool->nextSourceFileIndex ) - 1;

		if ( sourceFileIndex >= pool->sourceFilesCount ) {
			break;
		}

		if ( !Builder_CompileSourceFile( pool->context, pool->sourceFiles[sourceFileIndex] ) ) {
			Builder_AtomicIncrement( &pool->numFailed );
		}
	}
}

#if defined( _WIN32 )
static DWORD WINAPI Builder_CompileJobThreadProc( LPVOID param ) {
	Builder_RunCompileJobPool( (builderCompileJobPool_t *) param );
	return 0;
}
#elif defined( __linux__ )
static void *Builder_CompileJobThreadProc( void *param ) {
	Builder_RunCompileJobPool( (builderCompileJobPool_t *) param );
	return NULL;
}
#endif

static bool Builder_CreateCompileJobThread( builderCompileJobPool_t *pool, builderThread_t *outThread ) {
#if defined( _WIN32 )
	HANDLE thread = CreateThread( NULL, 0, Builder_CompileJobThreadProc, pool, 0, NULL );

	if ( !thread ) {
		Builder_Error( "Failed to create compile worker thread: 0x%X\n", GetLastError() );
		return false;
	}

	*outThread = thread;

	return true;
#elif defined( __linux__ )
	pthread_t thread;

	int result = pthread_create( &thread, NULL, Builder_CompileJobThreadProc, pool );

	if ( result != 0 ) {
		Builder_Error( "Failed to create compile worker thread: %s\n", strerror( result ) );
		return false;
	}

	*outThread = thread;

	return true;
#endif
}

static void Builder_ThreadJoin( builderThread_t thread ) {
#if defined( _WIN32 )
	WaitForSingleObject( thread, INFINITE );
	CloseHandle( thread );
#elif defined( __linux__ )
	if ( pthread_join( thread, NULL ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to join thread: %s\n", strerror( err ) );
	}
#endif
}

int Build( BuilderOptions *options ) {
	double totalTimeStart = Builder_TimeMS();

	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	for ( int argIndex = 0; argIndex < options->argc; argIndex++ ) {
		if ( Builder_StringStartsWith( options->argv[argIndex], ARG_HELP_SHORT ) || Builder_StringStartsWith( options->argv[argIndex], ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}
	}

	// validate cmd line args
	const char *nameOfConfigToBuild = Builder_GetNameOfConfigToBuild( options );
	BuildConfig *targetConfig = NULL;
	{
		if ( options->configsCount == 0 ) {
			Builder_Error( "No BuildConfig was registered.  You must call AddBuildConfig() at least once.\n" );
			return 1;
		} else if ( options->configsCount > 1 && !nameOfConfigToBuild ) {
			Builder_Error( "You have more than 1 BuildConfig defined, but you never told me which you wanted me to build via \"" ARG_CONFIG "\".  You need to tell me what config you want me to build, or set a default via BuilderOptions::defaultConfig.\n" );
			return 1;
		}

		if ( nameOfConfigToBuild ) {
			for ( uint32_t configIndex = 0; configIndex < options->configsCount; configIndex++ ) {
				if ( options->configs[configIndex].name && Builder_StringEquals( options->configs[configIndex].name, nameOfConfigToBuild ) ) {
					targetConfig = &options->configs[configIndex];
					break;
				}
			}

			if ( !targetConfig ) {
				Builder_Error( "No BuildConfig found with the name \"%s\".\n", nameOfConfigToBuild );
				return 1;
			}
		} else {
			targetConfig = &options->configs[0];
		}
	}

	// only query for windows SDK and MSVC installations after verifying cmd line args and
#ifdef _WIN32
	builderWindowsSDKInstall_t windowsSDKInstall = {};
	if ( !Builder_GetWindowsSDKInstall( &windowsSDKInstall ) ) {
		return 1;
	}

	builderMSVCInstall_t msvcInstall = {};
	if ( !Builder_GetMSVCInstall( &msvcInstall ) ) {
		return 1;
	}
#endif

	const char *compilerPath = ( options->compilerPath && options->compilerPath[0] ) ? options->compilerPath : "clang";

#if defined( _WIN32 )
	bool useMSVC = Builder_StringEquals( compilerPath, "cl" ) || Builder_StringEquals( compilerPath, "cl.exe" );

	if ( useMSVC ) {
		compilerPath = Builder_FormatString( "%s\\bin\\Hostx64\\x64\\cl.exe", msvcInstall.rootFolder );
	}
#else
	bool useMSVC = false;
#endif

	if ( options->compilerVersion && options->compilerVersion[0] ) {
		if ( useMSVC ) {
#if defined( _WIN32 )
			char *actualVersion = Builder_FormatString( "%d.%d.%d", msvcInstall.version.v0, msvcInstall.version.v1, msvcInstall.version.v2 );

			if ( !Builder_StringEquals( actualVersion, options->compilerVersion ) ) {
				Builder_Warning( "You are using compiler version \"%s\", but \"%s\" was set as BuilderOptions::compilerVersion.  I will continue building anyway, but you may not get what you expect.\n", actualVersion, options->compilerVersion );
			}

			free( actualVersion );
#endif
		} else {
			char *versionCmd = Builder_FormatString( "\"%s\" --version", compilerPath );
			char *versionOutput = NULL;

			Builder_RunProcess( versionCmd, &versionOutput );

			if ( !Builder_StringContains( versionOutput, options->compilerVersion ) ) {
				char *actualVersion = Builder_ExtractVersionNumber( versionOutput );

				Builder_Warning( "You are using compiler version \"%s\", but \"%s\" was set as BuilderOptions::compilerVersion.  I will continue building anyway, but you may not get what you expect.\n", actualVersion, options->compilerVersion );

				free( actualVersion );
			}

			free( versionCmd );
			free( versionOutput );
		}
	}

	// TODO: DM: 10/08/2026: either do proper cleanup ourselves via goto to account for all cases
	// or wait for Tom's linear allocator
	uint32_t configsToBuildCount = 0;
	BuildConfig *configsToBuild = NULL;
	AddBuildConfigInternal( options, targetConfig, NULL, &configsToBuild, &configsToBuildCount );

	double totalCompileTimeMS = 0.0;
	double totalLinkTimeMS = 0.0;

	for ( uint32_t configIndex = 0; configIndex < configsToBuildCount; configIndex++ ) {
		BuildConfig *config = &configsToBuild[configIndex];

		double compileTimeMS = 0.0;
		double linkTimeMS = 0.0;

		if ( config->OnPreBuild ) {
			config->OnPreBuild( config );
		}

		// build the config
		{
			printf( "Building config \"%s\":\n", config->name ? config->name : "" );

			const char *intermediateFolder = config->intermediateFolder;

			if ( intermediateFolder && config->binaryFolder ) {
				intermediateFolder = Builder_FormatString( "%s%c%s", config->binaryFolder, BUILDER_PATH_SEPARATOR, intermediateFolder );
			} else if ( !intermediateFolder ) {
				intermediateFolder = config->binaryFolder;
			}

			if ( intermediateFolder && !Builder_CreateFolderIfItDoesntExist( intermediateFolder ) ) {
				Builder_Error( "Failed to create the intermediate folder \"%s\".\n", intermediateFolder );
				return 1;
			}

			// glob step
			builderFileGlobResult_t globResult = Builder_GlobFiles( config->sourceFiles );

			// compilation step
			{
				const double compileTimeStart = Builder_TimeMS();

				if ( globResult.count > 0 ) {
					// TODO: DM: 09/08/2026: is it OK to create and destroy a bunch of threads for each config?
					builderCompileContext_t compileContext = {
						.config				= config,
						.compilerPath		= compilerPath,
						.intermediateFolder	= intermediateFolder,
						.useMSVC			= useMSVC,
#if defined( _WIN32 )
						.msvcInstall		= &msvcInstall,
						.windowsSDKInstall	= &windowsSDKInstall,
#endif
					};

					builderCompileJobPool_t pool = {
						.context			= &compileContext,
						.sourceFiles		= globResult.files,
						.sourceFilesCount	= globResult.count,
					};

					// only spin up additional threads once theres more than one file
					// limit the number of threads we spin up to no higher than the number of CPU cores we have
					uint32_t numCPUCores = Builder_GetNumCPUCores();
					uint32_t numWorkers = ( numCPUCores < globResult.count ) ? numCPUCores : globResult.count;
					uint32_t numAdditionalThreads = ( numWorkers > 1 ) ? numWorkers - 1 : 0;

					printf( "Compiling %u files across %u threads.\n", globResult.count, numWorkers );

					builderThread_t *additionalThreads = NULL;
					uint32_t numCreatedThreads = 0;

					if ( numAdditionalThreads > 0 ) {
						additionalThreads = (builderThread_t *) malloc( numAdditionalThreads * sizeof( builderThread_t ) );

						for ( uint32_t threadIndex = 0; threadIndex < numAdditionalThreads; threadIndex++ ) {
							if ( Builder_CreateCompileJobThread( &pool, &additionalThreads[numCreatedThreads] ) ) {
								numCreatedThreads++;
							}
						}
					}

					// the main thread pulls jobs from the same pool instead of just sitting idle waiting on the additional threads
					Builder_RunCompileJobPool( &pool );

					for ( uint32_t threadIndex = 0; threadIndex < numCreatedThreads; threadIndex++ ) {
						Builder_ThreadJoin( additionalThreads[threadIndex] );
					}

					if ( additionalThreads ) {
						free( additionalThreads );
					}

					if ( pool.numFailed > 0 ) {
						Builder_Error( "Build failed.\n" );
						return 1;
					}
				}

				compileTimeMS = Builder_TimeMS() - compileTimeStart;
			}

			// link step
			{
				double linkTimeStart = Builder_TimeMS();

				if ( config->binaryFolder && !Builder_CreateFolderIfItDoesntExist( config->binaryFolder ) ) {
					Builder_Error( "Failed to create the binary folder \"%s\".\n", config->binaryFolder );
					return 1;
				}

				stringBuilder_t linkerArgs = {};
#if defined( _WIN32 )
				if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
					StringBuilder_Appendf( &linkerArgs, "\"%s\\bin\\Hostx64\\x64\\lib.exe\" ", msvcInstall.rootFolder );
				} else {
					StringBuilder_Appendf( &linkerArgs, "\"%s\\bin\\Hostx64\\x64\\link.exe\" ", msvcInstall.rootFolder );
				}

				if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
					StringBuilder_Appendf( &linkerArgs, "/DLL " );
				}

				StringBuilder_Appendf( &linkerArgs, "/OUT:" );
				Builder_AppendBinaryPath( &linkerArgs, config );

				StringBuilder_Appendf( &linkerArgs, "/LIBPATH:\"%s\" ", msvcInstall.libPath );
				StringBuilder_Appendf( &linkerArgs, "/LIBPATH:\"%s\" ", windowsSDKInstall.umLibPath );
				StringBuilder_Appendf( &linkerArgs, "/LIBPATH:\"%s\" ", windowsSDKInstall.ucrtLibPath );

				for ( uint32_t fileIndex = 0; fileIndex < globResult.count; ++fileIndex ) {
					Builder_AppendIntermediateFilePath( &linkerArgs, intermediateFolder, globResult.files[fileIndex] );
				}

				const char **additionalLibPath = config->additionalLibPaths;
				while ( additionalLibPath && *additionalLibPath ) {
					StringBuilder_Appendf( &linkerArgs, "/LIBPATH:\"%s\" ", *additionalLibPath );

					additionalLibPath++;
				}

				const char **additionalLib = config->additionalLibs;
				while ( additionalLib && *additionalLib ) {
					StringBuilder_Appendf( &linkerArgs, "%s.lib ", *additionalLib );

					additionalLib++;
				}

				// TODO: AK: 11/08/2026: handle this better
				bool debugDefineSet = false;
				const char **define = config->defines;
				while ( define && *define ) {
					if ( !debugDefineSet && _strnicmp( "_DEBUG", *(define++), sizeof("_DEBUG") ) == 0 ) {
						debugDefineSet = true;
						break;
					}
				}
				
				if ( config->binaryType != BINARY_TYPE_STATIC_LIBRARY ) {
					// clang doesn't embed /DEFAULTLIB directives the way cl.exe does
					// so link.exe has no idea which CRT/SDK libs to pull in unless we name them ourselves
					// TODO: AK: 11/08/2026: we need dynamic runtime support too
					if (debugDefineSet) {
						StringBuilder_Appendf( &linkerArgs, "libcmtd.lib libcpmtd.lib libvcruntimed.lib libucrtd.lib kernel32.lib " );
					} else {
						StringBuilder_Appendf( &linkerArgs, "libcmt.lib libcpmt.lib libvcruntime.lib libucrt.lib kernel32.lib " );
					}
				}

				const char **additionalLinkerArgument = config->additionalLinkerArguments;
				while ( additionalLinkerArgument && *additionalLinkerArgument ) {
					StringBuilder_Appendf( &linkerArgs, "%s ", *additionalLinkerArgument );

					additionalLinkerArgument++;
				}
#elif defined( __linux__ )
				if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
					StringBuilder_Appendf( &linkerArgs, "ar rcs " );
					Builder_AppendBinaryPath( &linkerArgs, config );

					for ( uint32_t fileIndex = 0; fileIndex < globResult.count; ++fileIndex ) {
						Builder_AppendIntermediateFilePath( &linkerArgs, intermediateFolder, globResult.files[fileIndex] );
					}
				} else {
					StringBuilder_Appendf( &linkerArgs, "\"%s\" ", compilerPath );

					if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
						StringBuilder_Appendf( &linkerArgs, "-shared " );
					}

					StringBuilder_Appendf( &linkerArgs, "-o " );
					Builder_AppendBinaryPath( &linkerArgs, config );

					for ( uint32_t fileIndex = 0; fileIndex < globResult.count; ++fileIndex ) {
						Builder_AppendIntermediateFilePath( &linkerArgs, intermediateFolder, globResult.files[fileIndex] );
					}

					const char **additionalLibPath = config->additionalLibPaths;
					while ( additionalLibPath && *additionalLibPath ) {
						StringBuilder_Appendf( &linkerArgs, "-L%s ", *additionalLibPath );

						additionalLibPath++;
					}

					const char **additionalLib = config->additionalLibs;
					while ( additionalLib && *additionalLib ) {
						StringBuilder_Appendf( &linkerArgs, "-l%s ", *additionalLib );

						additionalLib++;
					}

					const char **additionalLinkerArgument = config->additionalLinkerArguments;
					while ( additionalLinkerArgument && *additionalLinkerArgument ) {
						StringBuilder_Appendf( &linkerArgs, "%s ", *additionalLinkerArgument );

						additionalLinkerArgument++;
					}
				}
#endif

				char *args = StringBuilder_ToString( &linkerArgs );

				StringBuilder_Destroy( &linkerArgs );

				printf( "%s\n", args );

				int32_t linkResult = Builder_RunProcess( args, NULL );

				free( args );

				if ( linkResult != 0 ) {
					Builder_Error( "Link failed.\n" );
					return 1;
				}

				linkTimeMS = Builder_TimeMS() - linkTimeStart;
			}
		}

		if ( config->OnPostBuild ) {
			config->OnPostBuild( config );
		}

		printf( "Finished config \"%s\":\n", config->name ? config->name : "" );
		printf( "    Compile : %f ms\n", compileTimeMS );
		printf( "    Link    : %f ms\n", linkTimeMS );
		printf( "\n" );

		totalCompileTimeMS += compileTimeMS;
		totalLinkTimeMS += linkTimeMS;
	}

	// build summary
	{
		printf( "Finished:\n" );
		printf( "    Compile : %f ms\n", totalCompileTimeMS );
		printf( "    Link    : %f ms\n", totalLinkTimeMS );
		printf( "    Total   : %f ms\n", Builder_TimeMS() - totalTimeStart );
	}

	return 0;
}

#endif // BUILDER_IMPLEMENTATION

#pragma clang diagnostic pop

#ifdef __cplusplus
}
#endif