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

#include "builder_local.h"

#include "win_support.h"
#include "math.h"
#include "linear_allocator.h"
#include "array.inl"
#include "int_types.h"
#include "string.h"
#include "string_builder.h"
#include "paths.h"
#include "subprocess.h"
#include "file.h"
#include "typecast.h"
#include "temp_storage.h"
#include "hash.h"
#include "timer.h"
#include "library.h"
#include "hashmap.h"
#include "defer.h"
#include "thread.h"
#include "os.h"

#ifdef _WIN64
#include <Shlwapi.h>
#elif defined(__linux__)
#include <errno.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

/*
=============================================================================

	Builder

	by Dan Moody

=============================================================================
*/

enum {
	BUILDER_VERSION_MAJOR	= 0,
	BUILDER_VERSION_MINOR	= 14,
	BUILDER_VERSION_PATCH	= 0,
};

enum buildResult_t {
	BUILD_RESULT_SUCCESS	= 0,
	BUILD_RESULT_FAILED,
	BUILD_RESULT_SKIPPED
};

#define SET_BUILDER_OPTIONS_FUNC_NAME	"SetBuilderOptions"
#define PRE_BUILD_FUNC_NAME				"OnPreBuild"
#define POST_BUILD_FUNC_NAME			"OnPostBuild"

#ifdef _DEBUG
	#define QUIT_ERROR() \
		DebugBreak(); \
		return 1
#else
	#define QUIT_ERROR() \
		return 1
#endif

bool8 g_verbose = false;


u64 GetLastFileWriteTime( const char *filename ) {
	u64 lastWriteTime = 0;
	if ( !FS_GetFileLastWriteTime( filename, &lastWriteTime ) ) {
		return U64_MAX;
	}

	return lastWriteTime;
}

const char *GetFileExtensionFromBinaryType( const BinaryType type ) {
#ifdef _WIN32
	switch ( type ) {
		case BINARY_TYPE_EXE:				return ".exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".lib";
	}
#elif defined( __linux__ )
	switch ( type ) {
		case BINARY_TYPE_EXE:				return "";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".so";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".a";
	}
#else
#error Unrecognised paltform.
#endif

	Assert( false );

	return "ERROR";
}

bool8 FileIsSourceFile( const char *filename ) {
	static const char *fileExtensions[] = {
		".cpp",
		".cxx",
		".cc",
		".c",
	};

	For ( u64, extensionIndex, 0, COUNT_OF( fileExtensions ) ) {
		if ( String_EndsWith( filename, fileExtensions[extensionIndex] ) ) {
			return true;
		}
	}

	return false;
}

bool8 FileIsHeaderFile( const char *filename ) {
	static const char *fileExtensions[] = {
		".h",
		".hpp",
	};

	For ( u64, extensionIndex, 0, COUNT_OF( fileExtensions ) ) {
		if ( String_EndsWith( filename, fileExtensions[extensionIndex] ) ) {
			return true;
		}
	}

	return false;
}

static const char *BuildConfig_ToString( const BuildConfig *config, linearAllocator_t *allocator ) {
	auto LanguageVersionToString = []( const LanguageVersion version ) -> const char * {
		switch ( version ) {
			case LANGUAGE_VERSION_UNSET:	return "LANGUAGE_VERSION_UNSET";
			case LANGUAGE_VERSION_C89:		return "LANGUAGE_VERSION_C89";
			case LANGUAGE_VERSION_C99:		return "LANGUAGE_VERSION_C99";
			case LANGUAGE_VERSION_C11:		return "LANGUAGE_VERSION_C11";
			case LANGUAGE_VERSION_C17:		return "LANGUAGE_VERSION_C17";
			case LANGUAGE_VERSION_C23:		return "LANGUAGE_VERSION_C23";
			case LANGUAGE_VERSION_CPP11:	return "LANGUAGE_VERSION_CPP11";
			case LANGUAGE_VERSION_CPP14:	return "LANGUAGE_VERSION_CPP14";
			case LANGUAGE_VERSION_CPP17:	return "LANGUAGE_VERSION_CPP17";
			case LANGUAGE_VERSION_CPP20:	return "LANGUAGE_VERSION_CPP20";
			case LANGUAGE_VERSION_CPP23:	return "LANGUAGE_VERSION_CPP23";
		}
	};

	auto BinaryTypeToString = []( const BinaryType type ) -> const char * {
		switch ( type ) {
			case BINARY_TYPE_EXE:				return "BINARY_TYPE_EXE";
			case BINARY_TYPE_DYNAMIC_LIBRARY:	return "BINARY_TYPE_DYNAMIC_LIBRARY";
			case BINARY_TYPE_STATIC_LIBRARY:	return "BINARY_TYPE_STATIC_LIBRARY";
		}
	};

	auto OptimizationLevelToString = []( OptimizationLevel level ) -> const char * {
		switch ( level ) {
			case OPTIMIZATION_LEVEL_O0: return "OPTIMIZATION_LEVEL_O0";
			case OPTIMIZATION_LEVEL_O1: return "OPTIMIZATION_LEVEL_O1";
			case OPTIMIZATION_LEVEL_O2: return "OPTIMIZATION_LEVEL_O2";
			case OPTIMIZATION_LEVEL_O3: return "OPTIMIZATION_LEVEL_O3";
		}
	};

	stringBuilder_t builder = SB_Create( allocator );

	auto PrintCStringArray = [&builder]( const char *name, const std::vector<const char *> &array ) {
		SB_Appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			SB_Appendf( &builder, "%s", array[i] );

			if ( i < array.size() - 1 ) {
				SB_Appendf( &builder, ", " );
			}
		}
		SB_Appendf( &builder, " }\n" );
	};

	auto PrintSTDStringArray = [&builder]( const char *name, const std::vector<std::string> &array ) {
		SB_Appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			SB_Appendf( &builder, "%s", array[i].c_str() );

			if ( i < array.size() - 1 ) {
				SB_Appendf( &builder, ", " );
			}
		}
		SB_Appendf( &builder, " }\n" );
	};

	auto PrintField = [&builder]( const char *key, const char *value ) {
		SB_Appendf( &builder, "\t%s: %s\n", key, value );
	};

	SB_Appendf( &builder, "%s: {\n", config->name.c_str() );

	if ( config->dependsOn.size() > 0 ) {
		SB_Appendf( &builder, "\tdependsOn: { " );
		For ( u64, dependencyIndex, 0, config->dependsOn.size() ) {
			SB_Appendf( &builder, "%s", config->dependsOn[dependencyIndex].name.c_str() );

			if ( dependencyIndex < config->dependsOn.size() - 1 ) {
				SB_Appendf( &builder, ", " );
			}
		}
		SB_Appendf( &builder, " }\n" );
	}

	PrintSTDStringArray( "sourceFiles", config->sourceFiles );
	PrintSTDStringArray( "defines", config->defines );
	PrintSTDStringArray( "additionalIncludes", config->additionalIncludes );
	PrintSTDStringArray( "additionalLibPaths", config->additionalLibPaths );
	PrintSTDStringArray( "additionalLibs", config->additionalLibs );
	PrintSTDStringArray( "warningLevels", config->warningLevels );
	PrintSTDStringArray( "ignoreWarnings", config->ignoreWarnings );
	PrintSTDStringArray( "additionalCompilerArguments", config->additionalCompilerArguments );
	PrintSTDStringArray( "additionalLinkerArguments", config->additionalLinkerArguments );

	PrintField( "binaryName", config->binaryName.c_str() );
	PrintField( "binaryFolder", config->binaryFolder.c_str() );
	PrintField( "intermediateFolder", config->intermediateFolder.c_str() );
	PrintField( "languageVersion", LanguageVersionToString( config->languageVersion ) );
	PrintField( "binaryType", BinaryTypeToString( config->binaryType ) );
	PrintField( "optimizationLevel", OptimizationLevelToString( config->optimizationLevel ) );
	PrintField( "removeSymbols", config->removeSymbols ? "true" : "false" );
	PrintField( "removeFileExtension", config->removeFileExtension ? "true" : "false" );
	PrintField( "warningsAsErrors", config->warningsAsErrors ? "true" : "false" );

	// TODO(DM): 30/03/2026: how do we log OnPreBuild()/OnPostBuild() func ptrs?

	SB_Appendf( &builder, "}\n" );

	return SB_ToString( &builder );
}

const char *BuildConfig_GetFullBinaryName( const BuildConfig *config, linearAllocator_t *allocator ) {
	Assert( !config->binaryName.empty() );

	stringBuilder_t sb = SB_Create( allocator );

	if ( !config->binaryFolder.empty() ) {
		SB_Appendf( &sb, "%s%c", config->binaryFolder.c_str(), PATH_SEPARATOR );
	}

	SB_Appendf( &sb, "%s", config->binaryName.c_str() );

	if ( !config->removeFileExtension ) {
		SB_Appendf( &sb, "%s", GetFileExtensionFromBinaryType( config->binaryType ) );
	}

	return SB_ToString( &sb );
}

// TODO(DM): 31/03/2026: does this mean we want a verbose logging mode in Core?
void LogVerbose( const char *fmt, ... ) {
	if ( !g_verbose ) {
		return;
	}

	printf( "VERBOSE: " );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

s32 RunProc( array_t<const char *> *args, array_t<const char *> *environmentVariables, const procFlags_t procFlags, string_t *outStdout ) {
	Assert( args );
	Assert( args->data );
	Assert( args->count >= 1 );
	if ( outStdout ) {
		Assert( outStdout->data == NULL );
		Assert( outStdout->count == 0 );
	}

	if ( procFlags & PROC_FLAG_SHOW_ARGS ) {
		For ( u64, argIndex, 0, args->count ) {
			printf( "%s ", ( *args )[argIndex] );
		}
		printf( "\n" );
	}

	process_t *process = Proc_Create( Mem_GetTempStorage(), args, environmentVariables, PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	if ( !process ) {
		Error(
			"Failed to run process \"%s\".\n"
			"Is it definitely installed? Is it meant to be added to your PATH? Did you type the path correctly?\n"
			, ( *args )[0]
		);

		// DM: 20/07/2025: I'm not 100% sure that its totally ok to have -1 as our own special exit code to mean that the process couldnt be found
		// its totally possible for other processes to return -1 and have it mean something else
		// the interpretation of the exit code of the processes we run is the responsibility of the calling code and were probably making a lot of assumptions there
		return -1;
	}

	defer {
		Proc_Destroy( process );
		process = NULL;
	};

	// show stdout
	stringBuilder_t sb = SB_Create( Mem_GetTempStorage() );

	u64 bytesRead = 0;
	char buffer[1024] = {};
	while ( ( bytesRead = Proc_ReadStdout( process, buffer, COUNT_OF( buffer ) - 1 ) ) ) {
		buffer[bytesRead] = 0;

		SB_Appendf( &sb, "%s", buffer );

		if ( procFlags & PROC_FLAG_SHOW_STDOUT ) {
			printf( "%s", buffer );
		}
	}

	if ( outStdout ) {
		*outStdout = String_Set( SB_ToString( &sb ) );
	}

	s32 exitCode = Proc_Join( process );

	return exitCode;
}

bool8 WriteStringBuilderToFile( stringBuilder_t *stringBuilder, const char *filename ) {
	const char *msg = SB_ToString( stringBuilder );
	const u64 msgLength = strlen( msg );
	bool8 written = FS_WriteEntireFile( filename, msg, msgLength );

	if ( !written ) {
		s32 errorCode = GetLastErrorCode();
		Error( "Failed to write \"%s\": " ERROR_CODE_FORMAT ".\n", filename, errorCode );

		return false;
	}

	return true;
};

static s32 ShowUsage( const s32 exitCode ) {
	printf(
		"Builder.exe\n"
		"\n"
		"USAGE:\n"
		"    Builder.exe <file> [arguments] [custom arguments]\n"
		"\n"
		"Arguments:\n"
		"    " ARG_HELP_SHORT "|" ARG_HELP_LONG " (optional):\n"
		"        Shows this help and then exits.\n"
		"\n"
		"    " ARG_VERBOSE_SHORT "|" ARG_VERBOSE_LONG " (optional):\n"
		"        Enables verbose logging, so a lot more information gets output.\n"
		"\n"
		"    <file> (required):\n"
		"        The file you want to build with.  There can only be one.\n"
		"        This file must be a C++ code file.\n"
		"\n"
		"    " ARG_CONFIG "<config> (optional):\n"
		"        Sets the config to whatever you specify.\n"
		"        This must match the name of a config that you set inside \"" SET_BUILDER_OPTIONS_FUNC_NAME "\".\n"
		"\n"
		"    " ARG_NUKE " <folder> (optional):\n"
		"        Deletes every file in <folder> and all subfolders, but does not delete <folder>.\n"
		"\n"
		"    " ARG_VISUAL_STUDIO_BUILD " (optional):\n"
		"        Specifies that the build is being done from Visual Studio.\n"
		"        So even if BuilderOptions::generateSolution is set to true in the build settings source file we shouldn't generate Visual Studio project files and instead should just do a build using the specified config.\n"
		"\n"
		"    [custom arguments] (optional):\n"
		"        Any arguments not listed here are treated as custom arguments and passed through to your build source file via the CommandLineArgs parameter in " SET_BUILDER_OPTIONS_FUNC_NAME ".\n"
		"        Use HasCommandLineArg( CommandLineArgs *, const char * ) to query for them.\n"
		"\n"
	);

	return exitCode;
}

static bool8 ShouldRebuildSourceFile( const buildContext_t *context, const char *sourceFile, const char *intermediateFilename, const u32 sourceFileHashmapIndex ) {
	if ( context->forceRebuild ) {
		return true;
	}

	// if source file doesnt exist in hashmap then its a new file and we havent built this one before
	if ( sourceFileHashmapIndex == HASHMAP_INVALID_VALUE ) {
		return true;
	}

	// if the source file is newer than the intermediate file then we want to rebuild
	u64 intermediateFileLastWriteTime = 0;
	{
		// if the .o file doesnt exist then assume we havent built this file yet
		if ( !FS_GetFileLastWriteTime( intermediateFilename, &intermediateFileLastWriteTime ) ) {
			return true;
		}

		// if the .o file does exist but the source file was written to it more recently then we know we want to rebuild
		if ( GetLastFileWriteTime( sourceFile ) > intermediateFileLastWriteTime ) {
			return true;
		}
	}

	// if the source file wasnt newer than the .o file then do the same timestamp check for all the files that this source file depends on
	// just because the source file didnt change doesnt mean we dont want to recompile it
	// what if one of the header files it relies on changed? we still want to recompile that file!
	{
		const std::vector<std::string> &includeDependencies = context->sourceFileIncludeDependencies[sourceFileHashmapIndex].includeDependencies;

		For ( u64, dependencyIndex, 0, includeDependencies.size() ) {
			if ( GetLastFileWriteTime( includeDependencies[dependencyIndex].c_str() ) > intermediateFileLastWriteTime ) {
				return true;
			}
		}
	}

	return false;
}

struct compileJobPool_t {
	compilerBackend_t				*compilerBackend;
	buildContext_t					*context;
	BuildConfig						*config;
	compilationCommandArchetype_t	*cmdArchetype;
	std::vector<std::string>		*intermediateFiles;
	bool8							generateCompilationDatabase;
	u32								numSourceFiles;
	atomic32_t						nextSourceFileIndex;
	atomic32_t						numFailed;
};

static s32 CompileJobThread( void *data ) {
	compileJobPool_t *pool = Cast( compileJobPool_t *, data );

	while ( 1 ) {
		u32 sourceFileIndex = Thread_AtomicIncrement( &pool->nextSourceFileIndex ) - 1;

		if ( sourceFileIndex >= pool->numSourceFiles ) {
			break;
		}

		u64 marker = Mem_TempTell();
		defer { Mem_TempRewindTo( marker ); };

		const char *sourceFile = pool->config->sourceFiles[sourceFileIndex].c_str();

		string_t sourceFileNoPath = String_Set( sourceFile );
		sourceFileNoPath = Path_RemovePathFromFile( &sourceFileNoPath );

		string_t sourceFileNoPathAndExtension = Path_RemoveFileExtension( &sourceFileNoPath );

		string_t intermediateFilename = String_Printf( Mem_GetTempStorage(), "%s%c%s.o", pool->config->intermediateFolder.c_str(), PATH_SEPARATOR, String_Cstr( &sourceFileNoPathAndExtension ) );
		( *pool->intermediateFiles )[sourceFileIndex] = intermediateFilename.data;

		u32 sourceFileHashmapIndex = HM_GetValue( pool->context->sourceFileIndices, HashString( sourceFile, 0 ) );

		if ( !ShouldRebuildSourceFile( pool->context, sourceFile, intermediateFilename.data, sourceFileHashmapIndex ) ) {
			continue;
		}

		std::vector<std::string> includeDependencies;
		if ( !pool->compilerBackend->CompileSourceFile( pool->compilerBackend, pool->context, pool->config, *pool->cmdArchetype, sourceFile, pool->generateCompilationDatabase, sourceFileIndex, &includeDependencies ) ) {
			Thread_AtomicIncrement( &pool->numFailed );
			continue;
		}

		pool->context->sourceFileIncludeDependencies[sourceFileHashmapIndex].includeDependencies = std::move( includeDependencies );
	}

	return 0;
}

static buildResult_t BuildBinary( buildContext_t *context, BuildConfig *config, compilerBackend_t *compilerBackend, const BuilderOptions *options ) {
	// create binary folder
	if ( !FS_CreateFolderIfItDoesntExist( config->binaryFolder.c_str() ) ) {
		s32 errorCode = GetLastErrorCode();
		FatalError( "Failed to create the binary folder you specified inside %s: \"%s\".  Error code: " ERROR_CODE_FORMAT "", SET_BUILDER_OPTIONS_FUNC_NAME, config->binaryFolder.c_str(), errorCode );
		return BUILD_RESULT_FAILED;
	}

	// create intermediate folder
	if ( !FS_CreateFolderIfItDoesntExist( config->intermediateFolder.c_str() ) ) {
		s32 errorCode = GetLastErrorCode();
		FatalError( "Failed to create intermediate binary folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );
		return BUILD_RESULT_FAILED;
	}

	if ( config->OnPreBuild ) {
		LogVerbose( "Found a OnPreBuild() func ptr for BuildConfig: \"%s\".  Running...\n", config->name.c_str() );
		config->OnPreBuild();
	}

	std::vector<std::string> intermediateFiles;
	intermediateFiles.resize( config->sourceFiles.size() );

	// process_t only once how the base compilation command should look like, fill up dep/output/source args later for each source file
	compilationCommandArchetype_t cmdArchetype {};
	if ( !compilerBackend->GetCompilationCommandArchetype( compilerBackend, config, cmdArchetype ) ) {
		Error( "Failed to generate compilation command.\n" );
		return BUILD_RESULT_FAILED;
	}

	if ( context->consolidateCompilerArgs ) {
		printf( "Compiling with the following command line options for each source file:\n" );
		For ( u32, argIndex, 0, cmdArchetype.baseArgs.count ) {
			printf( "%s ", cmdArchetype.baseArgs[argIndex] );
		}
		printf( "\n" );
	} else {
		printf( "Compiling:\n" );
	}

	bool8 generateCompilationDatabase = options && options->generateCompilationDatabase;
	if ( generateCompilationDatabase ) {
		context->compilationDatabase.resize( config->sourceFiles.size() );
	}

	// ensure every source file has a slot in sourceFileIncludeDependencies before the compile loop runs
	// this allows compile jobs to write their include dependencies by index with no contention
	// the hashmap loaded from disk was sized for previously-seen files only
	// so rebuild it with enough capacity for all current source files before adding any new entries
	{
		u64 totalCapacity = context->sourceFileIncludeDependencies.size() + config->sourceFiles.size();

		hashmap_t *freshHashmap = HM_Create( context->allocator, TruncCast( u32, totalCapacity ), 0.5f );

		For ( u64, i, 0, context->sourceFileIncludeDependencies.size() ) {
			u64 hash = HashString( context->sourceFileIncludeDependencies[i].filename.c_str(), 0 );
			HM_SetValue( freshHashmap, hash, TruncCast( u32, i ) );
		}

		context->sourceFileIndices = freshHashmap;
	}

	For ( u64, sourceFileIndex, 0, config->sourceFiles.size() ) {
		const char *sourceFile = config->sourceFiles[sourceFileIndex].c_str();
		u64 sourceFileHash = HashString( sourceFile, 0 );

		if ( HM_GetValue( context->sourceFileIndices, sourceFileHash ) == HASHMAP_INVALID_VALUE ) {
			u32 newIndex = TruncCast( u32, context->sourceFileIncludeDependencies.size() );

			context->sourceFileIncludeDependencies.push_back( { sourceFile, {} } );

			HM_SetValue( context->sourceFileIndices, sourceFileHash, newIndex );
		}
	}

	// compile step
	// subtract 1 from the number of CPU cores queried because the main thread already occupies one core
	// spawning OS_GetNumCpuCores() threads would give us N+1 threads for N cores
	// causing the OS scheduler to context-switch between them, adding unnecessary overhead
	u32 numCores = Max( OS_GetNumCpuCores() - 1, 1 );
	u32 numThreads = Min( numCores, TruncCast( u32, config->sourceFiles.size() ) );

	printf( "Compiling %" PRIu64 " files across %u threads.\n", config->sourceFiles.size(), numThreads );

	compileJobPool_t pool = {
		.compilerBackend				= compilerBackend,
		.context						= context,
		.config							= config,
		.cmdArchetype					= &cmdArchetype,
		.intermediateFiles				= &intermediateFiles,
		.generateCompilationDatabase	= generateCompilationDatabase,
		.numSourceFiles					= TruncCast( u32, config->sourceFiles.size() ),
		.nextSourceFileIndex			= { 0 },
		.numFailed						= { 0 },
	};

	array_t<thread_t> threads;
	threads.Init( Mem_GetTempStorage() );
	threads.Reserve( numThreads );

	For ( u64, threadIndex, 0, numThreads ) {
		threads.Add( Thread_Create( CompileJobThread, &pool ) );
	}

	defer {
		For ( u64, threadIndex, 0, numThreads ) {
			Thread_Destroy( &threads[threadIndex] );
		}
	};

	For ( u64, threadIndex, 0, threads.count ) {
		Thread_Wait( &threads[threadIndex] );
	}

	if ( pool.numFailed.value > 0 ) {
		Error( "Compile failed.\n" );
		return BUILD_RESULT_FAILED;
	}

	// link step
	// we only want to link if the binary doesnt exist or if any of the intermediate files are newer than the binary
	// otherwise we can skip it
	{
		bool8 doLinking = false;

		const char *fullBinaryName = BuildConfig_GetFullBinaryName( config, Mem_GetTempStorage() );

		u64 binaryFileLastWriteTime = 0;

		if ( !FS_GetFileLastWriteTime( fullBinaryName, &binaryFileLastWriteTime ) ) {
			doLinking = true;
		} else {
			For ( u64, intermediateFileIndex, 0, intermediateFiles.size() ) {
				u64 intermediateFileLastWriteTime = GetLastFileWriteTime( intermediateFiles[intermediateFileIndex].c_str() );

				if ( intermediateFileLastWriteTime > binaryFileLastWriteTime ) {
					doLinking = true;
					break;
				}
			}
		}

		if ( !doLinking ) {
			return BUILD_RESULT_SKIPPED;
		}

		printf( "\nLinking:\n" );

		if ( !compilerBackend->LinkIntermediateFiles( compilerBackend, intermediateFiles, config, options ) ) {
			Error( "Linking failed.\n" );
			return BUILD_RESULT_FAILED;
		}
	}

	if ( config->OnPostBuild ) {
		LogVerbose( "Found a OnPostBuild() func ptr for BuildConfig: \"%s\".  Running...\n", config->name.c_str() );
		config->OnPostBuild();
	}

	return BUILD_RESULT_SUCCESS;
}

struct nukeContext_t {
	array_t<const char *>	subfolders;
	bool8					printDeletions;
};

static void Nuke_DeleteAllFilesAndCacheFoldersInternal( const fileInfo_t *fileInfo, void *userData ) {
	nukeContext_t *context = Cast( nukeContext_t *, userData );

	if ( fileInfo->isDirectory ) {
		context->subfolders.Add( fileInfo->fullFilename );
	} else {
		LogVerbose( "Deleting file \"%s\"\n", fileInfo->fullFilename );

		if ( !FS_DeleteFile( fileInfo->fullFilename ) ) {
			Error( "Nuke failed to delete file \"%s\".\n", fileInfo->fullFilename );
		}
	}
}

bool8 NukeFolder( const char *folder, const bool8 deleteRootFolder, const bool8 printDeletions ) {
	nukeContext_t nukeContext = {
		.printDeletions = printDeletions,
	};
	nukeContext.subfolders.Init( Mem_GetTempStorage() );

	fileVisitFlags_t visitFlags = FILE_VISIT_FILES | FILE_VISIT_RECURSIVE | FILE_VISIT_FOLDERS;

	if ( !FS_GetAllFilesInFolder( folder, visitFlags, Nuke_DeleteAllFilesAndCacheFoldersInternal, &nukeContext ) ) {
		Error( "Failed to visit all files in folder \"%s\" while trying to nuke it.  You'll have to clean these files and folders up manually.  Sorry.\n", folder );
		QUIT_ERROR();
	}

	bool8 result = true;

	RFor ( u64, subfolderIndex, 0, nukeContext.subfolders.count ) {
		const char *subfolder = nukeContext.subfolders[subfolderIndex];

		if ( printDeletions ) {
			printf( "Deleting folder \"%s\"\n", subfolder );
		}

		if ( !FS_DeleteFolder( subfolder ) ) {
			Error( "Failed to delete subfolder \"%s\".  You will need to nuke this one manually.  Sorry.\n", subfolder );
			result = false;
		}
	}

	if ( deleteRootFolder ) {
		if ( !FS_DeleteFolder( folder ) ) {
			Error( "Failed to nuke root folder \"%s\" after deleting all the files and folders inside it.  You'll need to do this manually.  Sorry.\n", folder );
			result = false;
		}
	}

	return result;
}


const char *GetNextSlashInPath( const char *path ) {
	const char *nextSlash = NULL;
	const char *nextBackSlash = strchr( path, '\\' );
	const char *nextForwardSlash = strchr( path, '/' );

	if ( !nextBackSlash && !nextForwardSlash ) {
		return NULL;
	}

	if ( Cast( u64, nextBackSlash ) < Cast( u64, nextForwardSlash ) ) {
		nextSlash = nextBackSlash;
	} else {
		nextSlash = nextForwardSlash;
	}

	return nextSlash;
}

static bool8 FileMatchesFilter( const string_t *filename, const string_t *filter ) {
	if ( filter->count == 0 ) {
		return false;
	}

	u64 afterLastWildcard = 0;
	u64 filterIndex = 0;
	For (u64, filenameIndex, 0, filename->count ) {
		// there are more characters that haven't been matched
		if ( filterIndex == filter->count ) {
			return false;
		}

		// keep consuming wildcards to reach next match
		while ( filter->data[filterIndex] == '*' ) {
			afterLastWildcard = ++filterIndex;
			if ( filterIndex == filter->count ) {
				return true; // rest of the characters are matched via wildcard
			}
		}

		// consume match resetting if match fails
		if ( filter->data[filterIndex++] != filename->data[filenameIndex] ) {
			filterIndex = afterLastWildcard;
		}
	}

	// since we consumed all characters in match and filename it's a match
	return filterIndex == filter->count;
}

bool8 PathMatchesFilter( const string_t *path, const string_t *pathFilter) {
	// simplest case where we have no glob pattern for folders at all
	if ( path->count == 0 && pathFilter->count == 0 ) {
		return true;
	}

	bool8 inRecursiveGlob = false;
	u64 afterLastSlash = 0;
	u64 filterIndex = 0;
	u64 pathIndex = 0;
	while ( filterIndex < pathFilter->count ) {
		if ( pathFilter->data[filterIndex] == '/' || pathFilter->data[filterIndex] == '\\' ) {
			// check if we must consume one folder (aka '/*/' )
			// or if we have begun a recursive glob (2 or more wildcards)
			u64 distance = filterIndex - afterLastSlash;
			if ( distance == 2 ) {
				inRecursiveGlob = true;
			} else if ( distance == 1 ) {
				if ( pathIndex == path->count ) {
					return false; // no more folders to consume
				}
				while ( pathIndex < path->count && path->data[pathIndex] != '/' && path->data[pathIndex] != '\\' ) {
					++pathIndex;
				}
				if ( pathIndex < path->count ) {
					++pathIndex;
				}
			}

			afterLastSlash = ++filterIndex;
		} else if ( pathFilter->data[filterIndex++] != '*' ) {
			// if there isn't a wildcard or slash we must find next folder that matches the user's pattern
			// we get each section by advancing to next slash or end of string
			while ( filterIndex < pathFilter->count && pathFilter->data[filterIndex] != '/' && pathFilter->data[filterIndex] != '\\' ) {
				++filterIndex;
			}
			string_t pattern = String_Substring( pathFilter->data, afterLastSlash, filterIndex - afterLastSlash );

			bool8 foundMatch = false;
			while ( pathIndex < path->count ) {
				u64 folderStart = pathIndex;
				while ( pathIndex < path->count && path->data[pathIndex] != '/' && path->data[pathIndex] != '\\' ) {
					++pathIndex;
				}

				// check for match in this section
				string_t folder = String_Substring( path->data, folderStart, pathIndex - folderStart );
				if ( pathIndex < path->count ) {
					++pathIndex; // go past the next slash after grabbing folder
				}
				if ( FileMatchesFilter( &folder, &pattern ) ) {
					foundMatch = true;
					break;
				} else if ( !inRecursiveGlob ) {
					return false;
				}
			}

			// there was SOME folder to match and we didn't match it
			if ( !foundMatch ) {
				return false;
			}

			// if we have reached the end then we actually matched everything and we are done
			inRecursiveGlob = (pathIndex == path->count && filterIndex == pathFilter->count);
		}
	}

	// if we finished parsing filter and were in recursive glob then
	// we can match anything remaining in the path
	return inRecursiveGlob;
}

struct sourceFileFindVisitorData_t {
	std::vector<std::string>	sourceFiles;
	const string_t				*searchFilter;
	const string_t				*folderFilter;
	const string_t				*basePath;
};

static void SourceFileVisitor( const fileInfo_t *fileInfo, void *userData ) {
	sourceFileFindVisitorData_t *visitorData2 = Cast( sourceFileFindVisitorData_t *, userData );

	// TODO: AK: 17/07/2026: currently we rely onthere not being new double slashes in the
	// path of the full filename, we should not do that and just have a proper standardisation
	// step in the right places, also if file globbing can't support mismatching double slashes
	// then that is probably a needed fix. We should add more tests that check the glob functionality.
	string_t filename = String_Set( fileInfo->filename );

	const u64 fullFilenameLen = strlen( fileInfo->fullFilename );
	const u64 basePathLen = visitorData2->basePath->count;
	if ( fullFilenameLen < basePathLen + filename.count ) { // maybe this should happen if they use ../../src/ or similar - where should we handle this?
		FatalError( "Source file \"%s\" is shorter than base path \"%s\".  This should never happen.\n", fileInfo->fullFilename, visitorData2->basePath);
		return;
	}

	// specifically the part of the path that needs to match the folder filter
	string_t pathToCheck = String_Substring( fileInfo->fullFilename, basePathLen, fullFilenameLen - basePathLen - filename.count );

	// match filename first as will generally be cheaper since only accounting for * not **
	bool8 fileMatches = FileMatchesFilter( &filename, visitorData2->searchFilter );

	if ( fileMatches && PathMatchesFilter( &pathToCheck, visitorData2->folderFilter ) ) {
		LogVerbose( " - Found \"%s\"\n", fileInfo->fullFilename );
		visitorData2->sourceFiles.push_back( fileInfo->fullFilename );
	}
}

std::vector<std::string> GetSourceFilesMatchingPattern( const string_t *basePath, const string_t *folderPattern, const string_t *filePattern ) {
	sourceFileFindVisitorData_t visitorData = {};
	visitorData.searchFilter = filePattern;
	visitorData.folderFilter = folderPattern;
	visitorData.basePath = basePath;

	fileVisitFlags_t visitFlags = FILE_VISIT_FILES;
	if ( folderPattern->count ) {
		visitFlags |= FILE_VISIT_RECURSIVE;
	}

	if ( !FS_GetAllFilesInFolder( basePath->data, visitFlags, SourceFileVisitor, &visitorData ) ) {
		FatalError( "Failed to get source file(s) \"%s%s\".  This should never happen.\n", folderPattern, filePattern);
	}

	return visitorData.sourceFiles;
}

static std::vector<std::string> GetAllSourceFiles( const string_t *inputFilePath, const std::vector<std::string>& sourceFiles ) {
	std::vector<std::string> allSourceFiles;

	For ( u64, sourceFileIndex, 0, sourceFiles.size() ) {
		const char *rawSourceFile = sourceFiles[sourceFileIndex].c_str();

		// normalise to an absolute path so wildcards, duplicate slashes, and
		// relative paths all resolve consistently against the build input dir
		// at this point we could also do some checks for duplication if we wanted to protect the user against this
		string_t sourceFile;
		if ( Path_IsAbsolute( rawSourceFile ) ) {
			sourceFile = Path_AbsolutePath( Mem_GetTempStorage(), rawSourceFile );
		} else {
#if defined(__linux__)
			// AK: I hate every part of this, but unlike windows the glob pattern is not handled fine being in Path_AbsolutePath
			string_t rawFile = String_Set( rawSourceFile );

			// seperate glob and forward from path before it
			string_t pathBeforeFirstGlob = {};
			For ( u64, rawFileChar, 0, rawFile.count ) {
				if ( rawFile.data[rawFileChar] == '*' ) {
					while ( rawFileChar-- != 0 ) { // will exit after processing 0
						if ( rawFile.data[rawFileChar] == '/' || rawFile.data[rawFileChar] == '\\' ) {
							pathBeforeFirstGlob = String_Set( rawFile.data, rawFileChar );
							rawFile = String_Substring( rawFile.data, rawFileChar + 1, rawFile.count - rawFileChar );
							break;
						}
					}
					break;
				}
			}

			// only calculate the absolute path on the glob-free portion
			const char *pathPortion;
			if ( pathBeforeFirstGlob.count != 0 ) {
				pathPortion = TempPrintf( "%s%c%s", inputFilePath->data, '/', String_Cstr( &pathBeforeFirstGlob ) );
			} else {
				pathPortion = inputFilePath->data;
			}
			sourceFile = Path_AbsolutePath( Mem_GetTempStorage(), pathPortion );

			// now combine the two and we have the same result...
			sourceFile = String_Printf( Mem_GetTempStorage(), "%s%c%s", String_Cstr( &sourceFile ), '/', String_Cstr( &rawFile ) );

#else
			const char *joined = TempPrintf( "%s%c%s", inputFilePath->data, '/', rawSourceFile );
			sourceFile = Path_AbsolutePath( Mem_GetTempStorage(), joined );
#endif
		}

		// only glob if the filename has a wildcard
		if ( String_Contains( &sourceFile, '*' ) ) {
			LogVerbose( "About to glob all source files found under user-specified pattern \"%s\" to the list of source files to build with:\n", rawSourceFile);

			// basePath must be the directory before the first wildcard
			u64 baseLen;
			String_FindFromLeft(&sourceFile, '*', &baseLen);
			while ( baseLen > 0 && sourceFile.data[baseLen - 1] != '/' && sourceFile.data[baseLen - 1] != '\\' ) {
				baseLen--;
			}

			// null terminate for api functions
			string_t basePath = String_Alloc( Mem_GetTempStorage(), sourceFile.data, baseLen + 1 );
			basePath.data[baseLen] = '\0';
			basePath.count = baseLen;

			u64 fileStart = sourceFile.count;
			while ( fileStart > 0 && sourceFile.data[fileStart - 1] != '/' && sourceFile.data[fileStart - 1] != '\\' ) {
				fileStart--;
			}
			string_t fileFilter = String_Substring( sourceFile.data, fileStart, sourceFile.count - fileStart );
			string_t folderFilter = String_Substring( sourceFile.data, baseLen, sourceFile.count - fileFilter.count - baseLen);

			std::vector<std::string> matches = GetSourceFilesMatchingPattern( &basePath, &folderFilter, &fileFilter );

			allSourceFiles.insert( allSourceFiles.end(), matches.begin(), matches.end() );
		} else {
			// otherwise its a single file, so we can just get it
			LogVerbose( "Adding source file \"%s\" to the list of source files to build with (no glob).\n", rawSourceFile);

			allSourceFiles.push_back( String_Cstr( &sourceFile ) );
		}
	}

	return allSourceFiles;
}

static inline std::vector<std::string> BuildConfig_GetAllSourceFiles( const buildContext_t *context, const BuildConfig *config ) {
	return GetAllSourceFiles( &context->inputFilePath, config->sourceFiles );
}

static void AddBuildConfigAndDependenciesUnique( buildContext_t *context, const BuildConfig *config, std::vector<BuildConfig> &outConfigs ) {
	u64 configNameHash = HashString( config->name.c_str(), 0 );

	if ( HM_GetValue( context->configIndices, configNameHash ) == HASHMAP_INVALID_VALUE ) {
		// add other configs that this config depends on first
		For ( size_t, dependencyIndex, 0, config->dependsOn.size() ) {
			AddBuildConfigAndDependenciesUnique( context, &config->dependsOn[dependencyIndex], outConfigs );
		}

		outConfigs.push_back( *config );

		HM_SetValue( context->configIndices, configNameHash, TruncCast( u32, outConfigs.size() - 1 ) );
	}
}

struct byteBuffer_t {
	array_t<u8>	data;
	u64			readOffset;
};

static void ReadIncludeDependenciesFile( buildContext_t *context ) {
	byteBuffer_t byteBuffer = {};

	// there wont be an include dependencies file on the first build or if you nuked the binaries folder (for instance)
	// so this is allowed to fail
	string_t includeDependenciesFileBuffer = {};
	if ( !FS_ReadEntireFile( context->includeDependenciesFilename.data, &includeDependenciesFileBuffer ) ) {
		context->sourceFileIndices = HM_Create( context->allocator, 1, 1.0f );
		return;
	}

	byteBuffer.data.data = Cast( u8 *, includeDependenciesFileBuffer.data );
	byteBuffer.data.count = includeDependenciesFileBuffer.count;

	auto ByteBuffer_Read_U32 = []( byteBuffer_t *buffer ) -> u32 {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
		u32 *result = Cast( u32 *, &buffer->data[buffer->readOffset] );
#pragma clang diagnostic pop

		buffer->readOffset += sizeof( u32 );

		return *result;
	};

	auto ByteBuffer_Read_String = [&ByteBuffer_Read_U32]( byteBuffer_t *buffer ) -> std::string {
		u32 stringLength = ByteBuffer_Read_U32( buffer );

		std::string result( Cast( char *, &buffer->data[buffer->readOffset] ), stringLength );

		buffer->readOffset += stringLength;

		return result;
	};

	u32 numSourceFiles = ByteBuffer_Read_U32( &byteBuffer );

	context->sourceFileIndices = HM_Create( context->allocator, numSourceFiles, 1.0f );

	context->sourceFileIncludeDependencies.resize( numSourceFiles );

	For ( u64, sourceFileIndex, 0, context->sourceFileIncludeDependencies.size() ) {
		includeDependencies_t *sourceFileIncludeDependencies = &context->sourceFileIncludeDependencies[sourceFileIndex];

		std::string sourceFilename = ByteBuffer_Read_String( &byteBuffer );
		u64 sourceFilenameHash = HashString( sourceFilename.c_str(), 0 );
		u32 sourceFileIndexU32 = TruncCast( u32, sourceFileIndex );
		HM_SetValue( context->sourceFileIndices, sourceFilenameHash, sourceFileIndexU32 );

		sourceFileIncludeDependencies->filename = sourceFilename;

		u64 numIncludeDependencies = ByteBuffer_Read_U32( &byteBuffer );
		sourceFileIncludeDependencies->includeDependencies.resize( numIncludeDependencies );

		For ( u64, dependencyIndex, 0, numIncludeDependencies ) {
			sourceFileIncludeDependencies->includeDependencies[dependencyIndex] = ByteBuffer_Read_String( &byteBuffer );
		}
	}
}

static bool8 WriteIncludeDependenciesFile( buildContext_t *context ) {
	byteBuffer_t byteBuffer = {};
	byteBuffer.data.Init( Mem_GetTempStorage() );

	auto ByteBuffer_Write_U32 = []( byteBuffer_t *buffer, const u32 x ) {
		buffer->data.Reserve( buffer->data.count + sizeof( u32 ) );

		buffer->data.Add( ( x ) & 0xFF );
		buffer->data.Add( ( x >> 8 ) & 0xFF );
		buffer->data.Add( ( x >> 16 ) & 0xFF );
		buffer->data.Add( ( x >> 24 ) & 0xFF );
	};

	auto ByteBuffer_Write_String = [&ByteBuffer_Write_U32]( byteBuffer_t *buffer, const std::string &string ) {
		u32 stringLength = TruncCast( u32, string.size() );

		ByteBuffer_Write_U32( buffer, stringLength );

		buffer->data.AddRange( Cast( const u8 *, string.data() ), stringLength );
	};

	ByteBuffer_Write_U32( &byteBuffer, TruncCast( u32, context->sourceFileIncludeDependencies.size() ) );

	For ( u64, sourceFileIndex, 0, context->sourceFileIncludeDependencies.size() ) {
		const includeDependencies_t *sourceFileIncludeDependencies = &context->sourceFileIncludeDependencies[sourceFileIndex];

		ByteBuffer_Write_String( &byteBuffer, context->sourceFileIncludeDependencies[sourceFileIndex].filename );

		ByteBuffer_Write_U32( &byteBuffer, TruncCast( u32, sourceFileIncludeDependencies->includeDependencies.size() ) );

		For ( u64, dependencyIndex, 0, sourceFileIncludeDependencies->includeDependencies.size() ) {
			const std::string &dependencyFilename = sourceFileIncludeDependencies->includeDependencies[dependencyIndex];

			ByteBuffer_Write_String( &byteBuffer, dependencyFilename );
		}
	}

	if ( !FS_WriteEntireFile( context->includeDependenciesFilename.data, byteBuffer.data.data, byteBuffer.data.count ) ) {
		s32 errorCode = GetLastErrorCode();
		Error( "Failed to write file \"%s\".  Error code: " ERROR_CODE_FORMAT ".\n", context->includeDependenciesFilename.data, errorCode );
		return false;
	}

	return true;
}

void RecordCompilationDatabaseEntry( buildContext_t *buildContext, const char *sourceFileName, const array_t<const char *> &compilationCommandArray, const u64 sourceFileIndex ) {
	u64 pos = Mem_TempTell();
	defer { Mem_TempRewindTo( pos ); };

	string_t directory = Path_AbsolutePath( Mem_GetTempStorage(), buildContext->inputFilePath.data );
	string_t file = Path_AbsolutePath( Mem_GetTempStorage(), sourceFileName );

	compilationDatabaseEntry_t entry = {};
	entry.directory = directory.data;
	entry.file = file.data;

	entry.arguments.reserve( compilationCommandArray.count );

	For ( u64, argIndex, 0, compilationCommandArray.count ) {
		const char *arg = compilationCommandArray[argIndex];
		// The reason for this is because Core uses a thirdparty library under-the-hood in prcoess_create for subprocesses,
		// which requires that the args list contains `NULL` at the end of the array, so we just insert one at the end so the user doesn't have to.
		if ( !arg ) {
			continue;
		}

		entry.arguments.emplace_back( arg );
	}

	buildContext->compilationDatabase[sourceFileIndex] = entry;
}

enum flagArgumentFormBits_t {
	JOINED		= BIT( 0 ),
	SEPARATE	= BIT( 1 ),
	COLON		= BIT( 2 )
};
typedef u32 argumentForms_t;

struct flagRule_t {
	const char		*flag = nullptr;
	argumentForms_t	forms;
};

static constexpr flagRule_t flagArgumentRules[] = {
	// MSVC
	{ "/I",     JOINED | SEPARATE },
	{ "/Fo",    JOINED | SEPARATE },
	{ "/Fd",    COLON | SEPARATE },
	{ "/Fp",    JOINED | COLON | SEPARATE },
	{ "/Yu",    JOINED },
	{ "/Yc",    JOINED },
	{ "/Fi",    SEPARATE },
	{ "@",      JOINED }, // ED: not supported for now

	// Clang/GCC
	{ "-I",         JOINED | SEPARATE },
	{ "-isystem",   SEPARATE },
	{ "-iquote",    SEPARATE },
	{ "-idirafter", SEPARATE },
	{ "-imacros",   SEPARATE },
	{ "-include",   SEPARATE },
	{ "-F",         SEPARATE },
	{ "-MF",        SEPARATE },
	{ "-MT",        SEPARATE },
	{ "-o",         SEPARATE }
};

static const flagRule_t *IsFlagMatch( const char *arg ) {
	for ( const auto &r : flagArgumentRules ) {
		if ( String_StartsWith( arg, r.flag ) ) {
			return &r;
		}
	}

	return nullptr;
}

static void FixCompilatiomDatabasePath( std::string &path ) {
	for ( char &c : path ) {
		if ( c == '\\' ) {
			c = '/';
		}
	}
}

// Processes the compilation arguments and sanitizes those that are paths arguments, to follow the json format,
// but following the possible combinations in which the compile flag can be provided, based on the compiler
// (see flagRule_t). This was thought as a more optimal way of doing it, instead of checking character by character for each argument.
// Also, AFAIK paths in compilation databases are expected to be full paths.
static void SanitizeCompilationDatabaseArgs( std::vector<std::string> &args ) {
	For ( size_t, argIndex, 0, args.size() ) {
		std::string &arg = args[argIndex];

		if ( arg.empty() ) {
			continue;
		}

		const size_t argLength = arg.size();
		const char *argPtr = arg.c_str();

		const flagRule_t *rule = IsFlagMatch( arg.c_str() );

		// Paths not related to compiler-specific flags
		if ( !rule ) {
			if ( Path_IsAbsolute( argPtr ) || FileIsSourceFile( argPtr ) || FileIsHeaderFile( argPtr ) ) {
#if 0
				std::string path = Path_AbsolutePath( arg.c_str() );
				FixCompilatiomDatabasePath( path );
				arg = std::move( path );
#else
				string_t path = Path_AbsolutePath( Mem_GetTempStorage(), arg.c_str() );
				path = String_Replace( Mem_GetTempStorage(), &path, '\\', '/' );
				arg = String_Cstr( &path );
#endif
			}

			continue;
		}

		u64 ruleLength = strlen( rule->flag );
		const argumentForms_t ruleForms = rule->forms;
		const char *ruleFlag = rule->flag;

		bool handled = false;

		// Joined form
		if ( ( ruleForms & JOINED ) && argLength > ruleLength && arg.compare( 0, ruleLength, ruleFlag ) == 0 ) {
#if 0
			std::string path = Path_AbsolutePath( arg.substr( ruleLength ).c_str() );
			if ( !path.empty() ) {
				FixCompilatiomDatabasePath( path );
				arg = ruleFlag + path;
				handled = true;
			}
#else
			string_t path = Path_AbsolutePath( Mem_GetTempStorage(), arg.c_str() );
			path = String_Replace( Mem_GetTempStorage(), &path, '\\', '/' );
			arg = String_Cstr( &path );
#endif
		}

		// Colon form
		if ( !handled && ( ruleForms & COLON ) && argLength > ruleLength && arg[ruleLength] == ':' ) {
#if 0
			std::string path = Path_AbsolutePath( arg.substr( ruleLength + 1 ).c_str() );
			FixCompilatiomDatabasePath( path );
			arg = std::string( ruleFlag ) + ":" + path;
			handled = true;
#else
			string_t path = Path_AbsolutePath( Mem_GetTempStorage(), arg.substr( ruleLength + 1 ).c_str() );
			path = String_Replace( Mem_GetTempStorage(), &path, '\\', '/' );
			arg = std::string( ruleFlag ) + ":" + String_Cstr( &path );
			handled = true;
#endif
		}

		// Separate form
		if ( !handled && ( ruleForms & SEPARATE ) ) {
			if ( argIndex + 1 < args.size() ) {
#if 0
				std::string &nextArg = args[++argIndex];
				std::string path = Path_AbsolutePath( nextArg.c_str() );
				FixCompilatiomDatabasePath( path );
				nextArg = std::move( path );
#else
				std::string &nextArg = args[++argIndex];
				string_t path = Path_AbsolutePath( Mem_GetTempStorage(), nextArg.c_str() );
				path = String_Replace( Mem_GetTempStorage(), &path, '\\', '/' );
				nextArg = String_Cstr( &path );
#endif
			}
		}
	}
}

static bool WriteCompilationDatabase( buildContext_t *context ) {
	if ( context->compilationDatabase.empty() ) {
		return true;
	}

	stringBuilder_t sb = SB_Create( Mem_GetTempStorage() );

	SB_Appendf( &sb, "[\n" );

	std::vector<u64> validIndices;
	For ( u64, i, 0, context->compilationDatabase.size() ) {
		if ( !context->compilationDatabase[i].file.empty() ) {
			validIndices.push_back( i );
		}
	}

	const u64 validCount = validIndices.size();
	For ( u64, validIndex, 0, validCount ) {
		compilationDatabaseEntry_t &entry = context->compilationDatabase[validIndices[validIndex]];

		FixCompilatiomDatabasePath( entry.directory );
		FixCompilatiomDatabasePath( entry.file );

		const char *directory = entry.directory.c_str();
		const char *file = entry.file.c_str();

		SB_Appendf(
			&sb,
			"  {\n"
			"    \"directory\": \"%s\",\n"
			"    \"file\": \"%s\",\n"
			"    \"arguments\": [\n",
			directory,
			file
		);

		SanitizeCompilationDatabaseArgs( entry.arguments );

		const u64 argumentsCount = entry.arguments.size();
		For ( u64, argIndex, 0, argumentsCount ) {
			SB_Appendf(
				&sb,
				"      \"%s\"%s\n",
				entry.arguments[argIndex].c_str(),
				( argIndex + 1 < argumentsCount ) ? "," : ""
			);
		}

		SB_Appendf(
			&sb,
			"    ]\n"
			"  }%s\n",
			( validIndex + 1 < validCount ) ? "," : ""
		);
	}

	SB_Appendf( &sb, "]\n" );

	const char *json = SB_ToString( &sb );
	Assert( json );

	const char *outputFilename = TempPrintf( "%s%ccompile_commands.json", context->inputFilePath.data, PATH_SEPARATOR );
	if ( !FS_WriteEntireFile( outputFilename, json, strlen( json ) ) ) {
		s32 errorCode = GetLastErrorCode();
		Error(
			"Failed to write compilation database \"%s\". Error code: " ERROR_CODE_FORMAT "\n",
			outputFilename,
			errorCode
		);
		return false;
	}

	return true;
}

int BuilderMain( const int firstArg, int argc, const char * const * argv ) {
	float64 totalTimeStart = Time_MS();

	float64 userConfigBuildTimeMS = -1.0;
	float64 setBuilderOptionsTimeMS = -1.0;
	float64 compilerBackendInitTimeMS = -1.0;
	float64 visualStudioGenerationTimeMS = -1.0;
	float64 vsCodeJSONGenerationTimeMS = -1.0;
	float64 zedJSONGenerationTimeMS = -1.0;

	// TODO: DM: 07/05/2026: we only do this check of ownership because the tests also need to init temp storage
	// I feel like there's a cleaner solution to this, just not sure what
	bool8 ownsTempStorage = Mem_GetTempStorage() == NULL;
	if ( ownsTempStorage ) {
		Mem_InitTempStorage( MEM_MEGABYTES( 128 ) );
	}
	defer {
		if ( ownsTempStorage ) {
			Mem_ShutdownTempStorage();
		}
	};

	printf( "Builder v%d.%d.%d RC0\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = {};
	context.allocator = Mem_CreateAllocator( MEM_KILOBYTES( 128 ) );
	context.configIndices = HM_Create( context.allocator, 1 );	// TODO(DM): 30/03/2025: whats a reasonable default here?

	defer {
		Mem_DestroyAllocator( context.allocator );
		context.allocator = NULL;
	};

	// init command line args
	const char *inputConfigName = NULL;
	u64 inputConfigNameHash = 0;

	bool8 isVisualStudioBuild = false;

	CommandLineArgs args = {
		.argc = argc,
		// .argv = argv,
	};

	args.argv = Cast( char **, Mem_Alloc( context.allocator, Cast( u32, argc ) * sizeof( const char * ) ) );

	For ( s32, argIndex, 0, argc ) {
		u64 argLength = strlen( argv[argIndex] );

		char *outArg = Cast( char *, Mem_Alloc( context.allocator, argLength + 1 ) );
		memcpy( outArg, argv[argIndex], argLength );
		outArg[argLength] = 0;

		args.argv[argIndex] = outArg;
	}

	For ( s32, argIndex, firstArg, argc ) {
		const char *arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( String_Equals( arg, ARG_HELP_SHORT ) || String_Equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( String_Equals( arg, ARG_VERBOSE_SHORT ) || String_Equals( arg, ARG_VERBOSE_LONG ) ) {
			g_verbose = true;
			continue;
		}

		if ( FileIsSourceFile( arg ) ) {
			if ( context.inputFile != NULL ) {
				Error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}

			context.inputFile = arg;

			continue;
		}

		if ( String_StartsWith( arg, ARG_CONFIG ) ) {
			const char *equals = strchr( arg, '=' );

			if ( !equals ) {
				Error( "I detected that you want to set a config, but you never gave me the equals (=) immediately after it.  You need to do that.\n" );

				return ShowUsage( 1 );
			}

			const char *configName = equals + 1;

			if ( strlen( configName ) < 1 ) {
				Error( "You specified the start of the config arg, but you never actually gave me a name for the config.  I need that.\n" );

				return ShowUsage( 1 );
			}

			inputConfigName = configName;
			inputConfigNameHash = HashString( inputConfigName, 0 );

			continue;
		}

		if ( String_Equals( arg, ARG_NUKE ) ) {
			if ( argIndex == argc - 1 ) {
				Error( "You passed in " ARG_NUKE " but you never told me what folder you want me to nuke.  I need to know!" );
				QUIT_ERROR();
			}

			const char *folderToNuke = argv[argIndex + 1];

			float64 startTime = Time_MS();

			printf( "Nuking \"%s\"\n", folderToNuke );

			if ( !FS_FolderExists( folderToNuke ) ) {
				Error( "Can't nuke folder \"%s\" because it doesn't exist.  Have you typed it in correctly?\n", folderToNuke );
				QUIT_ERROR();
			}

			if ( !NukeFolder( folderToNuke, false, true ) ) {
				Error( "Failed to nuke folder \"%s\".  You will have to clean this one up manually by yourself.  Sorry.\n", folderToNuke );
				QUIT_ERROR();
			}

			float64 endTime = Time_MS();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		if ( String_Equals( arg, ARG_VISUAL_STUDIO_BUILD ) ) {
			isVisualStudioBuild = true;

			continue;
		}
	}

	// we need a source file specified at the command line
	// otherwise we dont know what to build!
	if ( context.inputFile == NULL ) {
		Error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Run builder " ARG_HELP_LONG " if you need help.\n"
		);

		QUIT_ERROR();
	}

#ifdef _WIN32
	if ( !Win_GetWindowsSDK( context.allocator, &context.winSDK ) ) {
		QUIT_ERROR();
	}

	if ( !Win_GetMSVCInstall( context.allocator, &context.msvcInstall ) ) {
		QUIT_ERROR();
	}

	printf( "\n" );
#endif

	// the default binary folder is the same folder as the source file
	// if the file doesnt have a path then assume its in the same path as the current working directory (where we are calling builder from)
	{
		string_t inputFileStr = String_Set( context.inputFile );
		string_t inputFilePath = Path_RemoveFileFromPath( &inputFileStr );

		if ( inputFilePath.count == inputFileStr.count ) {
			// the input file has no path part, so assume it lives in the current working directory
			inputFilePath = Path_GetCwd( Mem_GetTempStorage() );
		}

		context.inputFilePath = String_Alloc( context.allocator, inputFilePath.data, inputFilePath.count + 1 );
		context.inputFilePath.data[inputFilePath.count] = '\0';
		context.inputFilePath.count = inputFilePath.count;

		context.dotBuilderFolder = Path_Join( context.allocator, context.inputFilePath.data, ".builder" );

		string_t inputFileStripped = String_Set( context.inputFile );
		inputFileStripped = Path_RemoveFileExtension( &inputFileStripped );
		inputFileStripped = Path_RemovePathFromFile( &inputFileStripped );

		context.includeDependenciesFilename = String_Printf( context.allocator, "%s%c%s.include_dependencies", context.dotBuilderFolder.data, PATH_SEPARATOR, String_Cstr( &inputFileStripped ) );

		LogVerbose( "input file path                  : %s\n", context.inputFilePath.data );
		LogVerbose( ".builder folder location         : %s\n", context.dotBuilderFolder.data );
		LogVerbose( "includedependencies file location: %s\n", context.includeDependenciesFilename.data );
	}

	string_t defaultBinaryNameView = String_Set( context.inputFile );
	defaultBinaryNameView = Path_RemovePathFromFile( &defaultBinaryNameView );
	defaultBinaryNameView = Path_RemoveFileExtension( &defaultBinaryNameView );

	string_t defaultBinaryName = String_Alloc( context.allocator, defaultBinaryNameView.data, defaultBinaryNameView.count + 1 );
	defaultBinaryName.data[defaultBinaryNameView.count] = '\0';
	defaultBinaryName.count = defaultBinaryNameView.count;

	ReadIncludeDependenciesFile( &context );

	string_t appPathOnly = Path_AppPath( Mem_GetTempStorage() );
	appPathOnly = Path_RemoveFileFromPath( &appPathOnly );
	appPathOnly = String_Alloc( Mem_GetTempStorage(), appPathOnly.data, appPathOnly.count + 1 );
	appPathOnly.data[appPathOnly.count - 1] = '\0';
	appPathOnly.count -= 1;

	// init default compiler backend (the version of clang that builder came with)
	compilerBackend_t compilerBackend = {};
	CreateCompilerBackend_Clang( &compilerBackend );
	string_t defaultCompilerPath = Path_Join( Mem_GetTempStorage(), appPathOnly.data, "..", "clang", "bin", "clang" );
	compilerBackend.Init( &compilerBackend, &context, defaultCompilerPath.data, NULL );
	defer {
		compilerBackend.Shutdown( &compilerBackend );
	};

	// user config build step
	// see if they have SetBuilderOptions() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	buildResult_t userConfigBuildResult = BUILD_RESULT_SKIPPED;
	const char *userConfigFullBinaryName = NULL;
	{
		float64 userConfigBuildTimeStart = Time_MS();

		printf( "Doing user config build:\n" );

		BuildConfig userConfigBuildConfig = {
			.sourceFiles = {
				context.inputFile,
			},
			.defines = {
				"_CRT_SECURE_NO_WARNINGS",
				"BUILDER_DOING_USER_CONFIG_BUILD",
#if defined( _DEBUG )
				"_DEBUG",
#else
				"NDEBUG",
#endif
				"_DLL",
			},
			.additionalIncludes = {
				// add the folder that builder lives in as an additional include path otherwise people have no real way of being able to include it
				TempPrintf( "%s%c..%cinclude", appPathOnly.data, PATH_SEPARATOR, PATH_SEPARATOR ),
			},
			.additionalLibs = {
#if defined( _WIN64 )
				"user32",
#endif
			},
			.ignoreWarnings = {
				"-Wno-missing-prototypes",	// otherwise the user has to forward declare functions like SetBuilderOptions and thats annoying
				"-Wno-reorder-init-list",	// allow users to initialize struct members in whatever order they want
			},
#ifdef __linux__
			.additionalCompilerArguments = {
				"-fPIC"
			},
#endif
			.binaryName = defaultBinaryName.data,
			.binaryFolder = context.dotBuilderFolder.data,
			.intermediateFolder = context.dotBuilderFolder.data,
			.binaryType = BINARY_TYPE_DYNAMIC_LIBRARY,
			// this is needed because this tells the compiler what to set _ITERATOR_DEBUG_LEVEL to
			// ABI compatibility will be broken if this is not the same between all binaries
#if defined( _DEBUG )
			.optimizationLevel = OPTIMIZATION_LEVEL_O0,
#else
			.optimizationLevel = OPTIMIZATION_LEVEL_O3,
#endif
		};

		userConfigFullBinaryName = BuildConfig_GetFullBinaryName( &userConfigBuildConfig, Mem_GetTempStorage() );

		// Within build binary and check against the options checks for its existance, defaulting to false which is what the user config build wants for each option
		// So just pass through nullptr when calling BuildBinary for the options build and it will work as expected
		userConfigBuildResult = BuildBinary( &context, &userConfigBuildConfig, &compilerBackend, nullptr );

		switch ( userConfigBuildResult ) {
			case BUILD_RESULT_SUCCESS: {
				printf( "\n" );
				// if the user config DLL got rebuilt then compile settings might have changed
				// force a rebuild of everything
				context.forceRebuild = true;

				LogVerbose( "User config build was successful.  All of the BuildConfigs we build from now on will be fully rebuilt...\n\n" );
			} break;

			case BUILD_RESULT_FAILED: {
				Error( "Pre-build failed!\n" );
				QUIT_ERROR();
			} //break;

			case BUILD_RESULT_SKIPPED: {
				printf( "Skipped!\n\n" );
		 	} break;
		}

		userConfigBuildTimeMS = Time_MS() - userConfigBuildTimeStart;
	}

	BuilderOptions options = {};

	library_t library = Library_Load( userConfigFullBinaryName );

	if ( !library.ptr ) {
		FatalError( "Failed to load the user-config build DLL \"%s\".  This should never happen!\n", userConfigFullBinaryName );
		QUIT_ERROR();
	}

	defer {
		Library_Unload( &library );
	};

	typedef void ( *setBuilderOptionsFunc_t )( BuilderOptions *options, CommandLineArgs *args );
	typedef void ( *preBuildFunc_t )();
	typedef void ( *postBuildFunc_t )();

	preBuildFunc_t preBuildFunc = Cast( preBuildFunc_t, Library_GetSymbol( library, PRE_BUILD_FUNC_NAME ) );
	postBuildFunc_t postBuildFunc = Cast( postBuildFunc_t, Library_GetSymbol( library, POST_BUILD_FUNC_NAME ) );

	// get the user-specified options
	{
		setBuilderOptionsFunc_t setBuilderOptionsFunc = Cast( setBuilderOptionsFunc_t, Library_GetSymbol( library, SET_BUILDER_OPTIONS_FUNC_NAME ) );

		float64 setBuilderOptionsTimeStart = Time_MS();

		if ( setBuilderOptionsFunc ) {
			printf( "%s override function found.  Running...\n", SET_BUILDER_OPTIONS_FUNC_NAME );

			setBuilderOptionsFunc( &options, &args );

			printf( "%s override function finished.\n\n", SET_BUILDER_OPTIONS_FUNC_NAME );
		} else {
			LogVerbose( "No %s override function was found.\n\n", SET_BUILDER_OPTIONS_FUNC_NAME );
		}

		context.forceRebuild |= options.forceRebuild;
		context.consolidateCompilerArgs = options.consolidateCompilerArgs;

		setBuilderOptionsTimeMS = Time_MS() - setBuilderOptionsTimeStart;
	}

	std::vector<BuildConfig> configsToBuild;

	array_t<float64> configBuildTimes;
	configBuildTimes.Init( context.allocator );
	array_t<buildResult_t> configBuildResults;
	configBuildResults.Init( context.allocator );

	// if the user wants to generate a visual studio solution then only do that
	if ( options.generateSolution && !isVisualStudioBuild ) {
		float64 start = Time_MS();

		// you either want to generate a visual studio solution or build this config, but not both
		if ( inputConfigName ) {
			Error(
				"I see you want to generate a Visual Studio Solution, but you've also specified a config that you want to build.\n"
				"You must do one or the other, you can't do both.\n\n"
			);

			QUIT_ERROR();
		}

		// make sure BuilderOptions::configs and configs from visual studio match
		// we will need this list later for validation
		// at the same time grab the list of files we are adding to the solution
		options.configs.clear();
		For ( u64, projectIndex, 0, options.solution.projects.size() ) {
			VisualStudioProject *project = &options.solution.projects[projectIndex];

			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				config->options.sourceFiles = BuildConfig_GetAllSourceFiles( &context, &config->options );
				AddBuildConfigAndDependenciesUnique( &context, &config->options, options.configs );
			}

			if ( !project->extraFiles.empty() ) {
				project->extraFiles = GetAllSourceFiles( &context.inputFilePath, project->extraFiles );
			}
		}

		printf( "Generating Visual Studio files\n" );

		bool8 generated = GenerateVisualStudioSolution( &context, &options );

		if ( !generated ) {
			Error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
			QUIT_ERROR();
		}

		printf( "Done.\n\n" );

		visualStudioGenerationTimeMS = Time_MS() - start;
	} else if ( options.generateVSCodeJSONFiles ) {
		float64 start = Time_MS();

		if ( !GenerateVSCodeJSONFiles( &context, &options ) ) {
			Error( "Failed to generate VS Code JSON files.\n" );
			QUIT_ERROR();
		}

		vsCodeJSONGenerationTimeMS = Time_MS() - start;
	} else if ( options.generateZedJSONFiles ) {
		float64 start = Time_MS();

		if ( !GenerateZedJSONFiles( &context, &options ) ) {
			Error( "Failed to generate Zed JSON files.\n" );
			QUIT_ERROR();
		}

		zedJSONGenerationTimeMS = Time_MS() - start;
	} else {
		// otherwise the user wants to actually build

		// if the user asked for a specific compiler, set that now
		// if the user never specified a compiler, we can build with the default compiler
		if ( !options.compilerPath.empty() ) {
			LogVerbose( "Found override compiler backend \"%s\" from %s.\n", options.compilerPath.c_str(), SET_BUILDER_OPTIONS_FUNC_NAME );

			compilerBackend.Shutdown( &compilerBackend );

			string_t actualCompilerPath = {};
			string_t compilerPathView = String_Set( options.compilerPath.c_str() );
			{
				string_t pathToCompiler = Path_RemoveFileFromPath( &compilerPathView );
				if ( pathToCompiler.count != compilerPathView.count && !Path_IsAbsolute( options.compilerPath.c_str() ) ) {
					actualCompilerPath = Path_Join( Mem_GetTempStorage(), context.inputFilePath.data, options.compilerPath.c_str() );
				} else {
					actualCompilerPath = String_Set( options.compilerPath.c_str() );
				}

				if ( String_EndsWith( actualCompilerPath.data, ".exe" ) ) {
					actualCompilerPath = Path_RemoveFileExtension( &actualCompilerPath );
					actualCompilerPath = String_Alloc( Mem_GetTempStorage(), actualCompilerPath.data, actualCompilerPath.count + 1 );
					actualCompilerPath.data[actualCompilerPath.count - 1] = '\0';
					actualCompilerPath.count -= 1;
				}
			}

			if ( String_EndsWith( actualCompilerPath.data, "clang" ) ) {
				CreateCompilerBackend_Clang( &compilerBackend );
			} else if ( String_EndsWith( actualCompilerPath.data, "gcc" ) ) {
				CreateCompilerBackend_GCC( &compilerBackend );
			} else if ( String_EndsWith( actualCompilerPath.data, "cl" ) ) {
#ifdef _WIN32
				CreateCompilerBackend_MSVC( &compilerBackend );
#else
				Error(
					"It appears you want to compile with MSVC on a non-Windows platform.\n"
					"MSVC only supports Windows.  Sorry.\n"
				);

				QUIT_ERROR();
#endif
			} else {
				Error(
					"The compiler you want to build with (\"%s\") is not one that I recognise.\n"
					"Currently, I only support: Clang, GCC, and MSVC.\n"
					"So you must use one of those compilers and make the compiler path end with the name of the executable.  Sorry!\n"
					, options.compilerPath.c_str()
				);

				QUIT_ERROR();
			}

			options.compilerPath = actualCompilerPath.data;

			// init new compiler backend
			{
				float64 compilerBackendInitStart = Time_MS();

				if ( !compilerBackend.Init( &compilerBackend, &context, options.compilerPath.c_str(), options.compilerVersion.c_str() ) ) {
					QUIT_ERROR();
				}

				float64 compilerBackendInitEnd = Time_MS();

				compilerBackendInitTimeMS = compilerBackendInitEnd - compilerBackendInitStart;
			}
		}

		// check that version of the compiler the user actually has is what they expect it to be
		if ( !options.compilerVersion.empty() ) {
			string_t compilerVersion = compilerBackend.GetCompilerVersion( &compilerBackend );

			if ( !String_Equals( compilerVersion.data, options.compilerVersion.c_str() ) ) {
				Warning(
					"I see that you are using compiler version \"%s\", but compiler version \"%s\" was set in %s.\n"
					"I will still go ahead with building the program, but things may not work as you expect.\n\n"
					, compilerVersion.data, options.compilerVersion.c_str(), SET_BUILDER_OPTIONS_FUNC_NAME
				);
			}
		}

		// if no configs were manually added then assume we are just doing a default build with no user-specified options
		if ( options.configs.size() == 0 ) {
			LogVerbose( "No BuildConfigs were found (either none were specified inside \"%s\" or that function was never defined), so Builder will now treat the input file specified at the command line as the source file you want to build.\n", SET_BUILDER_OPTIONS_FUNC_NAME );

			string_t inputFileNoPath = String_Set( context.inputFile );
			inputFileNoPath = Path_RemovePathFromFile( &inputFileNoPath );

			BuildConfig config = {
				.sourceFiles = { inputFileNoPath.data },
			};

			options.configs.push_back( config );
		}

		// if only one config was added (either by user or as a default build) then we know we just want that one, no config command line arg is needed
		if ( options.configs.size() == 1 ) {
			AddBuildConfigAndDependenciesUnique( &context, &options.configs[0], configsToBuild );
		} else {
			if ( !inputConfigName ) {
				Error(
					"This build has multiple configs, but you never specified a config name.\n"
					"You must pass in a config name via " ARG_CONFIG "\n"
					"Run builder " ARG_HELP_LONG " if you need help.\n"
				);

				QUIT_ERROR();
			}

			For ( size_t, configIndex, 0, options.configs.size() ) {
				if ( options.configs[configIndex].name.empty() ) {
					Error(
						"You have multiple BuildConfigs in your build source file, but some of them have empty names.\n"
						"When you have multiple BuildConfigs, ALL of them MUST have non-empty names.\n"
						"You need to set 'BuildConfig::name' in every BuildConfig that you add via AddBuildConfig() (including dependencies!).\n"
					);

					QUIT_ERROR();
				}
			}
		}

		// none of the configs can have the same name
		// TODO(DM): 14/11/2024: can we do better than o(n^2) here?
		For ( size_t, configIndexA, 0, options.configs.size() ) {
			const char *configNameA = options.configs[configIndexA].name.c_str();
			u64 configNameHashA = HashString( configNameA, 0 );

			For ( size_t, configIndexB, 0, options.configs.size() ) {
				if ( configIndexA == configIndexB ) {
					continue;
				}

				const char *configNameB = options.configs[configIndexB].name.c_str();
				u64 configNameHashB = HashString( configNameB, 0 );

				if ( configNameHashA == configNameHashB ) {
					Error( "I found multiple configs with the name \"%s\".  All config names MUST be unique, otherwise I don't know which specific config you want me to build.\n", configNameA );
					QUIT_ERROR();
				}
			}
		}

		// of all the configs that the user filled out inside SetBuilderOptions
		// find the one the user asked for in the command line
		if ( inputConfigName ) {
			bool8 foundConfig = false;
			For ( u64, configIndex, 0, options.configs.size() ) {
				const BuildConfig *config = &options.configs[configIndex];

				if ( HashString( config->name.c_str(), 0 ) == inputConfigNameHash ) {
					AddBuildConfigAndDependenciesUnique( &context, config, configsToBuild );

					foundConfig = true;

					break;
				}
			}

			if ( !foundConfig ) {
				Error( "You passed the config name \"%s\" via the command line, but I never found a config with that name inside %s.  Make sure they match.\n", inputConfigName, SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}
		}

		configBuildTimes.Resize( configsToBuild.size() );
		configBuildResults.Resize( configsToBuild.size() );

		if ( preBuildFunc ) {
			printf( "Running pre-build code...\n" );

			string_t oldCWD = Path_GetCwd( Mem_GetTempStorage() );
			Path_SetCwd( context.inputFilePath.data );
			defer {
				Path_SetCwd( oldCWD.data );
			};

			preBuildFunc();
		}

		u32 numSuccessfulBuilds = 0;
		u32 numFailedBuilds = 0;
		u32 numSkippedBuilds = 0;

		For ( u64, configToBuildIndex, 0, configsToBuild.size() ) {
			BuildConfig *config = &configsToBuild[configToBuildIndex];

			// make sure that the binary folder and binary name are at least set to defaults
			if ( !config->binaryFolder.empty() ) {
				config->binaryFolder = TempPrintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, config->binaryFolder.c_str() );
			} else {
				config->binaryFolder = context.inputFilePath.data;
			}

			// make sure intermediate folder is set relative to the binary folder
			if ( !config->intermediateFolder.empty() ) {
				config->intermediateFolder = TempPrintf( "%s%c%s", config->binaryFolder.c_str(), PATH_SEPARATOR, config->intermediateFolder.c_str() );
			} else {
				config->intermediateFolder = config->binaryFolder;
			}

			if ( config->binaryName.empty() ) {
				config->binaryName = defaultBinaryName.data;
			}

			{
				if ( !config->name.empty() ) {
					printf( "Building config \"%s\":\n", config->name.c_str() );
				} else {
					printf( "Building config:\n" );
				}
			}

#ifdef _WIN32
			if ( options.linkAgainstWindowsDynamicRuntime ) {
				config->defines.push_back( "_DLL" );
			}
#endif

			// make all non-absolute additional include paths relative to the build source file
			For ( u64, includeIndex, 0, config->additionalIncludes.size() ) {
				const char *additionalInclude = config->additionalIncludes[includeIndex].c_str();

				if ( !Path_IsAbsolute( additionalInclude ) ) {
					config->additionalIncludes[includeIndex] = TempPrintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalInclude );
				}
			}

			// make all non-absolute additional library paths relative to the build source file
			For ( u64, libPathIndex, 0, config->additionalLibPaths.size() ) {
				const char *additionalLibPath = config->additionalLibPaths[libPathIndex].c_str();

				if ( !Path_IsAbsolute( additionalLibPath ) ) {
					config->additionalLibPaths[libPathIndex] = TempPrintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalLibPath );
				}
			}

			// get all the "compilation units" that we are actually going to give to the compiler
			// if no source files were added in SetBuilderOptions() then assume they only want to build the same file as the one specified via the command line
			if ( config->sourceFiles.size() == 0 ) {
				LogVerbose( "No source files were detected in BuildConfig \"%s\".  Builder will assume that the source file you specified at the command line (\"%s\") is what you want to build with.\n", config->name.c_str(), context.inputFile );

				config->sourceFiles.push_back( context.inputFile );
			} else {
				// otherwise the user told us to build other source files, so go find and build those instead
				// keep this as a std::vector because this gets fed back into BuilderOptions::sourceFiles
				config->sourceFiles = BuildConfig_GetAllSourceFiles( &context, config );

				// at this point its totally acceptable for finalSourceFilesToBuild to be empty
				// this is because the compiler should be the one that tells the user they specified no valid source files to build with
				// the compiler can and will throw an error for that, so let it
			}

			// now do the actual build
			{
				float64 buildTimeStart = Time_MS();

				configBuildResults[configToBuildIndex] = BuildBinary( &context, config, &compilerBackend, &options );

				configBuildTimes[configToBuildIndex] = Time_MS() - buildTimeStart;

				switch ( configBuildResults[configToBuildIndex] ) {
					case BUILD_RESULT_SUCCESS:
						numSuccessfulBuilds++;
						printf( "Finished building \"%s\", %f ms\n\n", config->binaryName.c_str(), configBuildTimes[configToBuildIndex] );
						break;

					case BUILD_RESULT_FAILED:
						numFailedBuilds++;
						Error( "Build failed.\n\n" );
						QUIT_ERROR();

					case BUILD_RESULT_SKIPPED:
						numSkippedBuilds++;
						printf( "Skipped!\n\n" );
						break;
				}
			}

			Mem_ResetTempStorage();
		}

		if ( postBuildFunc ) {
			printf( "Running post-build code...\n" );

			string_t oldCWD = Path_GetCwd( Mem_GetTempStorage() );
			Path_SetCwd( context.inputFilePath.data );
			defer {
				Path_SetCwd( oldCWD.data );
			};

			postBuildFunc();
		}

		if ( numSuccessfulBuilds > 0 && numFailedBuilds == 0 ) {
			if ( !WriteIncludeDependenciesFile( &context ) ) {
				QUIT_ERROR();
			}

			if ( options.generateCompilationDatabase && !WriteCompilationDatabase( &context ) ) {
				context.compilationDatabase.clear();
				QUIT_ERROR();
			}
		}
	}

	// build summary
	{
		struct buildSummaryLine_t {
			const char		*description;
			const float64	timeMS;
			const char		*suffix;	// can be NULL
		};

		array_t<buildSummaryLine_t> buildSummaryLines;
		buildSummaryLines.Init( Mem_GetTempStorage() );
		buildSummaryLines.Reserve( 4 + configsToBuild.size() );
		buildSummaryLines.Add( { "User config build",  userConfigBuildTimeMS, ( userConfigBuildResult == BUILD_RESULT_SKIPPED ) ? "(skipped)" : "" } );
		buildSummaryLines.Add( { "Compiler init time", compilerBackendInitTimeMS } );
		buildSummaryLines.Add( { "SetBuilderOptions",  setBuilderOptionsTimeMS } );
		if ( options.generateSolution && !isVisualStudioBuild ) {
			buildSummaryLines.Add( { "Generate solution", visualStudioGenerationTimeMS } );
		}
		For ( u32, configIndex, 0, configsToBuild.size() ) {
			buildSummaryLines.Add( { TempPrintf( "Build \"%s\"", configsToBuild[configIndex].name.c_str() ), configBuildTimes[configIndex], ( configBuildResults[configIndex] == BUILD_RESULT_SKIPPED ) ? "(skipped)" : "" } );
		}

		u32 lineLength = 0;
		For ( u32, i, 0, buildSummaryLines.count ) {
			lineLength = Max( lineLength, Cast( u32, strlen( buildSummaryLines[i].description ) ) );
		}

		printf( "Finished:\n" );
		For ( u32, lineIndex, 0, buildSummaryLines.count ) {
			buildSummaryLine_t *line = &buildSummaryLines[lineIndex];

			if ( !Float64Equals( line->timeMS, -1.0 ) ) {
				printf( "    %-*s: %f ms %s\n", lineLength, line->description, line->timeMS, line->suffix ? line->suffix : "" );
			}
		}
		// leave this one separate at the end because we want to capture the end timestamp as late as possible
		printf( "    %-*s: %f ms\n", lineLength, "Total time", Time_MS() - totalTimeStart );
		printf( "\n" );
	}

	return 0;
}
