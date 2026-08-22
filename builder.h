/*
===========================================================================

Builder

Distributed under MIT License:
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
#include <inttypes.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

// up here rather than with the rest of the implementation defines because the Add*() macros below expand to it
#ifndef BUILDER_ASSERT
#include <assert.h>
#define BUILDER_ASSERT assert
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

typedef struct StringList {
	struct builderStringChunk_t	*head;
	struct builderStringChunk_t	*tail;
	uint32_t					count;
} StringList;

typedef struct ConfigPtrList {
	struct buildConfigPtrChunk_t	*head;
	struct buildConfigPtrChunk_t	*tail;
	uint32_t						count;
} ConfigPtrList;

// Builder owns every BuildConfig - you get a blank one from CreateBuildConfig() and fill it in, either by assigning
// the whole struct or a field at a time.  Write to these fields directly; the list fields are the only ones that need
// building first, which is what MakeStringList() and MakeDependencies() are for.
// A config keeps the pointers you give it rather than copying them, so anything not a string literal wants to come
// from Config_FormatString().
typedef struct BuildConfig {
	// Required, and unique across your configs.  It's what "--config=" matches against and what the build log calls it.
	const char				*name;
	// Other BuildConfigs that need to be built before this one - see MakeDependencies() and AddDependencies().
	// Building a config builds everything in here first, so you only ever have to ask for the top-level one.
	ConfigPtrList			dependsOn;
	const char				*binaryName;
	// The folder the binary is placed into, relative to the file you pass into Builder.
	// If this folder doesn't exist then Builder will create it for you.
	// Leave unset to put the binary alongside the source file.
	const char				*binaryFolder;
	StringList		sourceFiles;
	StringList		defines;
	StringList		additionalIncludes;
	StringList		additionalCompilerArguments;
	StringList		additionalLibPaths;
	StringList		additionalLibs;
	StringList		warningLevels;
	StringList		ignoreWarnings;
	StringList		additionalLinkerArguments;
	BinaryType				binaryType;
	LanguageVersion			languageVersion;
	Optimization			optimization;
	bool					removeSymbols;
	bool					warningsAsErrors;
	// When doing incremental builds should we exclude checking system headers
	// this passes -M instead of -MM, and does not do anything on MSVC
	bool				excludeSystemHeaderDependencyChecking;
	bool				useDynamicRuntimeOnWindows;
	void					( *OnPreBuild )( struct BuildConfig *config );
	void					( *OnPostBuild )( struct BuildConfig *config );
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

	// The folder that intermediate build files (object files) are placed into, relative to build executable.
	// If this folder doesn't exist then Builder will create it for you.
	// Leave NULL to put intermediate files at ./intermediates/
	const char	*intermediateFolder;

	// If no config is specified at the command line via --config=, what config do you want Builder to build by default?
	BuildConfig	*defaultConfig;

	// Set this to true if you want Builder to force-rebuild your program.
	// All binaries and intermediate files will get rebuilt.
	// This is really only useful to those who are either using an editor + command line workflow, or just hate incremental builds.
	bool						forceRebuild;

	// Enables extra diagnostic logging throughout the build (each line prefixed "VERBOSE: ").
	// You can set this yourself, or leave it and Builder will set it automatically if "-v" or "--verbose" is present in argv.
	bool		verboseLogging;

	// The list of configs that gets populated when calling CreateBuildConfig().
	// Don't write to this directly unless you know what you're doing.
	ConfigPtrList	configs;
} BuilderOptions;

// Creates a zeroed BuildConfig, registers it with options, and hands it back.  Builder owns it and keeps it alive
// until the program exits, so nothing user-owned is ever pointed at.
// Fill it in however you like - assign the whole struct at once, or use the Add*()/Set*() functions below:
//
//	BuildConfig *program = CreateBuildConfig( options );
//	*program = (BuildConfig) {
//		.name        = "program",
//		.binaryType  = BINARY_TYPE_EXE,
//		.sourceFiles = MakeStringList( "main.c", "util.c" ),
//		.dependsOn   = MakeDependencies( lib ),
//	};
//
// Every config needs a name - it's what "--config=" matches against and what the build log calls it - and no two may
// share one.  Build() checks both before it builds anything.
BuildConfig	*CreateBuildConfig( BuilderOptions *options );

// Bundles the arguments of the macros below into an array and its length, so a call site never has to write a count or
// a terminator.  sizeof doesn't evaluate its operand, so naming __VA_ARGS__ twice costs nothing at runtime.
#define BUILDER_STRING_ARGS( ... )	(const char *[]) { __VA_ARGS__ },	(uint32_t) ( sizeof( (const char *[]) { __VA_ARGS__ } ) / sizeof( const char * ) )
#define BUILDER_CONFIG_ARGS( ... )	(BuildConfig *[]) { __VA_ARGS__ },	(uint32_t) ( sizeof( (BuildConfig *[]) { __VA_ARGS__ } ) / sizeof( BuildConfig * ) )

// Build a whole list in one expression, for assigning straight into a BuildConfig.  What they hand back is the list
// wrapper by value - everything it points at lives in Builder's memory, so it survives being copied about and outlasts
// the strings you built it from.
#define MakeStringList( ... )	Builder_MakeStringListInternal( BUILDER_STRING_ARGS( __VA_ARGS__ ) )
#define MakeDependencies( ... )	Builder_MakeDependenciesInternal( BUILDER_CONFIG_ARGS( __VA_ARGS__ ) )

// Builds a string in Builder's memory, so it's still alive when the build runs.
// A BuildConfig keeps whatever pointers you give it rather than copying them, which is free for the string literals
// that make up almost every build script.  Use this for anything you need to build at runtime - a version number, a
// path assembled from parts - rather than pointing a config at a local buffer that's about to go out of scope.
const char	*Config_FormatString( const char *fmt, ... );

// DO NOT CALL THESE DIRECTLY
// CALL THE MACRO VERSIONS ABOVE INSTEAD
StringList		Builder_MakeStringListInternal( const char **strings, uint32_t count );
ConfigPtrList	Builder_MakeDependenciesInternal( BuildConfig **dependencies, uint32_t count );

// Append to a list that's already on a config, for layering settings on after it's been filled in.  Additive: call them
// as often as you like and the entries accumulate.
#define AddSourceFiles( config, ... )		( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->sourceFiles, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddDefines( config, ... )			( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->defines, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )				// no "-D"/"/D" - Builder adds that for you
#define AddIncludes( config, ... )			( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->additionalIncludes, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddCompilerArguments( config, ... )	( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->additionalCompilerArguments, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddLibPaths( config, ... )			( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->additionalLibPaths, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddLibs( config, ... )				( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->additionalLibs, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddWarningLevels( config, ... )		( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->warningLevels, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddIgnoreWarnings( config, ... )	( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->ignoreWarnings, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )
#define AddLinkerArguments( config, ... )	( BUILDER_ASSERT( config ), Builder_AddStringsInternal( &( config )->additionalLinkerArguments, BUILDER_STRING_ARGS( __VA_ARGS__ ) ) )

// Every dependency gets built before config does.  Builder keeps the pointers, so they need to still be alive when
// Build() runs - a config from CreateBuildConfig() always is.
#define AddDependencies( config, ... )		Builder_AddDependenciesInternal( ( config ), BUILDER_CONFIG_ARGS( __VA_ARGS__ ) )

// DO NOT CALL THESE DIRECTLY
// CALL THE MACRO VERSIONS ABOVE INSTEAD
void	Builder_AddStringsInternal( StringList *list, const char **strings, uint32_t count );
void	Builder_AddDependenciesInternal( BuildConfig *config, BuildConfig **dependencies, uint32_t count );


int		Build( BuilderOptions *options, int argc, char **argv );


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

#if defined( _WIN32 )
#define BUILDER_PATH_SEPARATOR	'\\'
#elif defined( __linux__ )
#define BUILDER_PATH_SEPARATOR	'/'
#endif

#if defined( _MSC_VER ) && !defined( __cplusplus )
#define BUILDER_THREAD_LOCAL	__declspec( thread )
#else
#define BUILDER_THREAD_LOCAL	__thread
#endif

#if defined( _WIN32 )
#define BUILDER_MAX_PATH	MAX_PATH
#elif defined( __linux__ )
#define BUILDER_MAX_PATH	PATH_MAX
#endif

#ifndef BUILDER_COUNT_OF
#define BUILDER_COUNT_OF( array )	( sizeof( array ) / sizeof( array[0] ) )
#endif

// alignof is only a keyword in C++ and C23. C11 and C17 have it as a macro in <stdalign.h>, which we'd rather not
// pull in (or clash with), and _Alignof has been a keyword since C11 anyway.
#ifdef __cplusplus
#define BUILDER_ALIGNOF( type )		alignof( type )
#elif defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 201112L
#define BUILDER_ALIGNOF( type )		_Alignof( type )
#else
#define BUILDER_ALIGNOF( type )		__alignof__( type )
#endif

#define ARG_HELP_SHORT				"-h"
#define ARG_HELP_LONG				"--help"
#define ARG_VERBOSE_SHORT			"-v"
#define ARG_VERBOSE_LONG			"--verbose"
#define ARG_CONFIG					"--config="

#define ARENA_DEFAULT_BLOCK_SIZE	( 2 * 1024 * 1024 )

#define NUM_SCRATCH_ARENAS			2

enum {
	BUILDER_VERSION_MAJOR	= 1,
	BUILDER_VERSION_MINOR	= 0,
	BUILDER_VERSION_PATCH	= 0,
};

typedef struct arena_t arena_t;

typedef struct arenaBlock_t {
	void				*block;
	uint64_t			position;
	size_t				capacity;
	// Orders two blocks without walking the chain - see Builder_RewindScratch().
	// Only valid because blocks are always appended to the end, never spliced into the middle.
	uint64_t			index;
	struct arenaBlock_t	*next;
	struct arena_t*		owner;
} arenaBlock_t;

typedef struct arena_t {
	arenaBlock_t	*head;
	arenaBlock_t	*tail;
} arena_t;

typedef struct arenaRewindSpot_t {
	arenaBlock_t	*block;
	uint64_t		position;
} arenaRewindSpot_t;

typedef struct scratch_t {
	arena_t				*arena;
	arenaRewindSpot_t	rewind;
} scratch_t;


BUILDER_THREAD_LOCAL arena_t g_scratches[NUM_SCRATCH_ARENAS];

arenaRewindSpot_t Builder_ArenaTell( arena_t *arena ) {
	BUILDER_ASSERT( arena );

	arenaRewindSpot_t marker;
	marker.block = arena->tail;
	marker.position = arena->tail ? arena->tail->position : 0;

	return marker;
}

scratch_t Builder_GetScratch( arena_t *activeArena ) {
	// NULL means the caller isn't holding one, so there's nothing to avoid
	for ( int scratchIndex = 0; scratchIndex < NUM_SCRATCH_ARENAS; scratchIndex++ ) {
		arena_t *scratchArena = &g_scratches[scratchIndex];

		if ( activeArena != scratchArena ) {
			scratch_t onThisArena;
			onThisArena.arena = scratchArena;
			onThisArena.rewind = Builder_ArenaTell(scratchArena);

			return onThisArena;
		}
	}

	BUILDER_ASSERT( false && "No free scratch arena - activeArena occupied every slot" );

	scratch_t empty = { 0 };

	return empty;
}

void Builder_RewindArena( arena_t *arena, arenaRewindSpot_t *rewindLocation ) {
	BUILDER_ASSERT( arena );
	BUILDER_ASSERT( rewindLocation );

	if ( rewindLocation->block ) {
		BUILDER_ASSERT( rewindLocation->block->owner == arena );
	}

	// A rewind spot can only wind backwards. A NULL block is the arena's earliest state, so it's always valid.
	BUILDER_ASSERT( ( rewindLocation->block == NULL
		|| ( arena->tail != NULL
			&& ( rewindLocation->block->index < arena->tail->index
				|| ( rewindLocation->block == arena->tail && rewindLocation->position <= rewindLocation->block->position ) ) ) )
		&& "rewind spot is ahead of the arena - it was already rewound, or rewound out of order" );

	// Builder_ArenaAllocateInternal() zeroes the blocks past this one as it moves back onto them.
	arena->tail = rewindLocation->block;

	if ( rewindLocation->block != NULL ) {
		rewindLocation->block->position = rewindLocation->position;
	}
}

void Builder_RewindScratch( scratch_t *scratch ) {
	BUILDER_ASSERT( scratch );

	Builder_RewindArena( scratch->arena, &scratch->rewind );
}

void Builder_FreeScratch( void ) {
	for ( int scratchIndex = 0; scratchIndex < NUM_SCRATCH_ARENAS; scratchIndex++ ) {
		arena_t *arena = &g_scratches[scratchIndex];
		arenaBlock_t *block = arena->head;

		while ( block != NULL ) {
			arenaBlock_t *next = block->next;

			free( block );	// block->block was allocated as part of this same malloc - see arenaAllocate().

			block = next;
		}

		arena->head = NULL;
		arena->tail = NULL;
	}
}

void *Builder_ArenaAllocateInternal( arena_t *arena, size_t size, size_t alignment ) {
	BUILDER_ASSERT( arena );

	arenaBlock_t *block = arena->tail;

	for ( ;; ) {
		if ( block != NULL ) {
			uint64_t alignedPosition = ( block->position + alignment - 1 ) & ~( alignment - 1 );

			if ( alignedPosition + size <= block->capacity ) {
				void *result = (char *) block->block + alignedPosition;

				block->position = alignedPosition + size;
				arena->tail = block;

				return result;
			}
		}

		// Doesn't fit in the current block (or there is no current block yet) -
		// move forward to the next one, reusing it if Builder_RewindScratch() already left one there.
		arenaBlock_t *next = block ? block->next : arena->head;

		if ( next == NULL ) {
			// Nothing left to reuse - lazily allocate a new block.
			// The extra `alignment` bytes guarantee this block fits `size` even after
			// AlignUp() padding pushes the start position forward.
			size_t capacity = size + alignment;

			if ( capacity < ARENA_DEFAULT_BLOCK_SIZE ) {
				capacity = ARENA_DEFAULT_BLOCK_SIZE;
			}

			next = (arenaBlock_t *) malloc( sizeof( arenaBlock_t ) + capacity );
			BUILDER_ASSERT( next != NULL && "Out of memory." );

			next->block		= (void *) ( next + 1 );
			next->position	= 0;
			next->capacity	= capacity;
			next->index		= block ? block->index + 1 : 0;
			next->next		= NULL;
			next->owner		= arena;

			if ( block != NULL ) {
				block->next = next;
			} else {
				arena->head = next;
			}
		} else {
			// Left behind by Builder_RewindScratch(). Anything past the tail is dead, so resetting it here is safe.
			next->position = 0;
		}

		block = next;
	}
}

#define Builder_ArenaAlloc( arena, type, count )	( (type *) Builder_ArenaAllocateInternal( ( arena ), sizeof( type ) * ( count ), BUILDER_ALIGNOF( type ) ) )

// Temporary stand-in until we have a proper growable array. There's no portable way to recover
// "type" from an old pointer alone (no typeof in plain C11/C17, and MSVC/GCC/Clang don't agree on
// any extension for it), so it has to be passed in explicitly like scratchPush's. oldCount is
// needed too - scratch arenas only ever grow forward, so the new block has no idea how much of
// the old one was live and memcpy needs a length. This leaks the old block, same as every other
// scratch allocation - nothing here is individually freed until the whole arena rewinds.
#define Builder_ArenaRealloc( arena, old, type, oldCount, newCount ) \
	( (type *) memcpy( Builder_ArenaAlloc( ( arena ), type, ( newCount ) ), ( old ), sizeof( type ) * (	size_t ) ( oldCount ) ) )

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

static char *Builder_FormatStringV( arena_t *arena, const char *fmt, va_list args ) {
	va_list argsCopy;
	va_copy( argsCopy, args );

	int length = vsnprintf( NULL, 0, fmt, args );

	char *result = Builder_ArenaAlloc( arena, char, (uint64_t) length + 1 );
	vsnprintf( result, (size_t) length + 1, fmt, argsCopy );
	va_end( argsCopy );

	return result;
}

static char *Builder_FormatString( arena_t *arena, const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );

	char *result = Builder_FormatStringV( arena, fmt, args );

	va_end( args );

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

static void Builder_LogVerbose( const BuilderOptions *options, const char *fmt, ... ) {
	if ( !options->verboseLogging ) {
		return;
	}

	printf( "VERBOSE: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

// name of the BuildConfig the user wants built: whatever --config=<name> says, or BuilderOptions::defaultConfig if that arg wasn't given
static const char *Builder_GetNameOfConfigToBuild( const BuilderOptions *options, int argc, char **argv ) {
	for ( int argIndex = 0; argIndex < argc; argIndex++ ) {
		if ( Builder_StringStartsWith( argv[argIndex], ARG_CONFIG ) ) {
			return argv[argIndex] + strlen( ARG_CONFIG );
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


static void StringBuilder_Appendf( arena_t *arena, stringBuilder_t *builder, const char *fmt, ... ) {
	BUILDER_ASSERT( arena );
	BUILDER_ASSERT( builder );
	BUILDER_ASSERT( fmt );

	va_list args;
	va_start( args, fmt );

	stringBuilderBuffer_t *buffer = Builder_ArenaAlloc( arena, stringBuilderBuffer_t, 1 );
	memset( buffer, 0, sizeof( stringBuilderBuffer_t ) );

	va_list argsCopy;
	va_copy( argsCopy, args );

	buffer->length = (uint32_t) vsnprintf( NULL, 0, fmt, args );

	buffer->data = Builder_ArenaAlloc( arena, char, buffer->length + 1 );
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

static char *StringBuilder_ToString( arena_t *arena, stringBuilder_t *builder ) {
	BUILDER_ASSERT( arena );
	BUILDER_ASSERT( builder );

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

	result = Builder_ArenaAlloc( arena, char, totalLength );

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
	scratch_t scratch = Builder_GetScratch( NULL );
	char *pathCopy = Builder_ArenaAlloc( scratch.arena, char, pathLength + 1 );
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

	Builder_RewindScratch( &scratch );

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

static bool Builder_WriteStringBuilderToFile( arena_t *arena, const stringBuilder_t *sb, const char *filename ) {
	const char *str = StringBuilder_ToString( arena, (stringBuilder_t *) sb );
	return Builder_WriteEntireFile( filename, str );
}

static bool HasCommandLineArg( int argc, char **argv, const char *arg ) {
	for ( int argIndex = 0; argIndex < argc; argIndex++ ) {
		if ( Builder_StringEquals( argv[argIndex], arg ) ) {
			return true;
		}
	}

	return false;
}

#define STRING_CHUNK_SIZE 16

// Grows by chaining another chunk on rather than reallocating, so entries already added never move and it can be
// appended to while it's being walked.
typedef struct builderStringChunk_t {
	const char					*items[STRING_CHUNK_SIZE];
	uint32_t					count;
	struct builderStringChunk_t	*next;
} builderStringChunk_t;

// same shape as builderStringChunk_t, but it chains backwards as well - Builder_CollectConfigsToBuild() uses one of
// these lists as a stack, and popping has to find the chunk before the current one
#define BUILD_CONFIG_PTR_CHUNK_SIZE 8
typedef struct buildConfigPtrChunk_t {
	BuildConfig						*items[BUILD_CONFIG_PTR_CHUNK_SIZE];
	uint32_t						count;
	struct buildConfigPtrChunk_t	*next;
	struct buildConfigPtrChunk_t	*previous;
} buildConfigPtrChunk_t;

static void Builder_StringListPush( arena_t *arena, StringList *list, const char *string ) {
	BUILDER_ASSERT( list );

	if ( !list->tail || list->tail->count == STRING_CHUNK_SIZE ) {
		builderStringChunk_t *chunk = Builder_ArenaAlloc( arena, builderStringChunk_t, 1 );
		chunk->count	= 0;
		chunk->next		= NULL;

		if ( list->tail ) {
			list->tail->next = chunk;
		} else {
			list->head = chunk;
		}

		list->tail = chunk;
	}

	list->tail->items[list->tail->count++] = string;
	list->count++;
}

static void Builder_ConfigListPush( arena_t *arena, ConfigPtrList *list, BuildConfig *config ) {
	BUILDER_ASSERT( list );

	// popping walks tail back, so tail can be NULL while head still holds the chain, and any chunk past the tail
	// was emptied rather than freed - pick those up before allocating another
	buildConfigPtrChunk_t *tail = list->tail ? list->tail : list->head;

	if ( tail && tail->count == BUILD_CONFIG_PTR_CHUNK_SIZE ) {
		tail = tail->next;
	}

	if ( !tail ) {
		buildConfigPtrChunk_t *chunk = Builder_ArenaAlloc( arena, buildConfigPtrChunk_t, 1 );
		chunk->count	= 0;
		chunk->next		= NULL;
		chunk->previous	= list->tail;

		if ( list->tail ) {
			list->tail->next = chunk;
		} else {
			list->head = chunk;
		}

		tail = chunk;
	}

	list->tail = tail;

	list->tail->items[list->tail->count++] = config;
	list->count++;
}

// only the ancestry stack pops - emptied chunks are left chained on for Builder_ConfigListPush() to pick up again
static void Builder_ConfigListPop( ConfigPtrList *list ) {
	BUILDER_ASSERT( list );
	BUILDER_ASSERT( list->count > 0 && "Popped a config list that had nothing in it." );

	list->tail->count--;
	list->count--;

	if ( list->tail->count == 0 ) {
		list->tail = list->tail->previous;
	}
}

// Every BuildConfig, and every string one of them owns, lives on this arena.  Builder_GetScratch( NULL ) always hands
// back the same one and nothing rewinds it, so anything put here outlasts the scratches that come and go on top of it.
// g_scratches is BUILDER_THREAD_LOCAL though, so it's one arena per thread: creating or adding to a config has to stay
// on the main thread, or this needs to become an arena of its own outside g_scratches.  Reading it anywhere is fine.
static arena_t *Builder_GetConfigArena( void ) {
	scratch_t configStorage = Builder_GetScratch( NULL );

	return configStorage.arena;
}

BuildConfig *CreateBuildConfig( BuilderOptions *options ) {
	BUILDER_ASSERT( options );

	arena_t *configArena = Builder_GetConfigArena();

	BuildConfig *config = Builder_ArenaAlloc( configArena, BuildConfig, 1 );
	memset( config, 0, sizeof( BuildConfig ) );

	Builder_ConfigListPush( configArena, &options->configs, config );

	return config;
}

const char *Config_FormatString( const char *fmt, ... ) {
	BUILDER_ASSERT( fmt );

	va_list args;
	va_start( args, fmt );

	const char *result = Builder_FormatStringV( Builder_GetConfigArena(), fmt, args );

	va_end( args );

	return result;
}

StringList Builder_MakeStringListInternal( const char **strings, uint32_t count ) {
	StringList list = { 0 };

	Builder_AddStringsInternal( &list, strings, count );

	return list;
}

ConfigPtrList Builder_MakeDependenciesInternal( BuildConfig **dependencies, uint32_t count ) {
	BUILDER_ASSERT( dependencies );

	ConfigPtrList list = { 0 };
	arena_t *configArena = Builder_GetConfigArena();

	for ( uint32_t dependencyIndex = 0; dependencyIndex < count; dependencyIndex++ ) {
		BuildConfig *dependency = dependencies[dependencyIndex];

		BUILDER_ASSERT( dependency );

		Builder_ConfigListPush( configArena, &list, dependency );
	}

	return list;
}

void Builder_AddStringsInternal( StringList *list, const char **strings, uint32_t count ) {
	BUILDER_ASSERT( list );
	BUILDER_ASSERT( strings );

	arena_t *configArena = Builder_GetConfigArena();

	for ( uint32_t stringIndex = 0; stringIndex < count; stringIndex++ ) {
		const char *string = strings[stringIndex];

		BUILDER_ASSERT( string && string[0] && "Adding an empty entry to a BuildConfig list doesn't do anything." );

		// the string itself isn't copied - almost every entry is a literal, and Config_FormatString() covers the rest
		Builder_StringListPush( configArena, list, string );
	}
}

void Builder_AddDependenciesInternal( BuildConfig *config, BuildConfig **dependencies, uint32_t count ) {
	BUILDER_ASSERT( config );
	BUILDER_ASSERT( dependencies );

	arena_t *configArena = Builder_GetConfigArena();

	for ( uint32_t dependencyIndex = 0; dependencyIndex < count; dependencyIndex++ ) {
		BuildConfig *dependency = dependencies[dependencyIndex];

		BUILDER_ASSERT( dependency );
		BUILDER_ASSERT( config != dependency && "A BuildConfig can't depend on itself." );

		Builder_ConfigListPush( configArena, &config->dependsOn, dependency );
	}
}

// Depth-first walk of config->dependsOn that appends each config to outConfigsToBuild only once everything it depends
// on is already in there, so the build order falls out of the walk.
// ancestry is the chain of configs the walk is currently inside (config -> dependency -> ...) and starts out empty.  A
// config turning up in there again is a cycle, which there's no sensible way to build, so it's fatal.
// arena backs both lists, plus the error message if it comes to that.
static void Builder_CollectConfigsToBuild( arena_t *arena, BuildConfig *config, ConfigPtrList *ancestry, ConfigPtrList *outConfigsToBuild ) {
	BUILDER_ASSERT( config );
	BUILDER_ASSERT( ancestry );
	BUILDER_ASSERT( outConfigsToBuild );

	// multiple configs can rely on the same config (e.g. configs A and B may both rely on config C), and anything
	// already in the list has had its whole subtree walked and cleared of cycles
	for ( buildConfigPtrChunk_t *chunk = outConfigsToBuild->head; chunk; chunk = chunk->next ) {
		for ( uint32_t collectedIndex = 0; collectedIndex < chunk->count; collectedIndex++ ) {
			if ( chunk->items[collectedIndex] == config ) {
				return;
			}
		}
	}

	{
		// where in the ancestry the config turned up, if it did - the cycle is everything from there onwards
		buildConfigPtrChunk_t *cycleChunk = NULL;
		uint32_t cycleIndex = 0;

		for ( buildConfigPtrChunk_t *chunk = ancestry->head; chunk && !cycleChunk; chunk = chunk->next ) {
			for ( uint32_t ancestorIndex = 0; ancestorIndex < chunk->count; ancestorIndex++ ) {
				if ( chunk->items[ancestorIndex] == config ) {
					cycleChunk = chunk;
					cycleIndex = ancestorIndex;
					break;
				}
			}
		}

		if ( cycleChunk ) {
			// throwaway scratch, we're about to exit
			scratch_t errorScratch = Builder_GetScratch( arena );
			stringBuilder_t cycle = { 0 };

			// picking the walk back up where the match was found spells out the cycle and nothing that came before it
			for ( buildConfigPtrChunk_t *chunk = cycleChunk; chunk; chunk = chunk->next ) {
				for ( uint32_t ancestorIndex = ( chunk == cycleChunk ) ? cycleIndex : 0; ancestorIndex < chunk->count; ancestorIndex++ ) {
					StringBuilder_Appendf( errorScratch.arena, &cycle, "%s -> ", chunk->items[ancestorIndex]->name );
				}
			}

			StringBuilder_Appendf( errorScratch.arena, &cycle, "%s", config->name );

			char *cycleString = StringBuilder_ToString( errorScratch.arena, &cycle );
			Builder_Error( "Cyclic BuildConfig dependency detected: %s\n", cycleString );

			exit( 1 );
		}
	}

	Builder_ConfigListPush( arena, ancestry, config );

	// dependencies go in first so they show up (and get built) ahead of the config that needs them
	for ( buildConfigPtrChunk_t *chunk = config->dependsOn.head; chunk; chunk = chunk->next ) {
		for ( uint32_t dependencyIndex = 0; dependencyIndex < chunk->count; dependencyIndex++ ) {
			Builder_CollectConfigsToBuild( arena, chunk->items[dependencyIndex], ancestry, outConfigsToBuild );
		}
	}

	Builder_ConfigListPop( ancestry );

	Builder_ConfigListPush( arena, outConfigsToBuild, config );
}


static int32_t Builder_RunProcess( arena_t* results, const char *processAndArgs, bool discardStderr, char **outCapturedOutput ) {
#if defined( _WIN32 )
	SECURITY_ATTRIBUTES secAttr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	PROCESS_INFORMATION	processInfo;

	// TODO: AK: 21/08/2026: We are leaking handles everytime we return due to an error
	HANDLE stdoutRead = NULL;
	HANDLE stdoutWrite = NULL;
	HANDLE stderrWrite = NULL;
	if ( !CreatePipe( &stdoutRead, &stdoutWrite, &secAttr, 0 ) ) {
		Builder_Error( "CreatePipe call failed for stdout: 0x%X.\n", GetLastError() );
		return -1;
	}

	if ( discardStderr ) { 
		stderrWrite = CreateFile(
			"NUL",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			&secAttr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if ( stderrWrite == INVALID_HANDLE_VALUE ) {
			Builder_Error( "CreateFile call failed to open NUL device: 0x%X.\n", GetLastError() );
			return -1;
    	}
	} else {
		stderrWrite = stdoutWrite;
	}

	STARTUPINFO startInfo = { sizeof( startInfo ) };
	startInfo.dwFlags = STARTF_USESTDHANDLES;
	startInfo.hStdOutput = stdoutWrite;
	startInfo.hStdError = stderrWrite;

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
		return -1;
	}

	// close the write ends of the pipes on the parent side
	// the child inherited its own copies so this must happen before we start reading
	// otherwise the parents dangling copy keeps the pipe "open" and ReadFile below blocks forever once the child exits
	if ( !CloseHandle( stdoutWrite ) ) {
		Builder_Error( "Failed to close stdout write handle: Windows error code: 0x%X\n", GetLastError() );
		return -1;
	}
	if ( discardStderr && !CloseHandle( stderrWrite ) ) {
		Builder_Error( "Failed to close stderr write handle: Windows error code: 0x%X\n", GetLastError() );
		return -1;
	}
	stdoutWrite = NULL;
	stderrWrite = NULL;

	char buffer[1024] = { 0 };
	stringBuilder_t capturedOutput = { 0 };

	// the builder's own buffers are throwaway, so they go somewhere other than the scratch the caller wants the result in
	scratch_t temporaryScratch = Builder_GetScratch( results );

	bool finished = false;
	while ( !finished ) {
		DWORD bytesRead = 0;
		DWORD toRead = sizeof( buffer ) - 1;
		if ( ReadFile( stdoutRead, buffer, toRead, &bytesRead, NULL ) && bytesRead != 0 ) {
			buffer[bytesRead] = 0;
			if ( outCapturedOutput ) {
				StringBuilder_Appendf( temporaryScratch.arena, &capturedOutput, "%s", buffer );
			} else {
				printf( "%s", buffer );
			}
		} else {
			// the child closing its end of the pipe (e.g. on exit) surfaces as ERROR_BROKEN_PIPE here - that's expected EOF, not a real failure
			DWORD lastError = GetLastError();
			if ( lastError != ERROR_BROKEN_PIPE ) {
				Builder_Error( "Failed to read stdout of subprocess: Windows error code: 0x%X.\n", lastError );
			}
			finished = false;
			break;
		}
	}

	if ( outCapturedOutput ) {
		BUILDER_ASSERT( results && "capturing output needs an arena to put the result in" );

		*outCapturedOutput = StringBuilder_ToString( results, &capturedOutput );
	}

	Builder_RewindScratch( &temporaryScratch );

	// wait for process to finish
	if ( !CloseHandle( stdoutRead ) ) {
		Builder_Error( "Failed to close stdout read handle: Windows error code: 0x%X\n", GetLastError() );
		return -1;
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

	if ( !CloseHandle( processInfo.hProcess ) || !CloseHandle( processInfo.hThread ) ) {
		Builder_Error( "Failed to close a process handle: Windows error code: 0x%X\n", GetLastError() );
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

		// TODO: AK: 21/08/2026: this needs verifying / testing @dangmoody :)
		if ( discardStderr ) {
			int devNull = open( "/dev/null", O_WRONLY );
			if ( devNull == -1 ) { 
				err = errno;
				Builder_Error( "Failed to open /dev/null: %s\n", strerror( err ) );
			}
			
			if ( dup2( devNull, STDERR_FILENO ) == -1 ) { 
				err = errno;
				Builder_Error( "Failed to duplicate stderr to /dev/null: %s\n", strerror( err ) );
			}
			if ( close( devNull ) == -1 ) { 
				err = errno;
				Builder_Error( "Failed to close dev/null: %s\n", strerror( err ) );
			}
		} else if ( dup2( stdoutPipe[1], STDERR_FILENO ) == -1 ) {
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

	char buffer[1024] = { 0 };
	ssize_t bytesRead = 0;

	stringBuilder_t capturedOutput = { 0 };

	// the builder's own buffers are throwaway, so they go somewhere other than the scratch the caller wants the result in
	scratch_t temporaryScratch = Builder_GetScratch( results );

	while ( ( bytesRead = read( stdoutPipe[0], buffer, sizeof( buffer ) - 1 ) ) > 0 ) {
		buffer[bytesRead] = 0;

		if ( outCapturedOutput ) {
			StringBuilder_Appendf( temporaryScratch.arena, &capturedOutput, "%s", buffer );
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
		BUILDER_ASSERT( results && "capturing output needs an arena to put the result in" );

		*outCapturedOutput = StringBuilder_ToString( results, &capturedOutput );
	}

	Builder_RewindScratch( &temporaryScratch );

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
	scratch_t scratch = Builder_GetScratch( NULL );
	// we need the exe filename on windows to end with ".exe"
	// otherwise the file will fail to be found
#ifdef _WIN32
	if ( !Builder_PathHasFileExtension( binaryPath, ".exe" ) ) {
		binaryPath = Builder_FormatString( scratch.arena, "%s.exe", argv[0] );
	}

	// if the old binary was left around from the previous build, clean it up now
	{
		char *oldBackupPath = Builder_FormatString( scratch.arena, "%s.rebuild.old", binaryPath );
		DeleteFile( oldBackupPath );
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
		Builder_RewindScratch( &scratch );
		return;
	}

	printf( "'%s' is stale, rebuilding...\n", binaryPath );

	stringBuilder_t tempPathBuilder = { 0 };
	StringBuilder_Appendf( scratch.arena, &tempPathBuilder, "%s.rebuild.tmp", binaryPath );
	const char *tempBinaryPath = StringBuilder_ToString( scratch.arena, &tempPathBuilder );

	stringBuilder_t compileArgs = { 0 };
	StringBuilder_Appendf( scratch.arena, &compileArgs, "clang " );
	StringBuilder_Appendf( scratch.arena, &compileArgs, "-o %s ", tempBinaryPath );
	StringBuilder_Appendf( scratch.arena, &compileArgs, "%s ", sourceFile );

	const char *compileCmd = StringBuilder_ToString( scratch.arena, &compileArgs );

	printf( "%s\n", compileCmd );

	if ( Builder_RunProcess( NULL, compileCmd, false, NULL ) != 0 ) {
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
	stringBuilder_t backupPathBuilder = { 0 };
	StringBuilder_Appendf( scratch.arena, &backupPathBuilder, "%s.rebuild.old", binaryPath );
	const char *backupBinaryPath = StringBuilder_ToString( scratch.arena, &backupPathBuilder );

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
	stringBuilder_t execArgs = { 0 };
	StringBuilder_Appendf( scratch.arena, &execArgs, "\"%s\" ", binaryPath );

	for ( int argIndex = 1; argIndex < argc; argIndex++ ) {
		StringBuilder_Appendf( scratch.arena, &execArgs, "\"%s\" ", argv[argIndex] );
	}

	const char *execCmd = StringBuilder_ToString( scratch.arena, &execArgs );

	int32_t exitCode = Builder_RunProcess( NULL, execCmd, false, NULL );

	exit( exitCode );
#elif defined( __linux__ )
	if ( execv( binaryPath, argv ) == -1 ) {
		int err = errno;
		Builder_Error( "Failed to re-exec '%s': %s\n", binaryPath, strerror( err ) );

		exit( 1 );
	}
#endif
	Builder_RewindScratch( &scratch );
}

static int ShowUsage( const int exitCode ) {
	printf(
		"Builder\n"
		"\n"
		"USAGE:\n"
		"    <your build program> [arguments] [custom arguments]\n"
		"\n"
		"Arguments:\n"
		"    " ARG_HELP_SHORT "|" ARG_HELP_LONG " (optional):\n"
		"        Shows this help and then exits.\n"
		"\n"
		"    " ARG_VERBOSE_SHORT "|" ARG_VERBOSE_LONG " (optional):\n"
		"        Enables verbose logging, so a lot more information gets output.\n"
		"\n"
		"    " ARG_CONFIG "<config> (optional):\n"
		"        Sets the config to build to <config>.\n"
		"        This must match the name of a config you registered via AddBuildConfig().\n"
		"        If you only registered one config you don't need to specify this.\n"
		"        If you registered more than one config you must either specify this or set BuilderOptions::defaultConfig.\n"
		"\n"
		"    [custom arguments] (optional):\n"
		"        Any arguments not listed here are passed through to your build program via main()'s argc/argv.\n"
		"        Use HasCommandLineArg( int, char **, const char * ) to query for them.\n"
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

static const char * Builder_GetBinaryPath( arena_t *arena, const BuildConfig *config ) {
	if ( config->binaryFolder ) {
		return Builder_FormatString( arena, "%s%c%s%s", config->binaryFolder, BUILDER_PATH_SEPARATOR, config->binaryName, Builder_GetFileExtensionFromBinaryType( config->binaryType ) );
	} else {
		return Builder_FormatString( arena, "%s%s", config->binaryName, Builder_GetFileExtensionFromBinaryType( config->binaryType ) );
	}
}

static uint64_t Builder_HashString( const char *string ) {
	uint64_t hash = 0;

	while ( *string ) {
		hash ^= (unsigned char) *(string++);
		hash *= UINT64_C(1099511628211);
  }

	return hash;
}

// we get the hash of the compile command to figure out what the name of the sourceFile should be
static const char* Builder_GetIntermediateFilePath( arena_t *arena, const char *intermediateFolder, const char* compileCommand, const char* sourceFile ) {
	const char *fileName = sourceFile;

	for ( const char *c = sourceFile; *c; c++ ) {
		if ( *c == '/' || *c == '\\' ) {
			fileName = c + 1;
		}
	}

	const char *extension = strrchr( fileName, '.' );
	size_t fileNameLength = extension ? (size_t) ( extension - fileName ) : strlen( fileName );

	uint64_t hash = Builder_HashString( compileCommand );

	return Builder_FormatString( arena, "%s%c%.*s_%" PRIu64 ".o", intermediateFolder, BUILDER_PATH_SEPARATOR, (int) fileNameLength, fileName, hash );
}

typedef struct {
	uint64_t	sizeBytes;
	uint64_t	lastWriteTime;
	bool		isDirectory;
	const char	*filename;
	const char	*fullFilename;
} fileInfo_t;

// results is for anything the callback allocates that has to outlive the walk.  It's optional - pass NULL if the
// callback doesn't allocate.  Builder_VisitFiles() never allocates from it itself.
typedef void ( *builderFileVisitCallback_t )( arena_t *resultsArena, fileInfo_t *fileInfo, void *data );

typedef enum {
	BUILDER_FILE_VISIT_FILES		= 1 << 0,
	BUILDER_FILE_VISIT_FOLDERS		= 1 << 1,
	BUILDER_FILE_VISIT_RECURSIVE	= 1 << 2,
} builderFileVisitFlagBits_t;
typedef uint32_t builderFileVisitFlags_t;

static bool Builder_VisitFiles( arena_t *results, const char *path, const builderFileVisitFlags_t visitFlags, builderFileVisitCallback_t callback, void *data ) {
	BUILDER_ASSERT( path );
	BUILDER_ASSERT( callback );

	// the paths we build to walk the tree are ours alone - only the callback's allocations outlive us, and those go on results
	scratch_t scratch = Builder_GetScratch(results );

	StringList directories = { 0 };
	Builder_StringListPush( scratch.arena, &directories, path );

	// walking a folder pushes its subfolders on, so count grows underneath the inner loop and next appears when it overflows
	for ( builderStringChunk_t *chunk = directories.head; chunk; chunk = chunk->next ) {
		for ( uint32_t directoryIndex = 0; directoryIndex < chunk->count; directoryIndex++ ) {
			const char *dir = chunk->items[directoryIndex];

			size_t dirLength = strlen( dir );
			bool dirHasTrailingSeparator = dirLength > 0 && ( dir[dirLength - 1] == '\\' || dir[dirLength - 1] == '/' );

#if defined( _WIN32 )
			char *searchPath = Builder_FormatString( scratch.arena, dirHasTrailingSeparator ? "%s*" : "%s\\*", dir );

			WIN32_FIND_DATA findData = { 0 };
			HANDLE handle = FindFirstFile( searchPath, &findData );

			if ( handle == INVALID_HANDLE_VALUE ) {
				Builder_RewindScratch( &scratch );
				return false;
			}

			while ( 1 ) {
				fileInfo_t fileInfo = {
					.sizeBytes		= ( (uint64_t) findData.nFileSizeHigh << 32 ) | findData.nFileSizeLow,
					.lastWriteTime	= ( (uint64_t) findData.ftLastWriteTime.dwHighDateTime << 32 ) | findData.ftLastWriteTime.dwLowDateTime,
					.isDirectory	= (bool) ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ),
					.filename		= findData.cFileName,
					.fullFilename	= Builder_FormatString( scratch.arena, dirHasTrailingSeparator ? "%s%s" : "%s\\%s", dir, findData.cFileName ),
				};

				if ( fileInfo.isDirectory ) {
					if ( !Builder_StringEquals( findData.cFileName, "." ) && !Builder_StringEquals( findData.cFileName, ".." ) ) {
						if ( visitFlags & BUILDER_FILE_VISIT_FOLDERS ) {
							callback( results, &fileInfo, data );
						}

						if ( visitFlags & BUILDER_FILE_VISIT_RECURSIVE ) {
							Builder_StringListPush( scratch.arena, &directories, fileInfo.fullFilename );
						}
					}
				} else if ( visitFlags & BUILDER_FILE_VISIT_FILES ) {
					callback( results, &fileInfo, data );
				}

				if ( !FindNextFile( handle, &findData ) ) {
					break;
				}
			}

			if ( !FindClose( handle ) ) {
				Builder_RewindScratch( &scratch );
				return false;
			}
#elif defined( __linux__ )
			DIR *handle = opendir( dir );
			int err = errno;

			if ( !handle ) {
				Builder_Error( "Failed to open folder \"%s\": %s\n", dir, strerror( err ) );
				Builder_RewindScratch( &scratch );
				return false;
			}

			struct dirent *entry = NULL;

			while ( ( entry = readdir( handle ) ) != NULL ) {
				if ( Builder_StringEquals( entry->d_name, "." ) || Builder_StringEquals( entry->d_name, ".." ) ) {
					continue;
				}

				char *fullFilename = Builder_FormatString( scratch.arena, dirHasTrailingSeparator ? "%s%s" : "%s/%s", dir, entry->d_name );

				struct stat fileStat = { 0 };

				if ( stat( fullFilename, &fileStat ) != 0 ) {
					err = errno;
					Builder_Error( "Failed to stat \"%s\": %s\n", fullFilename, strerror( err ) );
					Builder_RewindScratch( &scratch );
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
						callback( results, &fileInfo, data );
					}

					if ( visitFlags & BUILDER_FILE_VISIT_RECURSIVE ) {
						Builder_StringListPush( scratch.arena, &directories, fileInfo.fullFilename );
					}
				} else if ( visitFlags & BUILDER_FILE_VISIT_FILES ) {
					callback( results, &fileInfo, data );
				}
			}

			if ( closedir( handle ) != 0 ) {
				err = errno;
				Builder_Error( "Failed to close folder \"%s\": %s\n", dir, strerror( err ) );
				Builder_RewindScratch( &scratch );
				return false;
			}
#endif
		}
	}

	Builder_RewindScratch( &scratch );

	return true;
}

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
	StringList *globResults;
	bool verboseLogging;
} builderGlobVisitCallbackData_t;

// remember to free sliceArray.data
static builderStringSliceArray_t Builder_SliceFilePath( arena_t* results, const char *filePath ) {
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

	sliceArray.data = Builder_ArenaAlloc(results, builderStringSlice_t, numSlices);
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

		// consume match, resetting if the match fails (TODO: AK: 11/08/2026: explain this better?)
		if ( patternSlice->begin[patternIndex] != '?' // can match any one character
			&& patternSlice->begin[patternIndex] != pathSlice->begin[pathIndex] ) {
			patternIndex = afterLastWildcard;
		} else {
			++patternIndex;
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

static void Builder_GlobVisitCallback( arena_t *resultsArena, fileInfo_t *fileInfo, void *data ) {
	builderGlobVisitCallbackData_t *callbackData = (builderGlobVisitCallbackData_t *)data;

	uint32_t filenameLen = strnlen( fileInfo->filename, 255 ); // filename length maxes here I think?
	uint32_t fullFilenameLen = strnlen( fileInfo->fullFilename, BUILDER_MAX_PATH + filenameLen );

	if ( fullFilenameLen - filenameLen < callbackData->searchPathLen ) {
		printf( "Error: Search path length was longer than full file path for file %s\n", fileInfo->fullFilename );
		return;
	}

	// we could do a heavyweight verification that the search path matches here,
	// but really it should match from the call sites
	const char* matchStart = fileInfo->fullFilename + callbackData->searchPathLen;
	scratch_t scratch = Builder_GetScratch( resultsArena );
	const builderStringSliceArray_t pathSlices = Builder_SliceFilePath( scratch.arena, matchStart );

	if ( Builder_PathMatchesPattern( &callbackData->patternSlices, &pathSlices ) ) {
		if ( callbackData->verboseLogging ) {
			printf( "VERBOSE:  - Found \"%s\"\n", fileInfo->fullFilename );
		}

		// fileInfo->fullFilename lives on Builder_VisitFiles' internal scratch and won't survive past this
		// call, so it has to be copied into resultsArena to outlive the walk.
		const char *fullFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
		Builder_StringListPush( resultsArena, callbackData->globResults, fullFilename );
	}

	Builder_RewindScratch( &scratch);
}

// builds onto whatever arena it's handed - the caller flattens it if it needs to index the result
static StringList Builder_GlobFiles( arena_t *resultsArena, const StringList *globPatterns, const BuilderOptions *options ) {
	StringList globResult = { 0 };

	scratch_t scratch = Builder_GetScratch( resultsArena );
	// setup re-usable search path allocation
	char *const searchPath = Builder_ArenaAlloc( scratch.arena, char, ( BUILDER_MAX_PATH + 1 ) );

	for ( builderStringChunk_t *chunk = globPatterns->head; chunk; chunk = chunk->next ) {
		for ( uint32_t patternIndex = 0; patternIndex < chunk->count; patternIndex++ ) {
			const char *globPattern = chunk->items[patternIndex];

			// start by copying and seeking to find if there is an asterisk, and then rewind to just after the preceding slash (or start if it is say **/*.cpp)
			const char* pattern = globPattern;
			while ( *pattern != '\0' )  {  // we could also early out if they put any erroneous characters
				if ( *(pattern++) == '*' ) {
					while ( --pattern != globPattern ) {
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
				if ( pattern != globPattern ) {
					Builder_LogVerbose( options, "Adding source file \"%s\" to the list of source files to build with (no glob).\n", globPattern );

					Builder_StringListPush( resultsArena, &globResult, globPattern ); // yes I realise this wasn't globbed \_O_O_/
				}
				continue;
			}

			const uint32_t globPathLen = (uint32_t) ( pattern - globPattern );
			if ( globPathLen != 0 ) {
				if ( globPathLen > BUILDER_MAX_PATH ) {
					printf( "Warning: Skipping pattern %s, path was larger than max path.\n", globPattern );
					continue;
				}
				memcpy( searchPath, globPattern, globPathLen );
			}
			searchPath[globPathLen] = '\0';

			builderGlobVisitCallbackData_t callbackData = {
				.patternSlices = Builder_SliceFilePath( scratch.arena, pattern),
				.globResults = &globResult,
				.searchPathLen = globPathLen,
				.verboseLogging = options->verboseLogging
			};

			builderFileVisitFlags_t visitFlags = BUILDER_FILE_VISIT_FILES;
			if (callbackData.patternSlices.count > 1) { // we are matching folders too
				visitFlags |= BUILDER_FILE_VISIT_RECURSIVE;
			}

			Builder_LogVerbose( options, "About to glob all source files found under user-specified pattern \"%s\" to the list of source files to build with:\n", globPattern );

			if ( !Builder_VisitFiles( resultsArena, searchPath, visitFlags, &Builder_GlobVisitCallback, &callbackData ) ) {
				printf( "Warning: Found no matches for pattern %s, at search path %s.\n", globPattern, searchPath );
			}
		}
	}

	Builder_RewindScratch( &scratch);
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

// How many Windows SDK versions / MSVC toolsets we're willing to find on one machine. User can specify with defines
#ifndef BUILDER_MAX_TOOLCHAIN_VERSIONS
#define BUILDER_MAX_TOOLCHAIN_VERSIONS 16
#endif

typedef struct {
	builderMSVCInstall_t	installs[BUILDER_MAX_TOOLCHAIN_VERSIONS];
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

typedef struct {
	builderWindowsSDKVersion_t	versions[BUILDER_MAX_TOOLCHAIN_VERSIONS];
	uint32_t					versionsCount;
} builderFoundWindowsSDKVersionData_t;

static void OnWindowsSDKVersionFound( arena_t *results, fileInfo_t *fileInfo, void *data ) {
	builderFoundWindowsSDKVersionData_t *foundData = (builderFoundWindowsSDKVersionData_t *) data;

	builderWindowsSDKVersion_t version = { 0 };

	if ( sscanf( fileInfo->filename, "%d.%d.%d.%d", &version.v0, &version.v1, &version.v2, &version.v3 ) != 4 ) {
		return;
	}

	BUILDER_ASSERT( foundData->versionsCount < BUILDER_MAX_TOOLCHAIN_VERSIONS && "Found more Windows SDK versions than BUILDER_MAX_TOOLCHAIN_VERSIONS allows for. Define this above builder.h to expand the search" );

	foundData->versions[foundData->versionsCount++] = version;
}

static int Builder_CompareWindowsSDKVersions( const void *a, const void *b ) {
	const builderWindowsSDKVersion_t *versionA = (const builderWindowsSDKVersion_t *) a;
	const builderWindowsSDKVersion_t *versionB = (const builderWindowsSDKVersion_t *) b;

	if ( versionA->v0 != versionB->v0 ) return versionB->v0 - versionA->v0;
	if ( versionA->v1 != versionB->v1 ) return versionB->v1 - versionA->v1;
	if ( versionA->v2 != versionB->v2 ) return versionB->v2 - versionA->v2;

	return versionB->v3 - versionA->v3;
}

// The paths this hands back via outSDK live for the whole build, so they go on the caller's scratch.
static bool Builder_GetWindowsSDKInstall( arena_t *results, builderWindowsSDKInstall_t *outSDK ) {
	BUILDER_ASSERT( outSDK );

	bool success = false;
	HKEY key = NULL;
	const char *windowsSDKRoot = NULL;

	builderFoundWindowsSDKVersionData_t foundData = { 0 };
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
		// snapshot results first - if this doesn't pan out (wrong type, or the read fails) we rewind
		// back to here rather than leaving the failed attempt sat on it
		arenaRewindSpot_t attempt = Builder_ArenaTell( results );
		char *windowsSDKRootStr = Builder_ArenaAlloc( results, char, windowsSDKRootLength );

		DWORD windowsSDKRootType = 0;

		status = RegQueryValueExA( key, winSDKRegKey, NULL, &windowsSDKRootType, (LPBYTE) windowsSDKRootStr, &windowsSDKRootLength );

		if ( status == ERROR_SUCCESS && windowsSDKRootType == REG_SZ ) {
			windowsSDKRoot = windowsSDKRootStr;
		} else {
			Builder_RewindArena( results, &attempt );
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
		char *windowsSDKLibFolder = Builder_FormatString( results, "%sLib", windowsSDKRoot );

		bool visited = Builder_VisitFiles( results, windowsSDKLibFolder, BUILDER_FILE_VISIT_FOLDERS, OnWindowsSDKVersionFound, &foundData );

		if ( !visited ) {
			Builder_Error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have at least one version of the Windows SDK installed?\n" );
			goto cleanup;
		}
	}

	if ( foundData.versionsCount == 0 ) {
		Builder_Error( "Failed to find any versions of the Windows SDK installed under \"%s\".\n", windowsSDKRoot );
		goto cleanup;
	}

	// newest version first
	qsort( foundData.versions, foundData.versionsCount, sizeof( builderWindowsSDKVersion_t ), Builder_CompareWindowsSDKVersions );

	// find the first windows SDK folder that isnt malformed
	for ( uint32_t versionIndex = 0; versionIndex < foundData.versionsCount; versionIndex++ ) {
		builderWindowsSDKVersion_t *version = &foundData.versions[versionIndex];

		// these go straight onto the results scratch because they're what we hand back if this version turns out to be the one.
		// if it isn't, this marker lets us drop just this attempt without touching whatever the caller already had in there
		arenaRewindSpot_t attempt = Builder_ArenaTell( results );

		char *ucrtIncludeFolder = Builder_FormatString( results, "%sinclude\\%d.%d.%d.%d\\ucrt", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *umIncludeFolder = Builder_FormatString( results, "%sinclude\\%d.%d.%d.%d\\um", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *sharedIncludeFolder = Builder_FormatString( results, "%sinclude\\%d.%d.%d.%d\\shared", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *ucrtLibFolder = Builder_FormatString( results, "%sLib\\%d.%d.%d.%d\\ucrt\\x64", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );
		char *umLibFolder = Builder_FormatString( results, "%sLib\\%d.%d.%d.%d\\um\\x64", windowsSDKRoot, version->v0, version->v1, version->v2, version->v3 );

		uint32_t missingFoldersCount = 0;
		const char *missingFolders[5] = { 0 };

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
			scratch_t scratch = Builder_GetScratch( results );
			stringBuilder_t sb = { 0 };

			StringBuilder_Appendf( scratch.arena, &sb, "Version %d.%d.%d.%d of your Windows SDK installation is malformed because the following folder(s) could not be found:\n", version->v0, version->v1, version->v2, version->v3 );

			for ( uint32_t missingFolderIndex = 0; missingFolderIndex < missingFoldersCount; missingFolderIndex++ ) {
				StringBuilder_Appendf( scratch.arena, &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}

			StringBuilder_Appendf( scratch.arena, &sb, "You must have the following folders in your Windows SDK install:\n"
				"    include/<version>/ucrt\n"
				"    include/<version>/um\n"
				"    include/<version>/shared\n"
				"    Lib/<version>/ucrt/x64\n"
				"    Lib/<version>/um/x64\n"
			);

			StringBuilder_Appendf( scratch.arena, &sb, "If you want to use this version of the Windows SDK specifically, you will need to fix this yourself.\n" );

			char *message = StringBuilder_ToString( scratch.arena, &sb );
			Builder_Warning( "%s", message );

			Builder_RewindScratch( &scratch );

			// this version is no good, so drop the paths we speculatively built for it
			Builder_RewindArena( results, &attempt );

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
			, foundData.versionsCount
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
// the paths built here end up in the install we hand back, so they go on the caller's scratch
static void Builder_OnMSVCInstallFound( arena_t *results, fileInfo_t *fileInfo, void *data ) {
	BUILDER_ASSERT( results );

	builderFoundMSVCInstallData_t *foundData = (builderFoundMSVCInstallData_t *) data;

	builderMSVCVersion_t version = { 0 };

	if ( sscanf( fileInfo->filename, "%d.%d.%d", &version.v0, &version.v1, &version.v2 ) != 3 ) {
		return;
	}

	builderMSVCInstall_t install = {
		.rootFolder		= Builder_FormatString( results, "%s", fileInfo->fullFilename ),
		.includePath	= Builder_FormatString( results, "%s\\include", fileInfo->fullFilename ),
		.libPath		= Builder_FormatString( results, "%s\\lib\\x64", fileInfo->fullFilename ),
		.version		= version,
	};

	BUILDER_ASSERT( foundData->installsCount < BUILDER_MAX_TOOLCHAIN_VERSIONS && "Found more MSVC installs than BUILDER_MAX_TOOLCHAIN_VERSIONS allows for. Define this above builder.h to expand the search" );

	foundData->installs[foundData->installsCount++] = install;
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
// The paths this hands back via outInstall live for the whole build, so they go on the caller's scratch.
static bool Builder_GetMSVCInstall( arena_t *results, builderMSVCInstall_t *outInstall ) {
	BUILDER_ASSERT( results );
	BUILDER_ASSERT( outInstall );

	// the VS install paths we search through are only needed while we're searching, so they don't go on the caller's scratch
	scratch_t scratch = Builder_GetScratch( results );

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

	builderFoundMSVCInstallData_t foundData = { 0 };

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

			visualStudioInstallationPath = Builder_ArenaAlloc( scratch.arena, char, (uint64_t) utf8Length + 1 );

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

		char *msvcRootFolder = Builder_FormatString( scratch.arena, "%s\\VC\\Tools\\MSVC", visualStudioInstallationPath );

		if ( !Builder_VisitFiles( results, msvcRootFolder, BUILDER_FILE_VISIT_FOLDERS, Builder_OnMSVCInstallFound, &foundData ) ) {
			Builder_Error( "Failed to query for MSVC installation folders under \"%s\".\n", msvcRootFolder );
			instance->vtable->Release( instance );
			goto cleanup;
		}

		instance->vtable->Release( instance );

		hr = instances->vtable->Next( instances, 1, &instance, &foundInstance );
	}

	if ( foundData.installsCount == 0 ) {
		success = Builder_MSVCNotInstalled();
		goto cleanup;
	}

	// newest version first
	qsort( foundData.installs, foundData.installsCount, sizeof( builderMSVCInstall_t ), Builder_CompareMSVCInstallVersions );

	bool found = false;
	uint32_t useVersionIndex = 0;

	for ( uint32_t versionIndex = 0; versionIndex < foundData.installsCount; versionIndex++ ) {
		builderMSVCInstall_t *install = &foundData.installs[versionIndex];

		uint32_t missingFoldersCount = 0;
		const char *missingFolders[2] = { 0 };

		if ( !Builder_FolderExists( install->includePath ) ) {
			missingFolders[missingFoldersCount++] = install->includePath;
		}

		if ( !Builder_FolderExists( install->libPath ) ) {
			missingFolders[missingFoldersCount++] = install->libPath;
		}

		if ( missingFoldersCount > 0 ) {
			stringBuilder_t sb = { 0 };

			StringBuilder_Appendf( scratch.arena, &sb, "Version %d.%d.%d of your MSVC installation is malformed because the following folder(s) could not be found:\n", install->version.v0, install->version.v1, install->version.v2 );

			for ( uint32_t missingFolderIndex = 0; missingFolderIndex < missingFoldersCount; missingFolderIndex++ ) {
				StringBuilder_Appendf( scratch.arena, &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}

			StringBuilder_Appendf( scratch.arena, &sb, "You must have the following folders in your MSVC install:\n"
				"    %s\n"
				"    %s\n"
				, install->includePath
				, install->libPath
			);

			StringBuilder_Appendf( scratch.arena, &sb, "If you want to use this version of MSVC specifically, you will need to fix this yourself.\n" );

			char *message = StringBuilder_ToString( scratch.arena, &sb );
			Builder_Warning( "%s", message );

			Builder_RewindScratch( &scratch );

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

	*outInstall = foundData.installs[useVersionIndex];

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

// the version string it finds is handed back to the caller, so it goes on their scratch
static char *Builder_ExtractVersionNumber( arena_t *results, const char *text ) {
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
			return Builder_FormatString( results, "%.*s", (int) ( end - start ), start );
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
	BuildConfig	   *config;
	const char	   *compilerPath;
	bool			useMSVC;
	bool			debugDefineSet;
#if defined( _WIN32 )
	builderMSVCInstall_t		*msvcInstall;
	builderWindowsSDKInstall_t	*windowsSDKInstall;
#endif
} builderCompileContext_t;

static stringBuilder_t Builder_CreateCompilationCommand( arena_t *commandArena, builderCompileContext_t *context ) {
	BuildConfig *config = context->config;

	arenaRewindSpot_t rewind = Builder_ArenaTell( commandArena );

	stringBuilder_t compileArgs = { 0 };
	StringBuilder_Appendf( commandArena, &compileArgs, "\"%s\" ", context->compilerPath );

	if ( context->useMSVC ) {
#if defined( _WIN32 )
		builderMSVCInstall_t *msvcInstall = context->msvcInstall;
		builderWindowsSDKInstall_t *windowsSDKInstall = context->windowsSDKInstall;

		StringBuilder_Appendf( commandArena, &compileArgs, "/nologo " );	// disable MSVC spamming its copyright banner for every compilation unit
		StringBuilder_Appendf( commandArena, &compileArgs, "/c " );

		if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "/std:%s ", GetLanguageVersionString( config->languageVersion ) );
		}

		if ( !config->removeSymbols ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "/Z7 " );
		}

		StringBuilder_Appendf( commandArena, &compileArgs, "%s ", Builder_GetOptimizationString_MSVC( config->optimization ) );

		for ( builderStringChunk_t *chunk = config->defines.head; chunk; chunk = chunk->next ) {
			for ( uint32_t defineIndex = 0; defineIndex < chunk->count; defineIndex++ ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "/D%s ", chunk->items[defineIndex] );

				// TODO: AK: 11/08/2026: handle this better
				if ( !context->debugDefineSet && _strnicmp( "_DEBUG", chunk->items[defineIndex], sizeof( "_DEBUG" ) ) == 0 ) {
					context->debugDefineSet = true;
				}
			}
		}

		// cl.exe doesn't know where the CRT/Windows SDK headers live unless you're in a Developer Command Prompt, so point it there ourselves
		StringBuilder_Appendf( commandArena, &compileArgs, "/I\"%s\" /I\"%s\" /I\"%s\" /I\"%s\" "
			, msvcInstall->includePath
			, windowsSDKInstall->ucrtIncludePath
			, windowsSDKInstall->umIncludePath
			, windowsSDKInstall->sharedIncludePath );

		for ( builderStringChunk_t *chunk = config->additionalIncludes.head; chunk; chunk = chunk->next ) {
			for ( uint32_t includeIndex = 0; includeIndex < chunk->count; includeIndex++ ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "/I%s ", chunk->items[includeIndex] );
			}
		}

		if ( config->warningsAsErrors ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "/WX " );
		}

		bool sawWarningLevel = false;

		for ( builderStringChunk_t *chunk = config->warningLevels.head; chunk; chunk = chunk->next ) {
			for ( uint32_t warningLevelIndex = 0; warningLevelIndex < chunk->count; warningLevelIndex++ ) {
				const char *warningLevel = chunk->items[warningLevelIndex];

				if ( sawWarningLevel ) {
					Builder_Error( "MSVC only allows one warning level to be set at a time, but you specified more than one.\n" );
					Builder_RewindArena( commandArena, &rewind );
					return (stringBuilder_t) {0};
				}

				if ( !Builder_IsWarningLevelAllowed_MSVC( warningLevel ) ) {
					Builder_Error(
						"Warning level \"%s\" is not a valid one.  Allowed warning levels are:\n"
						"    /W0\n"
						"    /W1\n"
						"    /W2\n"
						"    /W3\n"
						"    /W4\n"
						"    /Wall\n"
						, warningLevel
					);

					Builder_RewindArena( commandArena, &rewind );
					return (stringBuilder_t) {0};
			}

				StringBuilder_Appendf( commandArena, &compileArgs, "%s ", warningLevel );

				sawWarningLevel = true;
			}
		}

		if ( context->debugDefineSet ) {
			if ( config->useDynamicRuntimeOnWindows ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "/MDd " );
			} else {
				StringBuilder_Appendf( commandArena, &compileArgs, "/MTd " );
			}
		} else {
			if ( config->useDynamicRuntimeOnWindows ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "/MD " );
			} else {
				StringBuilder_Appendf( commandArena, &compileArgs, "/MT " );
			}
		}
#endif
	} else {
		if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "-std=%s ", GetLanguageVersionString( config->languageVersion ) );
		}

		if ( !config->removeSymbols ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "-g " );
		}

		StringBuilder_Appendf( commandArena, &compileArgs, "%s ", Builder_GetOptimizationString_Clang( config->optimization ) );

#if defined( __linux__ )
		if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "-fPIC " );
		}
#endif

		StringBuilder_Appendf( commandArena, &compileArgs, "-c " );

		for ( builderStringChunk_t *chunk = config->defines.head; chunk; chunk = chunk->next ) {
			for ( uint32_t defineIndex = 0; defineIndex < chunk->count; defineIndex++ ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "-D%s ", chunk->items[defineIndex] );

				// TODO: AK: 11/08/2026: handle this better
				if ( !context->debugDefineSet && _strnicmp( "_DEBUG", chunk->items[defineIndex], sizeof( "_DEBUG" ) ) == 0 ) {
					context->debugDefineSet = true;
				}
			}
		}

#if defined( _WIN32 )
		if ( config->useDynamicRuntimeOnWindows ) {
			StringBuilder_Appendf( commandArena, &compileArgs, " -D_MT -D_DLL " );
		}
#endif

		for ( builderStringChunk_t *chunk = config->additionalIncludes.head; chunk; chunk = chunk->next ) {
			for ( uint32_t includeIndex = 0; includeIndex < chunk->count; includeIndex++ ) {
				StringBuilder_Appendf( commandArena, &compileArgs, "-I%s ", chunk->items[includeIndex] );
			}
		}

		if ( config->warningsAsErrors ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "-Werror " );
		}

		for ( builderStringChunk_t *chunk = config->warningLevels.head; chunk; chunk = chunk->next ) {
			for ( uint32_t warningLevelIndex = 0; warningLevelIndex < chunk->count; warningLevelIndex++ ) {
				const char *warningLevel = chunk->items[warningLevelIndex];

				if ( !Builder_IsWarningLevelAllowed_Clang( warningLevel ) ) {
					Builder_Error(
						"Warning level \"%s\" is not a valid one.  Allowed warning levels are:\n"
						"    -Wall\n"
						"    -Weverything\n"
						"    -Wextra\n"
						"    -Wpedantic\n"
						, warningLevel
					);

					Builder_RewindArena( commandArena, &rewind );
					return (stringBuilder_t) {0};
				}

				StringBuilder_Appendf( commandArena, &compileArgs, "%s ", warningLevel );
			}
		}
	}

	for ( builderStringChunk_t *chunk = config->ignoreWarnings.head; chunk; chunk = chunk->next ) {
		for ( uint32_t ignoreWarningIndex = 0; ignoreWarningIndex < chunk->count; ignoreWarningIndex++ ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "%s ", chunk->items[ignoreWarningIndex] );
		}
	}

	for ( builderStringChunk_t *chunk = config->additionalCompilerArguments.head; chunk; chunk = chunk->next ) {
		for ( uint32_t extraArgIndex = 0; extraArgIndex < chunk->count; extraArgIndex++ ) {
			StringBuilder_Appendf( commandArena, &compileArgs, "%s ", chunk->items[extraArgIndex] );
		}
	}
	
	return compileArgs;
}

static bool Builder_CompileSourceFile( const char *compileCommand ) {
	printf( "%s\n", compileCommand );
	
	int32_t compileResult = Builder_RunProcess( NULL, compileCommand, false, NULL );

	return compileResult == 0;
}

typedef struct compilePacket_t {
	const char *sourceFile;
	const char *intermediateFile;
	const char *compileCommand;
	uint64_t 	objectWriteTime;
} compilePacket_t;

typedef struct builderCompileJobPool_t {
	const char					  **compileCommands;
	uint32_t						compileCommandCount;
	builderAtomic32_t				nextCompileCommandIndex;
	builderAtomic32_t				numFailed;
} builderCompileJobPool_t;

static void Builder_RunCompileJobPool( builderCompileJobPool_t *pool ) {
	while ( 1 ) {
		uint32_t compileCommandIndex = Builder_AtomicIncrement( &pool->nextCompileCommandIndex ) - 1;

		if ( compileCommandIndex >= pool->compileCommandCount ) {
			break;
		}

		if ( !Builder_CompileSourceFile( pool->compileCommands[compileCommandIndex] ) ) {
			Builder_AtomicIncrement( &pool->numFailed );
		}
	}
}

#if defined( _WIN32 )
static DWORD WINAPI Builder_CompileJobThreadProc( LPVOID param ) {
	Builder_RunCompileJobPool( (builderCompileJobPool_t *) param );
	// End of thread, teardown any scratch memory used
	Builder_FreeScratch();
	return 0;
}
#elif defined( __linux__ )
static void *Builder_CompileJobThreadProc( void *param ) {
	Builder_RunCompileJobPool( (builderCompileJobPool_t *) param );
		// End of thread, teardown any scratch memory used
	Builder_FreeScratch();
	return NULL;
}
#endif

#if defined( _WIN32 )
static bool Builder_CreateJobThread( LPTHREAD_START_ROUTINE proc, void *args, builderThread_t *outThread ) {
	HANDLE thread = CreateThread( NULL, 0, proc, args, 0, NULL );

	if ( !thread ) {
		Builder_Error( "Failed to create compile worker thread: 0x%X\n", GetLastError() );
		return false;
	}

	*outThread = thread;

	return true;
#elif defined( __linux__ )
static bool Builder_CreateJobThread( void *(proc)(void *), void *args, builderThread_t *outThread ) {
	pthread_t thread;

	int result = pthread_create( &thread, NULL, proc, args );

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

typedef struct compileDependency_t {
	const char *dependency;
	uint64_t 	dependencyLength;
	uint64_t 	writeTime;
} compileDependency_t;

typedef struct compileDependencyArray_t {
	uint32_t				count;
	uint32_t				capacity;
	compileDependency_t	   *dependencies;
} compileDependencyArray_t;

typedef struct builderDependencyJobInfo_t {
	const BuilderOptions   *options;
	const bool				useMSVC;
	const char			   *baseCompilerArgs;
	const uint32_t			packetsPerSlice;
	const uint32_t			leftoverPackets;
	const compilePacket_t  *packetsBegin;

	const char			  **outCompileCommands;
	builderAtomic32_t		needsCompileCommandCount;
	builderAtomic32_t		nextDependencySliceIndex;
} builderDependencyJobInfo_t;

static bool Builder_DependencyArrayGetLastWriteTime( const char *dependency, const compileDependencyArray_t *dependencyArray, uint64_t *outWriteTime ) {
	for ( uint32_t dependencyIndex = 0; dependencyIndex < dependencyArray->count; ++dependencyIndex ) {
		const compileDependency_t *compileDependency = &dependencyArray->dependencies[dependencyIndex];
		if ( strncmp( dependency, compileDependency->dependency, compileDependency->dependencyLength ) == 0 ) {
			*outWriteTime = compileDependency->writeTime;
			return true;
		}
	}
	return false;
}

static void Builder_AddTodependencyArray( arena_t *dependencyArena, compileDependencyArray_t *dependencyArray, const char *dependency, uint64_t writeTime) {
	if ( dependencyArray->capacity == dependencyArray->count ) {
		Builder_ArenaRealloc( dependencyArena, dependencyArray->dependencies, compileDependency_t, dependencyArray->capacity, dependencyArray->capacity * 2 );
		dependencyArray->capacity *= 2;
	}

	dependencyArray->dependencies[dependencyArray->count++] = (compileDependency_t) {
		.dependency = dependency,
		.dependencyLength = strnlen( dependency, 512 ),
		.writeTime = writeTime
	};
}

static bool Builder_AreDependenciesNewer( arena_t *dependencyArena, const BuilderOptions *options, bool useMSVC, compileDependencyArray_t *dependencyArray, char **compilerOutput, const compilePacket_t *toCheck ) {
	bool foundNewerDependency = false;
	char *current;

	if ( useMSVC ) {
#if defined( _WIN32 )
		// skip first line 
		current = strchr( *compilerOutput, '\n' );

		const char *includeDependencyPrefix = "Note: including file: ";
		const uint64_t includeDependencyPrefixLength = strlen( includeDependencyPrefix );

		while ( *current ) {
			char *dependencyStart = current;
			dependencyStart += includeDependencyPrefixLength;

			while ( *dependencyStart == ' ' ) {
				dependencyStart += 1;
			}

			// get end of the filename
			char *dependencyEnd = strchr( dependencyStart, '\n' );
			if ( !dependencyEnd ) dependencyEnd = strchr( dependencyStart, '\0' );
			BUILDER_ASSERT( dependencyEnd );

			if ( *( dependencyEnd - 1 ) == '\r' ) {
				dependencyEnd -= 1;
			}

			if ( !foundNewerDependency ) {
				// get the substring we actually need
				uint64_t dependencyFilenameLength = ( (uint64_t) dependencyEnd ) - ( (uint64_t) dependencyStart );
				char *dependencyFilename = Builder_FormatString( dependencyArena, "%.*s", dependencyFilenameLength, dependencyStart );
				for ( uint64_t i = 0; i < dependencyFilenameLength; ++i ) {
					if ( dependencyFilename[i] == '\\' && dependencyFilename[i + 1] == ' ' ) {
						memmove( dependencyFilename + i, dependencyFilename + i + 1, dependencyFilenameLength - i ); // - 1 (count) + 1 '\0'
						dependencyFilenameLength--;
					}
				}
	
				uint64_t lastWriteTime;
				if ( Builder_DependencyArrayGetLastWriteTime( dependencyFilename, dependencyArray, &lastWriteTime ) ) {
					foundNewerDependency = lastWriteTime > toCheck->objectWriteTime;
					Builder_LogVerbose( options, "Found %s's newer? (%s) dependency via dependency array.\n", toCheck->sourceFile, foundNewerDependency ? "true" : "false", dependencyFilename );
				} else if ( Builder_GetFileLastWriteTime( dependencyFilename, &lastWriteTime ) ) {
					foundNewerDependency = lastWriteTime > toCheck->objectWriteTime;
					Builder_LogVerbose( options, "Found %s's newer? (%s) dependency %s, last write time = %llu\n", toCheck->sourceFile, foundNewerDependency ? "true" : "false", dependencyFilename, lastWriteTime );

					Builder_AddTodependencyArray( dependencyArena, dependencyArray, dependencyFilename, lastWriteTime );
				} else {
					Builder_Error( "Couldn't get last write time for dependency: %s\n", dependencyFilename );
				}
			}

			if ( *dependencyEnd == '\0' ) {
				break;
			}

			// /r/n
			current = dependencyEnd + 2;
			if ( !Builder_StringStartsWith( current, includeDependencyPrefix ) ) {
				break;
			}
		}
#endif
	} else {
		// .d files start with the name of the binary followed by a colon
		// so skip past that first
		current = strchr( *compilerOutput, ':' );
		BUILDER_ASSERT( current );
		current += 1;	// skip past the colon
		current += 1;	// skip past the following whitespace
	
		bool firstDependency = true; // first is the source file we already checked
		while ( *current ) {
			// get start of the filename
			char *dependencyStart = current;
	
			while ( *dependencyStart == ' ' ) {
				dependencyStart += 1;
			}
	
			// get end of the filename
			char *dependencyEnd = NULL;
			// filenames are separated by either new line or space
			if ( !dependencyEnd ) dependencyEnd = strchr( dependencyStart, ' ' );
			if ( !dependencyEnd ) dependencyEnd = strchr( dependencyStart, '\n' );
			BUILDER_ASSERT( dependencyEnd );
			// paths can have spaces in them, but they are preceded by a single backslash (\)
			// so if we find a space but it has a single backslash just before it then keep searching for a space
			while ( dependencyEnd && ( *( dependencyEnd - 1 ) == '\\' ) ) {
				dependencyEnd = strchr( dependencyEnd + 1, ' ' );
			}
	
			if ( !dependencyEnd ) {
				break;
			}
	
			// we have found the start of the next file's dependencies
			bool finalDependency = *( dependencyEnd - 1 ) == ':';
			if ( finalDependency ) {
				dependencyEnd = strchr( dependencyStart, '\n' );
			}
	
			if ( *( dependencyEnd - 1 ) == '\r' ) {
				dependencyEnd -= 1;
			}
	
			if ( !foundNewerDependency && !firstDependency ) {
				// get the substring we actually need
				uint64_t dependencyFilenameLength = ( (uint64_t) dependencyEnd ) - ( (uint64_t) dependencyStart );
				char *dependencyFilename = Builder_FormatString( dependencyArena, "%.*s", dependencyFilenameLength, dependencyStart );
				for ( uint64_t i = 0; i < dependencyFilenameLength; ++i ) {
					if ( dependencyFilename[i] == '\\' && dependencyFilename[i + 1] == ' ' ) {
						memmove( dependencyFilename + i, dependencyFilename + i + 1, dependencyFilenameLength - i ); // - 1 (count) + 1 '\0'
						dependencyFilenameLength--;
					}
				}
	
				uint64_t lastWriteTime;
				if ( Builder_DependencyArrayGetLastWriteTime( dependencyFilename, dependencyArray, &lastWriteTime ) ) {
					foundNewerDependency = lastWriteTime > toCheck->objectWriteTime;
					Builder_LogVerbose( options, "Found %s's new? (%s) dependency [via array], %s, last write time = %llu\n", toCheck->sourceFile, foundNewerDependency ? "true" : "false", dependencyFilename, lastWriteTime);
				} else if ( Builder_GetFileLastWriteTime( dependencyFilename, &lastWriteTime ) ) {
					foundNewerDependency = lastWriteTime > toCheck->objectWriteTime;
					Builder_LogVerbose( options, "Found %s's new? (%s) dependency %s, last write time = %llu\n", toCheck->sourceFile, foundNewerDependency ? "true" : "false", dependencyFilename, lastWriteTime );

					Builder_AddTodependencyArray( dependencyArena, dependencyArray, dependencyFilename, lastWriteTime );
				} else {
					Builder_Error( "Couldn't get last write time for dependency: %s\n", dependencyFilename );
				}
			} else {
				firstDependency = false;
			}
	
			current = dependencyEnd + 1;
	
			while ( *current == '\\' ) {
				current += 1;
			}
	
			if ( *current == '\r' ) {
				current += 1;
			}
	
			if ( *current == '\n' ) {
				current += 1;
			}
	
			if ( finalDependency ) {
				break;
			}
		}
	}

	*compilerOutput = current;
	return foundNewerDependency;
}

static void Builder_CheckDependencies( builderDependencyJobInfo_t *jobInfo, uint32_t sliceIndex ) {
	scratch_t scratch = Builder_GetScratch( NULL );

	stringBuilder_t argsBuilder = {0};
	StringBuilder_Appendf( scratch.arena, &argsBuilder, "%s ", jobInfo->baseCompilerArgs );

	uint32_t packetOffset = jobInfo->packetsPerSlice * sliceIndex;
	uint32_t numToCheck = jobInfo->packetsPerSlice;
	if ( sliceIndex < jobInfo->leftoverPackets ) {
		numToCheck += 1;
		packetOffset += sliceIndex; // -1 + 1 (all the slices before us also had +1 for leftover)
	} else {
		packetOffset += jobInfo->leftoverPackets;
	}

	const compilePacket_t *relevantPackets = jobInfo->packetsBegin + packetOffset;
	for ( uint32_t packetIndex = 0; packetIndex < numToCheck; ++packetIndex ) {
		StringBuilder_Appendf( scratch.arena, &argsBuilder, "%s ", relevantPackets[packetIndex].sourceFile );
	}

	const char *args = StringBuilder_ToString( scratch.arena, &argsBuilder );

	Builder_LogVerbose( jobInfo->options, "Dependency check command: %s\n", args );
	
	char *dependencyInfo = NULL;
	int32_t dependencyCheckResult = Builder_RunProcess( scratch.arena, args, true, &dependencyInfo );

	if ( dependencyCheckResult != 0 ) {
		Builder_Warning( "Dependency check invocation returned %i. Adding all %u files to compile list.\n", dependencyCheckResult, numToCheck );

		for ( uint32_t packetIndex = 0; packetIndex < numToCheck; ++packetIndex ) {
			// TODO: AK: 21/08/2026: Bad sharing here probably?
			uint32_t compileCommandIndex = Builder_AtomicIncrement( &jobInfo->needsCompileCommandCount ) - 1;
			jobInfo->outCompileCommands[compileCommandIndex] = relevantPackets[packetIndex].compileCommand;
		}
	} else {
		Builder_LogVerbose( jobInfo->options, "Dependency info acquired:\n%s\n", dependencyInfo );

		// TODO: AK: 21/08/2026: this should be a hash map
		static const uint32_t dependenciesCapcity = 16;
		compileDependencyArray_t dependencyArray = {
			.count = 0,
			.capacity = dependenciesCapcity,
			.dependencies = Builder_ArenaAlloc( scratch.arena, compileDependency_t, dependenciesCapcity )
		};

		for ( uint32_t packetIndex = 0; packetIndex < numToCheck; ++packetIndex ) {
			if ( Builder_AreDependenciesNewer( scratch.arena, jobInfo->options, jobInfo->useMSVC, &dependencyArray, &dependencyInfo, &relevantPackets[packetIndex]) ) {
				// TODO: AK: 21/08/2026: Bad sharing here probably?
				uint32_t compileCommandIndex = Builder_AtomicIncrement( &jobInfo->needsCompileCommandCount ) - 1;
				jobInfo->outCompileCommands[compileCommandIndex] = relevantPackets[packetIndex].compileCommand;
			}
		}
	}

	Builder_RewindScratch( &scratch );
}

static void Builder_RunDependencyJob( builderDependencyJobInfo_t *jobInfo ) {
	uint32_t dependencySliceIndex = Builder_AtomicIncrement( &jobInfo->nextDependencySliceIndex ) - 1;

	Builder_CheckDependencies( jobInfo, dependencySliceIndex );
}

#if defined( _WIN32 )
static DWORD WINAPI Builder_DependencyJobThreadProc( LPVOID param ) {
	Builder_RunDependencyJob( (builderDependencyJobInfo_t *) param );
	// End of thread, teardown any scratch memory used
	Builder_FreeScratch();
	return 0;
}
#elif defined( __linux__ )
static void *Builder_DependencyJobThreadProc( void *param ) {
	Builder_RunDependencyJob( (builderDependencyJobInfo_t *) param );
		// End of thread, teardown any scratch memory used
	Builder_FreeScratch();
	return NULL;
}
#endif

int Build( BuilderOptions *options, int argc, char **argv ) {
	double totalTimeStart = Builder_TimeMS();

	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	for ( int argIndex = 0; argIndex < argc; argIndex++ ) {
		if ( Builder_StringStartsWith( argv[argIndex], ARG_HELP_SHORT ) || Builder_StringStartsWith( argv[argIndex], ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( Builder_StringStartsWith( argv[argIndex], ARG_VERBOSE_SHORT ) || Builder_StringStartsWith( argv[argIndex], ARG_VERBOSE_LONG ) ) {
			options->verboseLogging = true;
		}
	}

	// Names can only be checked here.  A config comes out of CreateBuildConfig() blank and gets filled in afterwards,
	// so this is the first point at which every config actually has the name it's going to be built under.
	for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
		for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
			BuildConfig *config = chunk->items[configIndex];

			if ( !config->name || !config->name[0] ) {
				Builder_Error( "One of your BuildConfigs has no name.  Every config needs one - it's what \"" ARG_CONFIG "\" matches against and what the build log calls it.\n" );
				return 1;
			}

			// only has to look at the configs after this one, since anything before it already compared against this
			for ( buildConfigPtrChunk_t *otherChunk = chunk; otherChunk; otherChunk = otherChunk->next ) {
				uint32_t firstOtherIndex = ( otherChunk == chunk ) ? configIndex + 1 : 0;

				for ( uint32_t otherIndex = firstOtherIndex; otherIndex < otherChunk->count; otherIndex++ ) {
					if ( Builder_StringEquals( otherChunk->items[otherIndex]->name, config->name ) ) {
						Builder_Error( "There is more than one BuildConfig called \"%s\".  Config names have to be unique, otherwise \"" ARG_CONFIG "%s\" has no way of telling them apart.\n", config->name, config->name );
						return 1;
					}
				}
			}
		}
	}

	// validate cmd line args
	const char *nameOfConfigToBuild = Builder_GetNameOfConfigToBuild( options, argc, argv );
	BuildConfig *targetConfig = NULL;
	{
		if ( options->configs.count == 0 ) {
			Builder_Error( "No BuildConfig was registered.  You must call CreateBuildConfig() at least once.\n" );
			return 1;
		} else if ( options->configs.count > 1 && !nameOfConfigToBuild ) {
			Builder_Error( "You have more than 1 BuildConfig defined, but you never told me which you wanted me to build via \"" ARG_CONFIG "\".  You need to tell me what config you want me to build, or set a default via BuilderOptions::defaultConfig.\n" );
			return 1;
		}

		if ( nameOfConfigToBuild ) {
			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk && !targetConfig; chunk = chunk->next ) {
				for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
					if ( Builder_StringEquals( chunk->items[configIndex]->name, nameOfConfigToBuild ) ) {
						targetConfig = chunk->items[configIndex];
						break;
					}
				}
			}

			if ( !targetConfig ) {
				Builder_Error( "No BuildConfig found with the name \"%s\".\n", nameOfConfigToBuild );
				return 1;
			}
		} else {
			// only one config was ever registered, so there's nothing to be ambiguous about
			targetConfig = options->configs.head->items[0];
		}
	}

	// toolchain and compiler paths are used right through to the link step.
	// this deliberately excludes the config arena: OnPreBuild/OnPostBuild callbacks run inside the loop below, so a
	// callback calling Add*()/Set*() allocates while this scratch is open.  If the two shared an arena, the per-config
	// rewind would take whatever the callback added with it and leave the config pointing into reusable memory.
	scratch_t buildScratch = Builder_GetScratch( Builder_GetConfigArena() );

	// only query for windows SDK and MSVC installations after verifying cmd line args and
#ifdef _WIN32
	builderWindowsSDKInstall_t windowsSDKInstall = { 0 };
	if ( !Builder_GetWindowsSDKInstall( buildScratch.arena, &windowsSDKInstall ) ) {
		return 1;
	}

	builderMSVCInstall_t msvcInstall = { 0 };
	if ( !Builder_GetMSVCInstall( buildScratch.arena, &msvcInstall ) ) {
		return 1;
	}
#endif

	if ( options->compilerPath && options->compilerPath[0] ) {
		Builder_LogVerbose( options, "Found override compiler backend \"%s\" from BuilderOptions::compilerPath.\n", options->compilerPath );
	}

	const char *compilerPath = ( options->compilerPath && options->compilerPath[0] ) ? options->compilerPath : "clang";

#if defined( _WIN32 )
	bool useMSVC = Builder_StringEquals( compilerPath, "cl" ) || Builder_StringEquals( compilerPath, "cl.exe" );

	if ( useMSVC ) {
		compilerPath = Builder_FormatString( buildScratch.arena, "%s\\bin\\Hostx64\\x64\\cl.exe", msvcInstall.rootFolder );
	}
#else
	bool useMSVC = false;
#endif

	char *compilerVersionString = NULL;
	if ( useMSVC ) {
#if defined( _WIN32 )
		compilerVersionString = Builder_FormatString( buildScratch.arena, "%d.%d.%d", msvcInstall.version.v0, msvcInstall.version.v1, msvcInstall.version.v2 );
#endif
	} else {
		char *versionCmd = Builder_FormatString( buildScratch.arena, "\"%s\" --version", compilerPath );
		char *versionOutput = NULL;

		Builder_RunProcess( buildScratch.arena, versionCmd, false, &versionOutput );
		compilerVersionString = Builder_ExtractVersionNumber( buildScratch.arena, versionOutput );
	}

	if ( options->compilerVersion && options->compilerVersion[0] ) {
		if ( !Builder_StringEquals( compilerVersionString, options->compilerVersion ) ) {
			Builder_Warning( "You are using compiler version \"%s\", but \"%s\" was set as BuilderOptions::compilerVersion.  I will continue building anyway, but you may not get what you expect.\n", compilerVersionString, options->compilerVersion );
		}
	}

	// the walk happens here rather than as configs are created because dependencies get attached to a config after
	// CreateBuildConfig() has handed it over, so this is the first point the graph is complete.
	// its lists go on buildScratch above the toolchain paths but below the rewind spot the loop takes for each config,
	// so the per-config rewind can't reach back and take them with it
	ConfigPtrList ancestry = { 0 };
	ConfigPtrList configsToBuild = { 0 };

	Builder_CollectConfigsToBuild( buildScratch.arena, targetConfig, &ancestry, &configsToBuild );

	double totalCompileTimeMS = 0.0;
	double totalLinkTimeMS = 0.0;

	for ( buildConfigPtrChunk_t *chunk = configsToBuild.head; chunk; chunk = chunk->next ) {
		for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
			BuildConfig *config = chunk->items[configIndex];

			// nothing this config allocates is wanted by the next one - the toolchain paths it reads were put on
			// buildScratch before the loop, so they sit below this and the rewind can't reach them
			arenaRewindSpot_t configStart = Builder_ArenaTell( buildScratch.arena );

			double compileTimeMS = 0.0;
			double linkTimeMS = 0.0;

			if ( config->OnPreBuild ) {
				Builder_LogVerbose( options, "Found a OnPreBuild() func ptr for BuildConfig: \"%s\".  Running...\n", config->name ? config->name : "" );

				config->OnPreBuild( config );
			}

			// build the config
			{
				printf( "Building config \"%s\":\n", config->name );

				const char *intermediateFolder = options->intermediateFolder;
				if ( !intermediateFolder || *intermediateFolder == '\0' ) {
					intermediateFolder = "intermediate";
				}

				if ( intermediateFolder && !Builder_CreateFolderIfItDoesntExist( intermediateFolder ) ) {
					Builder_Error( "Failed to create the intermediate folder \"%s\".\n", intermediateFolder );
					return 1;
				}

				// glob step - flattened into one array because the compile job pool indexes into it by job number
				StringList globList = Builder_GlobFiles( buildScratch.arena, &config->sourceFiles, options );

				uint32_t compilePacketCount = globList.count;
				compilePacket_t *compilePackets = NULL;
				
				uint32_t numCPUCores = Builder_GetNumCPUCores();

				builderCompileContext_t compileContext = {
					.config				= config,
					.compilerPath		= compilerPath,
					.useMSVC			= useMSVC,
#if defined( _WIN32 )
					.msvcInstall		= &msvcInstall,
					.windowsSDKInstall	= &windowsSDKInstall,
#endif
				};

				uint32_t needsCompileCommandCount = 0;
				const char **needsCompileCommands = NULL;

				if ( compilePacketCount > 0 ) {
					compilePackets = Builder_ArenaAlloc(buildScratch.arena, compilePacket_t, compilePacketCount );
					needsCompileCommands = Builder_ArenaAlloc(buildScratch.arena, const char *, compilePacketCount );

					scratch_t scratch = Builder_GetScratch( buildScratch.arena );

					stringBuilder_t compileCommand = Builder_CreateCompilationCommand( scratch.arena, &compileContext );
					if ( compileCommand.head == compileCommand.tail ) {
						Builder_Error( "Failed to create compilation command!\n");
						exit(1);
					}
					arenaRewindSpot_t commandRewind = Builder_ArenaTell( scratch.arena );

					uint32_t written = 0;

					for ( builderStringChunk_t *chunk = globList.head; chunk; chunk = chunk->next ) {
						for ( uint32_t globbedFileIndex = 0; globbedFileIndex < chunk->count; globbedFileIndex++ ) {
							const char *sourceFile = chunk->items[globbedFileIndex];
							StringBuilder_Appendf( scratch.arena, &compileCommand, "%s ", sourceFile );

							// we do the hash of the source file here to grab the actual intermediate file
							{
								// TODO: AK: 21/08/2026: Do we want to ensure the way the source file presents is unified? Otherwise we get ./main.cpp != main.cpp
								arenaRewindSpot_t preIntermediateRewind = Builder_ArenaTell( scratch.arena );
								StringBuilder_Appendf( scratch.arena, &compileCommand, "%s", compilerVersionString );
								const char *stringForIntermediateHash = StringBuilder_ToString( scratch.arena, &compileCommand );
								compilePackets[written].intermediateFile = Builder_GetIntermediateFilePath( buildScratch.arena, intermediateFolder, stringForIntermediateHash, sourceFile );
								Builder_RewindArena( scratch.arena, &preIntermediateRewind );
							}

							if ( compileContext.useMSVC ) {
#if defined( _WIN32 )
								StringBuilder_Appendf( scratch.arena, &compileCommand, "/Fo" );
#endif
							} else {
								StringBuilder_Appendf( scratch.arena, &compileCommand, "-o " );
							}

							StringBuilder_Appendf( scratch.arena, &compileCommand, compilePackets[written].intermediateFile );

							compilePackets[written].compileCommand = StringBuilder_ToString( buildScratch.arena, &compileCommand );
							compilePackets[written].sourceFile = sourceFile;

							++written;
							Builder_RewindArena( scratch.arena, &commandRewind );
						}
					}

					// we do a separate pass over the data here but really we could amorphise this with the above loop
					// also at some point we might want to go wide	over multiple threads to do this
					if ( !options->forceRebuild ) {
						// iterate through, and swap with end if a file needs a deeper dependency check
						uint32_t needsDependencyCheck = 0;
						uint32_t packetIndex = 0;
						while ( packetIndex < ( compilePacketCount - needsDependencyCheck ) ) {
							const char *sourceFile = compilePackets[packetIndex].sourceFile;
							const char *objectFile = compilePackets[packetIndex].intermediateFile;

							uint64_t sourceWriteTime, objectWriteTime;
							if ( Builder_GetFileLastWriteTime( objectFile, &objectWriteTime) ) {
								if ( !Builder_GetFileLastWriteTime( sourceFile, &sourceWriteTime ) ) {
									Builder_Warning( "Couldn't stat source file '%s'.\n", sourceFile ); // so skip it
									packetIndex++;
									continue;
								}
								
								// object is newer - we must do the deeper checks
								if ( objectWriteTime > sourceWriteTime ) {
									const uint32_t uncheckedPacketOffset = compilePacketCount - 1 - (needsDependencyCheck++);
									
									compilePacket_t toSwap = compilePackets[packetIndex];
									toSwap.objectWriteTime = objectWriteTime;

									compilePackets[packetIndex] = compilePackets[uncheckedPacketOffset];
									compilePackets[uncheckedPacketOffset] = toSwap;
									continue;
								}
							}

							// add commands with no object file or newer source file
							needsCompileCommands[needsCompileCommandCount++] = compilePackets[packetIndex++].compileCommand;
						}
						
						if ( needsDependencyCheck > 0 ) {
							// TODO: AK: 21/08/2026: If we are spinning up threads for this do we just spin the threads up and never look back?
							uint32_t numWorkers = ( numCPUCores < needsDependencyCheck ) ? numCPUCores : needsDependencyCheck;
							uint32_t numAdditionalThreads = ( numWorkers > 1 ) ? numWorkers - 1 : 0;
							printf("%u source files need deeper checks across %u threads.\n", needsDependencyCheck, numWorkers);

							// we kept the compile command scratch for this very purpose
							// since we need all the defines at minimum to check dependencies
							// if we run into issues with this later we will have to somehow
							// create a subset of their commands (might be ugly due to additional compile commands)
							if ( compileContext.useMSVC ) {
#if defined( _WIN32 )
								StringBuilder_Appendf( scratch.arena, &compileCommand,  "/showIncludes /Zs" );
#endif
							} else {
								const char *dependencyFlag = config->excludeSystemHeaderDependencyChecking ? "-MM " : "-M ";
								StringBuilder_Appendf( scratch.arena, &compileCommand,  dependencyFlag );
								StringBuilder_Appendf( scratch.arena, &compileCommand, "-Wno-unused-command-line-argument" );
							}
						
							const char *baseArgs = StringBuilder_ToString( scratch.arena, &compileCommand );
							
							uint32_t packetsPerSlice = needsDependencyCheck / numWorkers;
							uint32_t leftoverPackets = needsDependencyCheck % numWorkers;

							const uint32_t packetOffset = compilePacketCount - needsDependencyCheck;

							builderDependencyJobInfo_t jobInfo = {
								.options = options,
								.useMSVC = compileContext.useMSVC,
								.baseCompilerArgs = baseArgs,
								.packetsPerSlice = packetsPerSlice,
								.leftoverPackets = leftoverPackets,
								.packetsBegin = compilePackets + packetOffset,
								.outCompileCommands = needsCompileCommands,
								.needsCompileCommandCount = needsCompileCommandCount
							};

							builderThread_t *additionalThreads = NULL;
							uint32_t numCreatedThreads = 0;

							if ( numAdditionalThreads > 0 ) {
								additionalThreads = Builder_ArenaAlloc( buildScratch.arena, builderThread_t, numAdditionalThreads );

								for ( uint32_t threadIndex = 0; threadIndex < numAdditionalThreads; threadIndex++ ) {
									if ( Builder_CreateJobThread( Builder_DependencyJobThreadProc, &jobInfo, &additionalThreads[numCreatedThreads] ) ) {
										numCreatedThreads++;
									}
								}
							}

							// the main thread pulls jobs from the same pool instead of just sitting idle waiting on the additional threads
							Builder_RunDependencyJob( &jobInfo );

							for ( uint32_t threadIndex = 0; threadIndex < numCreatedThreads; threadIndex++ ) {
								Builder_ThreadJoin( additionalThreads[threadIndex] );
							}
						}
					} else {
						// we compile everything then
						needsCompileCommandCount = compilePacketCount;
						for ( uint32_t packetIndex = 0; packetIndex < compilePacketCount; ++packetIndex ) {
							needsCompileCommands[packetIndex] = compilePackets[packetIndex].compileCommand;
						}
					}
					Builder_RewindScratch( &scratch );
				}

				// compilation step
				{
					const double compileTimeStart = Builder_TimeMS();

					if ( needsCompileCommandCount > 0 ) {
						// TODO: DM: 09/08/2026: is it OK to create and destroy a bunch of threads for each config?
						builderCompileJobPool_t pool = {
							.compileCommands		= needsCompileCommands,
							.compileCommandCount	= needsCompileCommandCount,
						};

						// only spin up additional threads once theres more than one file
						// limit the number of threads we spin up to no higher than the number of CPU cores we have
						uint32_t numWorkers = ( numCPUCores < needsCompileCommandCount ) ? numCPUCores : needsCompileCommandCount;
						uint32_t numAdditionalThreads = ( numWorkers > 1 ) ? numWorkers - 1 : 0;

						printf( "Compiling %u files across %u threads.\n", needsCompileCommandCount, numWorkers );
						printf( "          %u files were skipped.\n", compilePacketCount - needsCompileCommandCount );

						builderThread_t *additionalThreads = NULL;
						uint32_t numCreatedThreads = 0;

						if ( numAdditionalThreads > 0 ) {
							additionalThreads = Builder_ArenaAlloc( buildScratch.arena, builderThread_t, numAdditionalThreads );

							for ( uint32_t threadIndex = 0; threadIndex < numAdditionalThreads; threadIndex++ ) {
								if ( Builder_CreateJobThread( Builder_CompileJobThreadProc, &pool, &additionalThreads[numCreatedThreads] ) ) {
									numCreatedThreads++;
								}
							}
						}

						// the main thread pulls jobs from the same pool instead of just sitting idle waiting on the additional threads
						Builder_RunCompileJobPool( &pool );

						for ( uint32_t threadIndex = 0; threadIndex < numCreatedThreads; threadIndex++ ) {
							Builder_ThreadJoin( additionalThreads[threadIndex] );
						}

						if ( pool.numFailed > 0 ) {
							Builder_Error( "Build failed.\n" );
							Builder_RewindScratch( &buildScratch );
							return 1;
						}
					} else {
							printf( "Skipping compilation of all %u files.\n", compilePacketCount - needsCompileCommandCount );
					}

						compileTimeMS = Builder_TimeMS() - compileTimeStart;
					}

					// link step
					{
						double linkTimeStart = Builder_TimeMS();

						if ( config->binaryFolder && !Builder_CreateFolderIfItDoesntExist( config->binaryFolder ) ) {
							Builder_Error( "Failed to create the binary folder \"%s\".\n", config->binaryFolder );
							Builder_RewindScratch( &buildScratch );
							return 1;
						}

						stringBuilder_t linkerArgs = { 0 };
						const char *binaryPath = Builder_GetBinaryPath( buildScratch.arena, config );
						// TODO: AK: 21/08/2026: We probably should just query if the file exists instead of using this function
						uint64_t binaryFileWriteTime;
						if ( needsCompileCommandCount > 0 || !Builder_GetFileLastWriteTime( binaryPath, &binaryFileWriteTime ) ) {				
							stringBuilder_t linkerArgs = {0};
#if defined( _WIN32 )
							if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "\"%s\\bin\\Hostx64\\x64\\lib.exe\" ", msvcInstall.rootFolder );
							} else {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "\"%s\\bin\\Hostx64\\x64\\link.exe\" ", msvcInstall.rootFolder );
							}

							if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/DLL " );
							}

							if ( !config->removeSymbols ) {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/DEBUG " );
							}

							StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/OUT:" );
							StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", binaryPath );

							StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/LIBPATH:\"%s\" ", msvcInstall.libPath );
							StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/LIBPATH:\"%s\" ", windowsSDKInstall.umLibPath );
							StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/LIBPATH:\"%s\" ", windowsSDKInstall.ucrtLibPath );

							// we always have to link all files
							for ( uint32_t intermediateIndex = 0; intermediateIndex < compilePacketCount; ++intermediateIndex ) {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", compilePackets[intermediateIndex].intermediateFile );
							}

							for ( builderStringChunk_t *chunk = config->additionalLibPaths.head; chunk; chunk = chunk->next ) {
								for ( uint32_t libPathIndex = 0; libPathIndex < chunk->count; libPathIndex++ ) {
									StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "/LIBPATH:\"%s\" ", chunk->items[libPathIndex] );
								}
							}

							for ( builderStringChunk_t *chunk = config->additionalLibs.head; chunk; chunk = chunk->next ) {
								for ( uint32_t libIndex = 0; libIndex < chunk->count; libIndex++ ) {
									const char *additionalLib = chunk->items[libIndex];

									// callers sometimes already include the ".lib" extension themselves - don't double it up
									size_t libNameLen = strlen( additionalLib );
									bool alreadyHasExtension = libNameLen >= 4 && _stricmp( additionalLib + libNameLen - 4, ".lib" ) == 0;

									if ( alreadyHasExtension ) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", additionalLib );
									} else {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s.lib ", additionalLib );
									}
								}
							}
							
							if ( config->binaryType != BINARY_TYPE_STATIC_LIBRARY ) {
								// clang doesn't embed /DEFAULTLIB directives the way cl.exe does
								// so link.exe has no idea which CRT/SDK libs to pull in unless we name them ourselves
								if ( config->useDynamicRuntimeOnWindows ) {
									if (compileContext.debugDefineSet) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "msvcrtd.lib msvcprtd.lib vcruntimed.lib ucrtd.lib kernel32.lib " );
									} else {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "msvcrt.lib msvcprt.lib vcruntime.lib ucrt.lib kernel32.lib " );
									}
								} else {
									if (compileContext.debugDefineSet) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "libcmtd.lib libcpmtd.lib libvcruntimed.lib libucrtd.lib kernel32.lib " );
									} else {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "libcmt.lib libcpmt.lib libvcruntime.lib libucrt.lib kernel32.lib " );
									}
								}
							}

							for ( builderStringChunk_t *chunk = config->additionalLinkerArguments.head; chunk; chunk = chunk->next ) {
								for ( uint32_t argumentIndex = 0; argumentIndex < chunk->count; argumentIndex++ ) {
									StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", chunk->items[argumentIndex] );
								}
							}
#elif defined( __linux__ )
							if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "ar rcs " );
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", binaryPath );

								// we always have to link all files
								for ( uint32_t intermediateIndex = 0; intermediateIndex < compilePacketCount; ++intermediateIndex ) {
									StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", compilePackets[intermediateIndex].intermediateFile );
								}
							} else {
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "\"%s\" ", compilerPath );

								if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
									StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "-shared " );
								}

								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "-o " );
								StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", binaryPath );

								// we always have to link all files
								for ( uint32_t intermediateIndex = 0; intermediateIndex < compilePacketCount; ++intermediateIndex ) {
									StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", compilePackets[intermediateIndex].intermediateFile );
								}

								for ( builderStringChunk_t *chunk = config->additionalLibPaths.head; chunk; chunk = chunk->next ) {
									for ( uint32_t libPathIndex = 0; libPathIndex < chunk->count; libPathIndex++ ) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "-L%s ", chunk->items[libPathIndex] );
									}
								}

								for ( builderStringChunk_t *chunk = config->additionalLibs.head; chunk; chunk = chunk->next ) {
									for ( uint32_t libIndex = 0; libIndex < chunk->count; libIndex++ ) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "-l%s ", chunk->items[libIndex] );
									}
								}

								for ( builderStringChunk_t *chunk = config->additionalLinkerArguments.head; chunk; chunk = chunk->next ) {
									for ( uint32_t argumentIndex = 0; argumentIndex < chunk->count; argumentIndex++ ) {
										StringBuilder_Appendf( buildScratch.arena, &linkerArgs, "%s ", chunk->items[argumentIndex] );
									}
								}
							}
#endif
							char *args = StringBuilder_ToString( buildScratch.arena, &linkerArgs );

							printf( "%s\n", args );

							int32_t linkResult = Builder_RunProcess( NULL, args, false, NULL );

							if ( linkResult != 0 ) {
								Builder_Error( "Link failed.\n" );
								Builder_RewindScratch( &buildScratch );
								return 1;
							}

							linkTimeMS = Builder_TimeMS() - linkTimeStart;
						}
					}

				if ( config->OnPostBuild ) {
					Builder_LogVerbose( options, "Found a OnPostBuild() func ptr for BuildConfig: \"%s\".  Running...\n", config->name ? config->name : "" );

					config->OnPostBuild( config );
				}

				printf( "Finished config \"%s\":\n", config->name ? config->name : "" );
				printf( "    Compile : %f ms\n", compileTimeMS );
				printf( "    Link    : %f ms\n", linkTimeMS );
				printf( "\n" );

				totalCompileTimeMS += compileTimeMS;
				totalLinkTimeMS += linkTimeMS;

				Builder_RewindArena( buildScratch.arena, &configStart );
			}
		}
	}

	// build summary
	{
		printf( "Finished:\n" );
		printf( "    Compile : %f ms\n", totalCompileTimeMS );
		printf( "    Link    : %f ms\n", totalLinkTimeMS );
		printf( "    Total   : %f ms\n", Builder_TimeMS() - totalTimeStart );
	}

	Builder_RewindScratch( &buildScratch );
	return 0;
}

#endif // BUILDER_IMPLEMENTATION

#pragma clang diagnostic pop

#ifdef __cplusplus
}
#endif
