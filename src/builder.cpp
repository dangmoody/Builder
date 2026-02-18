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

#include "core/include/allocation_context.h"
#include "core/include/array.inl"
#include "core/include/string_helpers.h"
#include "core/include/string_builder.h"
#include "core/include/paths.h"
#include "core/include/core_process.h"
#include "core/include/file.h"
#include "core/include/typecast.inl"
#include "core/include/temp_storage.h"
#include "core/include/hash.h"
#include "core/include/timer.h"
#include "core/include/library.h"
#include "core/include/core_string.h"
#include "core/include/hashmap.h"
#include "core/include/file.h"

#ifdef _WIN64
#include <Shlwapi.h>
#elif defined(__linux__)
#include <errno.h>
#endif

#include <stdio.h>

/*
=============================================================================

	Builder

	by Dan Moody

=============================================================================
*/

enum {
	BUILDER_VERSION_MAJOR	= 0,
	BUILDER_VERSION_MINOR	= 10,
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

#define QUIT_ERROR() \
	debug_break(); \
	return 1

#ifdef __linux__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

u64 GetLastFileWriteTime( const char *filename ) {
	u64 lastWriteTime = 0;
	if ( !file_get_last_write_time( filename, &lastWriteTime ) ) {
		assert( false );
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

	assertf( false, "Something went really wrong here.\n" );

	return "ERROR";
}

bool8 FileIsSourceFile( const char *filename ) {
	static const char *fileExtensions[] = {
		".cpp",
		".cxx",
		".cc",
		".c",
	};

	For ( u64, extensionIndex, 0, count_of( fileExtensions ) ) {
		if ( string_ends_with( filename, fileExtensions[extensionIndex] ) ) {
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

	For ( u64, extensionIndex, 0, count_of( fileExtensions ) ) {
		if ( string_ends_with( filename, fileExtensions[extensionIndex] ) ) {
			return true;
		}
	}

	return false;
}

static const char *BuildConfig_ToString( const BuildConfig *config ) {
	auto BinaryTypeToString = []( const BinaryType type ) -> const char * {
		switch ( type ) {
			case BINARY_TYPE_EXE:				return "BINARY_TYPE_EXE";
			case BINARY_TYPE_DYNAMIC_LIBRARY:	return "BINARY_TYPE_DYNAMIC_LIBRARY";
			case BINARY_TYPE_STATIC_LIBRARY:	return "BINARY_TYPE_STATIC_LIBRARY";
		}
	};

	auto OptimizationLevelToString = []( OptimizationLevel level ) -> const char * {
		switch ( level ) {
			case OPTIMIZATION_LEVEL_O0: return "OPTIMIZATION_LEVEL_00";
			case OPTIMIZATION_LEVEL_O1: return "OPTIMIZATION_LEVEL_01";
			case OPTIMIZATION_LEVEL_O2: return "OPTIMIZATION_LEVEL_02";
			case OPTIMIZATION_LEVEL_O3: return "OPTIMIZATION_LEVEL_03";
		}
	};

	StringBuilder builder = {};
	string_builder_reset( &builder );

	auto PrintCStringArray = [&builder]( const char *name, const std::vector<const char *> &array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i] );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	};

	auto PrintSTDStringArray = [&builder]( const char *name, const std::vector<std::string> &array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i].c_str() );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	};

	auto PrintField = [&builder]( const char *key, const char *value ) {
		string_builder_appendf( &builder, "\t%s: %s\n", key, value );
	};

	string_builder_appendf( &builder, "%s: {\n", config->name.c_str() );

	if ( config->dependsOn.size() > 0 ) {
		string_builder_appendf( &builder, "\tdepends_on: { " );
		For ( u64, dependencyIndex, 0, config->dependsOn.size() ) {
			string_builder_appendf( &builder, "%s", config->dependsOn[dependencyIndex].name.c_str() );

			if ( dependencyIndex < config->dependsOn.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	}

	PrintSTDStringArray( "sourceFiles", config->sourceFiles );
	PrintSTDStringArray( "defines", config->defines );
	PrintSTDStringArray( "additionalIncludes", config->additionalIncludes );
	PrintSTDStringArray( "additionalLibPaths", config->additionalLibPaths );
	PrintSTDStringArray( "additionalLibs", config->additionalLibs );
	PrintSTDStringArray( "ignoreWarnings", config->ignoreWarnings );
	PrintSTDStringArray( "additionalCompilerArguments", config->additionalCompilerArguments );

	PrintField( "binaryName", config->binaryName.c_str() );
	PrintField( "binaryFolder", config->binaryFolder.c_str() );
	PrintField( "binaryType", BinaryTypeToString( config->binaryType ) );
	PrintField( "optimizationLevel", OptimizationLevelToString( config->optimizationLevel ) );
	PrintField( "removeSymbols", config->removeSymbols ? "true" : "false" );
	PrintField( "removeFileExtension", config->removeFileExtension ? "true" : "false" );
	PrintField( "warningsAsErrors", config->warningsAsErrors ? "true" : "false" );

	string_builder_appendf( &builder, "}\n" );

	return string_builder_to_string( &builder );
}

const char *BuildConfig_GetFullBinaryName( const BuildConfig *config ) {
	assert( !config->binaryName.empty() );

	StringBuilder sb = {};
	string_builder_reset( &sb );

	if ( !config->binaryFolder.empty() ) {
		string_builder_appendf( &sb, "%s%c", config->binaryFolder.c_str(), PATH_SEPARATOR );
	}

	string_builder_appendf( &sb, "%s", config->binaryName.c_str() );

	if ( !config->removeFileExtension ) {
		string_builder_appendf( &sb, "%s", GetFileExtensionFromBinaryType( config->binaryType ) );
	}

	return string_builder_to_string( &sb );
}

s32 RunProc( Array<const char *> *args, Array<const char *> *environmentVariables, const procFlags_t procFlags, String *outStdout ) {
	assert( args );
	assert( args->data );
	assert( args->count >= 1 );

	if ( procFlags & PROC_FLAG_SHOW_ARGS ) {
		For ( u64, argIndex, 0, args->count ) {
			printf( "%s ", ( *args )[argIndex] );
		}
		printf( "\n" );
	}

	Process *process = process_create( args, environmentVariables, PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	if ( !process ) {
		error(
			"Failed to run process \"%s\".\n"
			"Is it definitely installed? Is it meant to be added to your PATH? Did you type the path correctly?\n"
			, ( *args )[0]
		);

		// DM: 20/07/2025: I'm not 100% sure that its totally ok to have -1 as our own special exit code to mean that the process couldnt be found
		// its totally possible for other processes to return -1 and have it mean something else
		// the interpretation of the exit code of the processes we run is the responsibility of the calling code and were probably making a lot of assumptions there
		return -1;
	}

	defer(
		process_destroy( process );
		process = NULL;
	);

	// show stdout
	StringBuilder sb = {};
	string_builder_reset( &sb );

	char buffer[1024] = {};
	while ( process_read_stdout( process, buffer, count_of( buffer ) ) ) {
		string_builder_appendf( &sb, buffer );

		if ( procFlags & PROC_FLAG_SHOW_STDOUT ) {
			printf( "%s", buffer );
		}
	}

	if ( outStdout ) {
		*outStdout = string_builder_to_string( &sb );
	}

	s32 exitCode = process_join( process );

	return exitCode;
}

static s32 ShowUsage( const s32 exitCode ) {
	printf(
		"Builder.exe\n"
		"\n"
		"USAGE:\n"
		"    Builder.exe <file> [arguments]\n"
		"\n"
		"Arguments:\n"
		"    " ARG_HELP_SHORT "|" ARG_HELP_LONG " (optional):\n"
		"        Shows this help and then exits.\n"
		"\n"
		"    " ARG_VERBOSE_SHORT "|" ARG_VERBOSE_LONG " (optional):\n"
		"        Enables verbose logging, so more detailed information gets output when doing a build.\n"
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
	);

	return exitCode;
}

static buildResult_t BuildBinary( buildContext_t *context, BuildConfig *config, compilerBackend_t *compilerBackend, bool generateCompilationDatabase ) {
	// create binary folder
	if ( !folder_create_if_it_doesnt_exist( config->binaryFolder.c_str() ) ) {
		errorCode_t errorCode = get_last_error_code();
		fatal_error( "Failed to create the binary folder you specified inside %s: \"%s\".  Error code: " ERROR_CODE_FORMAT "", SET_BUILDER_OPTIONS_FUNC_NAME, config->binaryFolder.c_str(), errorCode );
		return BUILD_RESULT_FAILED;
	}

	// create intermediate folder
	const char *intermediatePath = tprintf( "%s%c%s", config->binaryFolder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );
	if ( !folder_create_if_it_doesnt_exist( intermediatePath ) ) {
		errorCode_t errorCode = get_last_error_code();
		fatal_error( "Failed to create intermediate binary folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );
		return BUILD_RESULT_FAILED;
	}

	Array<const char *> intermediateFiles;
	intermediateFiles.reserve( config->sourceFiles.size() );

	// TODO(DM): 03/08/2025: this is kinda ugly
	auto ShouldRebuildSourceFile = [context]( const char *sourceFile, const char *intermediateFilename, u32 sourceFileHashmapIndex ) -> bool8 {
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
			if ( !file_get_last_write_time( intermediateFilename, &intermediateFileLastWriteTime ) ) {
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
	};

	// Process only once how the base compilation command should look like, fill up dep/output/source args later for each source file
	compilationCommandArchetype_t cmdArchetype {};
	if ( !compilerBackend->GetCompilationCommandArchetype( compilerBackend, config, cmdArchetype ) ) {
		error( "Failed to generate compilation command.\n" );
		return BUILD_RESULT_FAILED;
	}

	if ( generateCompilationDatabase ) {
		context->compilationDatabase.reserve( config->sourceFiles.size() );
	}

	// compile step
	// make .o files for all compilation units
	// TODO(DM): 14/06/2025: embarrassingly parallel
	For ( u64, sourceFileIndex, 0, config->sourceFiles.size() ) {
		const char *sourceFile = config->sourceFiles[sourceFileIndex].c_str();
		const char *sourceFileNoPath = path_remove_path_from_file( sourceFile );

		const char *intermediateFilename = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, path_remove_file_extension( sourceFileNoPath ) );
		intermediateFiles.add( intermediateFilename );

		const char *depFilename = tprintf( "%s%c%s.d", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

		u32 sourceFileHashmapIndex = hashmap_get_value( context->sourceFileIndices, hash_string( sourceFile, 0 ) );

		// only rebuild the .o file if the source file (or any of the files that source file depends on) was written to more recently or it doesnt exist
		if ( !ShouldRebuildSourceFile( sourceFile, intermediateFilename, sourceFileHashmapIndex ) ) {
			continue;
		}

		if ( !compilerBackend->CompileSourceFile( compilerBackend, context, config, cmdArchetype, sourceFile, generateCompilationDatabase ) ) {
			error( "Compile failed.\n" );
			return BUILD_RESULT_FAILED;
		}

		std::vector<std::string> includeDependencies;
		compilerBackend->GetIncludeDependenciesFromSourceFileBuild( compilerBackend, includeDependencies );

		if ( sourceFileHashmapIndex != HASHMAP_INVALID_VALUE ) {
			context->sourceFileIncludeDependencies[sourceFileHashmapIndex].includeDependencies = includeDependencies;
		} else {
			context->sourceFileIncludeDependencies.push_back( { sourceFile, includeDependencies } );
		}
	}

	// link step
	// we only want to link if the binary doesnt exist or if any of the intermediate files are newer than the binary
	// otherwise we can skip it
	{
		bool8 doLinking = false;

		const char *fullBinaryName = BuildConfig_GetFullBinaryName( config );

		u64 binaryFileLastWriteTime = 0;

		if ( !file_get_last_write_time( fullBinaryName, &binaryFileLastWriteTime ) ) {
			doLinking = true;
		} else {
			For ( u64, intermediateFileIndex, 0, intermediateFiles.count ) {
				u64 intermediateFileLastWriteTime = GetLastFileWriteTime( intermediateFiles[intermediateFileIndex] );

				if ( intermediateFileLastWriteTime > binaryFileLastWriteTime ) {
					doLinking = true;
					break;
				}
			}
		}

		if ( !doLinking ) {
			return BUILD_RESULT_SKIPPED;
		}

		if ( !compilerBackend->LinkIntermediateFiles( compilerBackend, intermediateFiles, config ) ) {
			error( "Linking failed.\n" );
			return BUILD_RESULT_FAILED;
		}
	}

	return BUILD_RESULT_SUCCESS;
}

struct nukeContext_t {
	Array<const char *>	subfolders;
	bool8				verbose;
};

static void Nuke_DeleteAllFilesAndCacheFoldersInternal( const FileInfo *fileInfo, void *user_data ) {
	nukeContext_t *context = cast( nukeContext_t *, user_data );

	if ( fileInfo->is_directory ) {
		context->subfolders.add( fileInfo->full_filename );
	} else {
		if ( context->verbose ) {
			printf( "Deleting file \"%s\"\n", fileInfo->full_filename );
		}

		if ( !file_delete( fileInfo->full_filename ) ) {
			error( "Nuke failed to delete file \"%s\".\n", fileInfo->full_filename );
		}
	}
}

bool8 NukeFolder( const char *folder, const bool8 deleteRootFolder, const bool8 verbose ) {
	nukeContext_t nukeContext = {
		.verbose = verbose
	};

	if ( !file_get_all_files_in_folder( folder, true, true, Nuke_DeleteAllFilesAndCacheFoldersInternal, &nukeContext ) ) {
		error( "Failed to visit all files in folder \"%s\" while trying to nuke it.  You'll have to clean these files and folders up manually.  Sorry.\n", folder );
		QUIT_ERROR();
	}

	bool8 result = true;

	RFor ( u64, subfolderIndex, 0, nukeContext.subfolders.count ) {
		const char *subfolder = nukeContext.subfolders[subfolderIndex];

		if ( nukeContext.verbose ) {
			printf( "Deleting folder \"%s\"\n", subfolder );
		}

		if ( !folder_delete( subfolder ) ) {
			error( "Failed to delete subfolder \"%s\".  You will need to nuke this one manually.  Sorry.\n", subfolder );
			result = false;
		}
	}

	if ( deleteRootFolder ) {
		if ( !folder_delete( folder ) ) {
			error( "Failed to nuke root folder \"%s\" after deleting all the files and folders inside it.  You'll need to do this manually.  Sorry.\n" );
			result = false;
		}
	}

	return result;
}

const char *GetNextSlashInPath( const char *path ) {
	const char *nextSlash = NULL;
	const char *nextBackSlash = strrchr( path, '\\' );
	const char *nextForwardSlash = strrchr( path, '/' );

	if ( !nextBackSlash && !nextForwardSlash ) {
		return NULL;
	}

	if ( cast( u64, nextBackSlash ) > cast( u64, nextForwardSlash ) ) {
		nextSlash = nextBackSlash;
	} else {
		nextSlash = nextForwardSlash;
	}

	return nextSlash;
}

static bool8 FileMatchesFilter( const char *filename, const char *filter ) {
	const char *filenameCopy = filename;
	const char *filterCopy = filter;

	const char *filenameBackup = NULL;
	const char *filterBackup = NULL;

	while ( *filenameCopy ) {
		if ( *filterCopy == '*' ) {
			filenameBackup = filenameCopy;
			filterBackup = ++filterCopy;
		} else if ( *filenameCopy == *filterCopy ) {
			filenameCopy += 1;
			filterCopy += 1;
		} else {
			if ( !filterBackup ) {
				return false;
			}

			filenameCopy = ++filenameBackup;
			filterCopy = filterBackup;
		}
	}

	return *filterCopy == 0;
}

struct sourceFileFindVisitorData_t {
	std::vector<std::string>	sourceFiles;
	const char					*searchFilter;
};

static void SourceFileVisitor( const FileInfo *fileInfo, void *userData ) {
	sourceFileFindVisitorData_t *visitorData2 = cast( sourceFileFindVisitorData_t *, userData );

	const char *checkAgainst = NULL;

	if ( string_contains( visitorData2->searchFilter, "/" ) || string_contains( visitorData2->searchFilter, "\\" ) ) {
		checkAgainst = fileInfo->full_filename;
	} else {
		checkAgainst = fileInfo->filename;
	}

	if ( FileMatchesFilter( checkAgainst, visitorData2->searchFilter ) ) {
		visitorData2->sourceFiles.push_back( fileInfo->full_filename );
	}
}

static std::vector<std::string> BuildConfig_GetAllSourceFiles( const buildContext_t *context, const BuildConfig *config ) {
	sourceFileFindVisitorData_t visitorData = {};

	For ( u64, sourceFileIndex, 0, config->sourceFiles.size() ) {
		const char *sourceFile = config->sourceFiles[sourceFileIndex].c_str();

		const char *sourceFileNoPath = path_remove_path_from_file( sourceFile );

		bool8 recursive = string_contains( sourceFile, "**" ) || string_contains( sourceFile, "/" );

		// TODO(DM): 02/10/2025: needing this is (probably) a hack
		// re-evaluate this
		bool8 inputFileIsSameAsSourceFile = string_equals( sourceFile, context->inputFile );
		if ( inputFileIsSameAsSourceFile ) {
			visitorData.searchFilter = context->inputFile;
		} else {
			visitorData.searchFilter = tprintf( "%s%c%s", context->inputFilePath.data, '/', sourceFile );
		}

		if ( !file_get_all_files_in_folder( context->inputFilePath.data, recursive, false, SourceFileVisitor, &visitorData ) ) {
			fatal_error( "Failed to get source file(s) \"%s\".  This should never happen.\n", sourceFile );
		}
	}

	return visitorData.sourceFiles;
}

static void AddBuildConfigAndDependenciesUnique( buildContext_t *context, const BuildConfig *config, std::vector<BuildConfig> &outConfigs ) {
	u64 configNameHash = hash_string( config->name.c_str(), 0 );

	if ( hashmap_get_value( context->configIndices, configNameHash ) == HASHMAP_INVALID_VALUE ) {
		// add other configs that this config depends on first
		For ( size_t, dependencyIndex, 0, config->dependsOn.size() ) {
			AddBuildConfigAndDependenciesUnique( context, &config->dependsOn[dependencyIndex], outConfigs );
		}

		outConfigs.push_back( *config );

		hashmap_set_value( context->configIndices, configNameHash, trunc_cast( u32, outConfigs.size() - 1 ) );
	}
}

struct byteBuffer_t {
	Array<u8>	data;
	u64			readOffset;
};

static const char *GetIncludeDepsFilename( buildContext_t *context ) {
	const char *inputFileStripped = path_remove_path_from_file( path_remove_file_extension( context->inputFile ) );
	const char *includeDepsFilename = tprintf( "%s%c%s.include_dependencies", context->dotBuilderFolder.data, PATH_SEPARATOR, inputFileStripped );

	return includeDepsFilename;
}

static void ReadIncludeDependenciesFile( buildContext_t *context ) {
	const char *includeDepsFilename = GetIncludeDepsFilename( context );

	byteBuffer_t byteBuffer = {};
	byteBuffer.data.allocator = mem_get_current_allocator();

	// there wont be an include dependencies file on the first build or if you nuked the binaries folder (for instance)
	// so this is allowed to fail
	if ( !file_read_entire( includeDepsFilename, cast( char **, &byteBuffer.data.data ), &byteBuffer.data.count ) ) {
		context->sourceFileIndices = hashmap_create( 1, 1.0f );
		return;
	}

	auto ByteBuffer_Read_U32 = []( byteBuffer_t *buffer ) -> u32 {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
		u32 *result = cast( u32 *, &buffer->data[buffer->readOffset] );
#pragma clang diagnostic pop

		buffer->readOffset += sizeof( u32 );

		return *result;
	};

	auto ByteBuffer_Read_String = [&ByteBuffer_Read_U32]( byteBuffer_t *buffer ) -> std::string {
		u32 stringLength = ByteBuffer_Read_U32( buffer );

		std::string result( cast( char *, &buffer->data[buffer->readOffset] ), stringLength );

		buffer->readOffset += stringLength;

		return result;
	};

	u32 numSourceFiles = ByteBuffer_Read_U32( &byteBuffer );

	context->sourceFileIndices = hashmap_create( numSourceFiles, 1.0f );

	context->sourceFileIncludeDependencies.resize( numSourceFiles );

	For ( u64, sourceFileIndex, 0, context->sourceFileIncludeDependencies.size() ) {
		includeDependencies_t *sourceFileIncludeDependencies = &context->sourceFileIncludeDependencies[sourceFileIndex];

		std::string sourceFilename = ByteBuffer_Read_String( &byteBuffer );
		u64 sourceFilenameHash = hash_string( sourceFilename.c_str(), 0 );
		u32 sourceFileIndexU32 = trunc_cast( u32, sourceFileIndex );
		hashmap_set_value( context->sourceFileIndices, sourceFilenameHash, sourceFileIndexU32 );

		sourceFileIncludeDependencies->filename = sourceFilename;

		u64 numIncludeDependencies = ByteBuffer_Read_U32( &byteBuffer );
		sourceFileIncludeDependencies->includeDependencies.resize( numIncludeDependencies );

		For ( u64, dependencyIndex, 0, numIncludeDependencies ) {
			sourceFileIncludeDependencies->includeDependencies[dependencyIndex] = ByteBuffer_Read_String( &byteBuffer );
		}
	}
}

static bool8 WriteIncludeDependenciesFile( buildContext_t *context ) {
	const char *includeDepsFilename = GetIncludeDepsFilename( context );

	byteBuffer_t byteBuffer = {};

	auto ByteBuffer_Write_U32 = []( byteBuffer_t *buffer, const u32 x ) {
		buffer->data.reserve( buffer->data.alloced + sizeof( u32 ) );

		buffer->data.add( ( x ) & 0xFF );
		buffer->data.add( ( x >> 8 ) & 0xFF );
		buffer->data.add( ( x >> 16 ) & 0xFF );
		buffer->data.add( ( x >> 24 ) & 0xFF );
	};

	auto ByteBuffer_Write_String = [&ByteBuffer_Write_U32]( byteBuffer_t *buffer, const std::string &string ) {
		u32 stringLength = trunc_cast( u32, string.size() );

		ByteBuffer_Write_U32( buffer, stringLength );

		buffer->data.add_range( cast( const u8 *, string.data() ), stringLength );
	};

	ByteBuffer_Write_U32( &byteBuffer, trunc_cast( u32, context->sourceFileIncludeDependencies.size() ) );

	For ( u64, sourceFileIndex, 0, context->sourceFileIncludeDependencies.size() ) {
		const includeDependencies_t *sourceFileIncludeDependencies = &context->sourceFileIncludeDependencies[sourceFileIndex];

		ByteBuffer_Write_String( &byteBuffer, context->sourceFileIncludeDependencies[sourceFileIndex].filename );

		ByteBuffer_Write_U32( &byteBuffer, trunc_cast( u32, sourceFileIncludeDependencies->includeDependencies.size() ) );

		For ( u64, dependencyIndex, 0, sourceFileIncludeDependencies->includeDependencies.size() ) {
			const std::string &dependencyFilename = sourceFileIncludeDependencies->includeDependencies[dependencyIndex];

			ByteBuffer_Write_String( &byteBuffer, dependencyFilename );
		}
	}

	if ( !file_write_entire( includeDepsFilename, byteBuffer.data.data, byteBuffer.data.count ) ) {
		errorCode_t errorCode = get_last_error_code();
		error( "Failed to write file \"%s\".  Error code: " ERROR_CODE_FORMAT ".\n", includeDepsFilename, errorCode );
		return false;
	}

	return true;
}


void RecordCompilationDatabaseEntry(
	buildContext_t *buildContext,
	const char *sourceFileName,
	const Array<const char *> &compilationCommandArray ) {

	compilationDatabaseEntry_t entry;
	entry.directory = path_absolute_path( buildContext->inputFilePath.data );
	entry.file = path_absolute_path( sourceFileName );

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

	buildContext->compilationDatabase.emplace_back( entry );
}

enum flagArgumentFormBits_t {
	JOINED		= bit( 0 ),
	SEPARATE	= bit( 1 ),
	COLON		= bit( 2 )
};
typedef u32 argumentForms_t;

struct flagRule_t {
	const char *flag = nullptr;
	argumentForms_t forms;
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
		if ( string_starts_with( arg, r.flag ) ) {
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
			if ( path_is_absolute( argPtr ) || FileIsSourceFile( argPtr ) || FileIsHeaderFile( argPtr ) ) {
				std::string path = path_absolute_path( arg.c_str() );
				FixCompilatiomDatabasePath( path );
				arg = std::move( path );
			}

			continue;
		}

		u64 ruleLength = strlen( rule->flag );
		const argumentForms_t ruleForms = rule->forms;
		const char *ruleFlag = rule->flag;

		bool handled = false;

		// Joined form
		if ( ( ruleForms & JOINED ) && argLength > ruleLength && arg.compare( 0, ruleLength, ruleFlag ) == 0 ) {
			std::string path = path_absolute_path( arg.substr( ruleLength ).c_str() );
			if ( !path.empty() ) {
				FixCompilatiomDatabasePath( path );
				arg = ruleFlag + path;
				handled = true;
			}
		}

		// Colon form
		if ( !handled && ( ruleForms & COLON ) && argLength > ruleLength && arg[ruleLength] == ':' ) {
			std::string path = path_absolute_path( arg.substr( ruleLength + 1 ).c_str() );
			FixCompilatiomDatabasePath( path );
			arg = std::string( ruleFlag ) + ":" + path;
			handled = true;
		}

		// Separate form
		if ( !handled && ( ruleForms & SEPARATE ) ) {
			if ( argIndex + 1 < args.size() ) {
				std::string &nextArg = args[++argIndex];
				std::string path = path_absolute_path( nextArg.c_str() );
				FixCompilatiomDatabasePath( path );
				nextArg = std::move( path );
			}
		}
	}
}

static bool WriteCompilationDatabase( buildContext_t *context ) {
	if ( context->compilationDatabase.empty() ) {
		return true;
	}

	StringBuilder sb = {};
	string_builder_reset( &sb );
	defer( string_builder_destroy( &sb ) );

	string_builder_appendf( &sb, "[\n" );

	const u64 entriesCount = context->compilationDatabase.size();
	For ( u64, i, 0, entriesCount ) {
		compilationDatabaseEntry_t &entry = context->compilationDatabase[i];

		FixCompilatiomDatabasePath( entry.directory );
		FixCompilatiomDatabasePath( entry.file );

		const char *directory = entry.directory.c_str();
		const char *file = entry.file.c_str();

		string_builder_appendf(
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
			string_builder_appendf(
				&sb,
				"      \"%s\"%s\n",
				entry.arguments[argIndex].c_str(),
				( argIndex + 1 < argumentsCount ) ? "," : ""
			);
		}

		string_builder_appendf(
			&sb,
			"    ]\n"
			"  }%s\n",
			( i + 1 < entriesCount ) ? "," : ""
		);
	}

	string_builder_appendf( &sb, "]\n" );

	const char *json = string_builder_to_string( &sb );
	assert( json );

	const char *outputFilename = tprintf( "%s%ccompile_commands.json", context->inputFilePath.data, PATH_SEPARATOR );
	if ( !file_write_entire( outputFilename, json, strlen( json ) ) ) {
		errorCode_t errorCode = get_last_error_code();
		error(
			"Failed to write compilation database \"%s\". Error code: " ERROR_CODE_FORMAT "\n",
			outputFilename,
			errorCode
		);
		return false;
	}

	return true;
}

int BuilderMain( const int firstArg, int argc, char **argv ) {
	float64 totalTimeStart = time_ms();

	float64 userConfigBuildTimeMS = -1.0;
	float64 setBuilderOptionsTimeMS = -1.0;
	float64 visualStudioGenerationTimeMS = -1.0;
	float64 buildInfoReadTimeMS = -1.0;
	float64 buildInfoWriteTimeMS = -1.0;

	core_init( MEM_MEGABYTES( 128 ) );	// TODO(DM): 26/03/2025: can we just use defaults for this now?
	defer( core_shutdown() );

	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = {
		.configIndices	= hashmap_create( 1 ),	// TODO(DM): 30/03/2025: whats a reasonable default here?
#ifdef _DEBUG
		.verbose		= true,
#else
		.verbose		= false,
#endif
	};

	// parse command line args
	const char *inputConfigName = NULL;
	u64 inputConfigNameHash = 0;

	bool8 isVisualStudioBuild = false;

	For ( s32, argIndex, firstArg, argc ) {
		const char *arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( string_equals( arg, ARG_HELP_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( string_equals( arg, ARG_VERBOSE_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			context.verbose = true;
			continue;
		}

		if ( FileIsSourceFile( arg ) ) {
			if ( context.inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}

			context.inputFile = arg;

			continue;
		}

		if ( string_starts_with( arg, ARG_CONFIG ) ) {
			const char *equals = strchr( arg, '=' );

			if ( !equals ) {
				error( "I detected that you want to set a config, but you never gave me the equals (=) immediately after it.  You need to do that.\n" );

				return ShowUsage( 1 );
			}

			const char *configName = equals + 1;

			if ( strlen( configName ) < 1 ) {
				error( "You specified the start of the config arg, but you never actually gave me a name for the config.  I need that.\n" );

				return ShowUsage( 1 );
			}

			inputConfigName = configName;
			inputConfigNameHash = hash_string( inputConfigName, 0 );

			continue;
		}

		if ( string_equals( arg, ARG_NUKE ) ) {
			if ( argIndex == argc - 1 ) {
				error( "You passed in " ARG_NUKE " but you never told me what folder you want me to nuke.  I need to know!" );
				QUIT_ERROR();
			}

			const char *folderToNuke = argv[argIndex + 1];

			float64 startTime = time_ms();

			printf( "Nuking \"%s\"\n", folderToNuke );

			if ( !folder_exists( folderToNuke ) ) {
				error( "Can't nuke folder \"%s\" because it doesn't exist.  Have you typed it in correctly?\n", folderToNuke );
				QUIT_ERROR();
			}

			if ( !NukeFolder( folderToNuke, false, true ) ) {
				error( "Failed to nuke folder \"%s\".  You will have to clean this one up manually by yourself.  Sorry.\n", folderToNuke );
				QUIT_ERROR();
			}

			float64 endTime = time_ms();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		if ( string_equals( arg, ARG_VISUAL_STUDIO_BUILD ) ) {
			isVisualStudioBuild = true;

			continue;
		}

		// unrecognised arg, show error
		error( "Unrecognised argument \"%s\".\n", arg );
		QUIT_ERROR();
	}

	// we need a source file specified at the command line
	// otherwise we dont know what to build!
	if ( context.inputFile == NULL ) {
		error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Run builder " ARG_HELP_LONG " if you need help.\n"
		);

		QUIT_ERROR();
	}

	// the default binary folder is the same folder as the source file
	// if the file doesnt have a path then assume its in the same path as the current working directory (where we are calling builder from)
	{
		const char *inputFilePath = path_remove_file_from_path( context.inputFile );

		if ( !inputFilePath ) {
			inputFilePath = path_current_working_directory();
		}

		const char *inputFileNoPath = path_remove_path_from_file( context.inputFile );
		const char *inputFileNoPathOrExtension = path_remove_file_extension( inputFileNoPath );

		context.inputFilePath = inputFilePath;

		string_printf( &context.dotBuilderFolder, "%s%c.builder", context.inputFilePath.data, PATH_SEPARATOR );
	}

	const char *defaultBinaryName = path_remove_file_extension( path_remove_path_from_file( context.inputFile ) );

	ReadIncludeDependenciesFile( &context );

	// init default compiler backend (the version of clang that builder came with)
	compilerBackend_t compilerBackend = {};
	CreateCompilerBackend_Clang( &compilerBackend );
	const char *defaultCompilerPath = tprintf( "%s%c..%cclang%cbin%cclang", path_remove_file_from_path( path_app_path() ), PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR );
	compilerBackend.Init( &compilerBackend, defaultCompilerPath, std::string() );
	defer( compilerBackend.Shutdown( &compilerBackend ) );

	// user config build step
	// see if they have SetBuilderOptions() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	buildResult_t userConfigBuildResult = BUILD_RESULT_SKIPPED;
	const char *userConfigFullBinaryName = NULL;
	{
		float64 userConfigBuildTimeStart = time_ms();

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
			},
			.additionalIncludes = {
				// add the folder that builder lives in as an additional include path otherwise people have no real way of being able to include it
				tprintf( "%s%c..%cinclude", path_remove_file_from_path( path_app_path() ), PATH_SEPARATOR, PATH_SEPARATOR ),
			},
			.additionalLibs = {
#if defined( _WIN64 )
				"user32.lib",
				// MSVCRT is needed for ABI compatibility between builder and the user config DLL on windows
#if defined( _DEBUG )
				"msvcrtd.lib",
#else
				"msvcrt.lib",
#endif
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
			.binaryName = defaultBinaryName,
			.binaryFolder = context.dotBuilderFolder.data,
			.binaryType = BINARY_TYPE_DYNAMIC_LIBRARY,
			// this is needed because this tells the compiler what to set _ITERATOR_DEBUG_LEVEL to
			// ABI compatibility will be broken if this is not the same between all binaries
#if defined( _DEBUG )
			.optimizationLevel = OPTIMIZATION_LEVEL_O0,
#else
			.optimizationLevel = OPTIMIZATION_LEVEL_O3,
#endif
		};

		userConfigFullBinaryName = BuildConfig_GetFullBinaryName( &userConfigBuildConfig );

		userConfigBuildResult = BuildBinary( &context, &userConfigBuildConfig, &compilerBackend, false );

		switch ( userConfigBuildResult ) {
			case BUILD_RESULT_SUCCESS:
				printf( "\n" );
				break;

			case BUILD_RESULT_FAILED:
				error( "Pre-build failed!\n" );
				QUIT_ERROR();

			case BUILD_RESULT_SKIPPED:
				printf( "Skipped!\n" );
				break;
		}

		float64 userConfigBuildTimeEnd = time_ms();

		userConfigBuildTimeMS = userConfigBuildTimeEnd - userConfigBuildTimeStart;
	}

	// if the user config DLL got rebuilt then compile settings might have changed
	// force a rebuild of everything
	if ( userConfigBuildResult == BUILD_RESULT_SUCCESS ) {
		context.forceRebuild = true;
	}

	BuilderOptions options = {};

	Library library = library_load( userConfigFullBinaryName );
	assertf( library.ptr, "Failed to load the user-config build DLL \"%s\".  This should never happen!\n", userConfigFullBinaryName );
	defer( library_unload( &library ) );

	typedef void ( *setBuilderOptionsFunc_t )( BuilderOptions *options );
	typedef void ( *preBuildFunc_t )();
	typedef void ( *postBuildFunc_t )();

	preBuildFunc_t preBuildFunc = cast( preBuildFunc_t, library_get_proc_address( library, PRE_BUILD_FUNC_NAME ) );
	postBuildFunc_t postBuildFunc = cast( postBuildFunc_t, library_get_proc_address( library, POST_BUILD_FUNC_NAME ) );

	{
		// now get the user-specified options
		setBuilderOptionsFunc_t setBuilderOptionsFunc = cast( setBuilderOptionsFunc_t, library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME ) );

		if ( setBuilderOptionsFunc ) {
			float64 setBuilderOptionsTimeStart = time_ms();

			setBuilderOptionsFunc( &options );

			float64 setBuilderOptionsTimeEnd = time_ms();

			setBuilderOptionsTimeMS = setBuilderOptionsTimeEnd - setBuilderOptionsTimeStart;

			context.forceRebuild = options.forceRebuild;

			// if the user wants to generate a visual studio solution then do that now
			if ( options.generateSolution && !isVisualStudioBuild ) {
				// you either want to generate a visual studio solution or build this config, but not both
				if ( inputConfigName ) {
					error(
						"I see you want to generate a Visual Studio Solution, but you've also specified a config that you want to build.\n"
						"You must do one or the other, you can't do both.\n\n"
					);

					QUIT_ERROR();
				}

				// make sure BuilderOptions::configs and configs from visual studio match
				// we will need this list later for validation
				options.configs.clear();
				For ( u64, projectIndex, 0, options.solution.projects.size() ) {
					VisualStudioProject *project = &options.solution.projects[projectIndex];

					For ( u64, configIndex, 0, project->configs.size() ) {
						VisualStudioConfig *config = &project->configs[configIndex];

						AddBuildConfigAndDependenciesUnique( &context, &config->options, options.configs );
					}
				}

				printf( "Generating Visual Studio files\n" );

				float64 start = time_ms();

				bool8 generated = GenerateVisualStudioSolution( &context, &options );

				float64 end = time_ms();

				visualStudioGenerationTimeMS = end - start;

				if ( !generated ) {
					error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
					QUIT_ERROR();
				}

				printf( "Done.\n" );

				return 0;
			}
		}
	}

	float64 compilerBackendInitTimeMS = -1.0f;
	{
		// if the user never specified a compiler, we can build with the default compiler that we just built the user config DLL with
		if ( !options.compilerPath.empty() ) {
			compilerBackend.Shutdown( &compilerBackend );

			if ( string_ends_with( options.compilerPath.c_str(), ".exe" ) ) {
				options.compilerPath = path_remove_file_extension( options.compilerPath.c_str() );
			}

			const char *path = path_remove_file_from_path( options.compilerPath.c_str() );

			if ( path && !path_is_absolute( path ) ) {
				options.compilerPath = context.inputFilePath.data + std::string( "/" ) + options.compilerPath;
			}

			if ( string_ends_with( options.compilerPath.c_str(), "clang" ) ) {
				CreateCompilerBackend_Clang( &compilerBackend );
			} else if ( string_ends_with( options.compilerPath.c_str(), "gcc" ) ) {
				CreateCompilerBackend_GCC( &compilerBackend );
			} else if ( string_ends_with( options.compilerPath.c_str(), "cl" ) ) {
#ifdef _WIN32
				CreateCompilerBackend_MSVC( &compilerBackend );
#else
				error(
					"It appears you want to compile with MSVC on a non-Windows platform.\n"
					"MSVC only supports Windows.  Sorry.\n"
				);

				QUIT_ERROR();
#endif
			} else {
				error(
					"The compiler you want to build with (\"%s\") is not one that I recognise.\n"
					"Currently, I only support: Clang, GCC, and MSVC.\n"
					"So you must use one of those compilers and make the compiler path end with the name of the executable.  Sorry!\n"
					, options.compilerPath.c_str()
				);

				QUIT_ERROR();
			}

			// init new compiler backend
			{
				float64 compilerBackendInitStart = time_ms();

				if ( !compilerBackend.Init( &compilerBackend, options.compilerPath.c_str(), options.compilerVersion.c_str() ) ) {
					QUIT_ERROR();
				}

				float64 compilerBackendInitEnd = time_ms();

				compilerBackendInitTimeMS = compilerBackendInitEnd - compilerBackendInitStart;
			}
		}

		{
			String compilerPath = compilerBackend.GetCompilerPath( &compilerBackend );
			const char *compilerPathPlusExtension = compilerPath.data;
#ifdef _WIN32
			// windows also requires the file extension to properly check for if a file exists
			if ( !string_ends_with( compilerPath.data, ".exe" ) ) {
				compilerPathPlusExtension = tprintf( "%s.exe", compilerPath.data );
			}
#endif

			if ( !file_exists( compilerPathPlusExtension ) ) {
				error( "Can't find path to overridden compiler \"%s\".  Did you type it correctly?\n", compilerPath.data );
				QUIT_ERROR();
			}
		}

		// check that version of the compiler the user actually has is what they expect it to be
		if ( !options.compilerVersion.empty() ) {
			String compilerVersion = compilerBackend.GetCompilerVersion( &compilerBackend );

			if ( !string_equals( compilerVersion.data, options.compilerVersion.c_str() ) ) {
				warning(
					"I see that you are using compiler version \"%s\", but compiler version \"%s\" was set in %s.\n"
					"I will still go ahead with building the program, but things may not work as you expect.\n\n"
					, compilerVersion.data, options.compilerVersion.c_str(), SET_BUILDER_OPTIONS_FUNC_NAME
				);
			}
		}
	}

	std::vector<BuildConfig> configsToBuild;

	// if no configs were manually added then assume we are just doing a default build with no user-specified options
	if ( options.configs.size() == 0 ) {
		BuildConfig config = {
			.sourceFiles = { context.inputFile },
			.binaryName = defaultBinaryName
		};

		options.configs.push_back( config );
	}

	// if only one config was added (either by user or as a default build) then we know we just want that one, no config command line arg is needed
	if ( options.configs.size() == 1 ) {
		AddBuildConfigAndDependenciesUnique( &context, &options.configs[0], configsToBuild );
	} else {
		if ( !inputConfigName ) {
			error(
				"This build has multiple configs, but you never specified a config name.\n"
				"You must pass in a config name via " ARG_CONFIG "\n"
				"Run builder " ARG_HELP_LONG " if you need help.\n"
			);

			QUIT_ERROR();
		}

		For ( size_t, configIndex, 0, options.configs.size() ) {
			if ( options.configs[configIndex].name.empty() ) {
				error(
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
		u64 configNameHashA = hash_string( configNameA, 0 );

		For ( size_t, configIndexB, 0, options.configs.size() ) {
			if ( configIndexA == configIndexB ) {
				continue;
			}

			const char *configNameB = options.configs[configIndexB].name.c_str();
			u64 configNameHashB = hash_string( configNameB, 0 );

			if ( configNameHashA == configNameHashB ) {
				error( "I found multiple configs with the name \"%s\".  All config names MUST be unique, otherwise I don't know which specific config you want me to build.\n", configNameA );
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

			if ( hash_string( config->name.c_str(), 0 ) == inputConfigNameHash ) {
				AddBuildConfigAndDependenciesUnique( &context, config, configsToBuild );

				foundConfig = true;

				break;
			}
		}

		if ( !foundConfig ) {
			error( "You passed the config name \"%s\" via the command line, but I never found a config with that name inside %s.  Make sure they match.\n", inputConfigName, SET_BUILDER_OPTIONS_FUNC_NAME );
			QUIT_ERROR();
		}
	}

	if ( preBuildFunc ) {
		printf( "Running pre-build code...\n" );

		const char *oldCWD = path_current_working_directory();
		path_set_current_directory( context.inputFilePath.data );
		defer( path_set_current_directory( oldCWD ) );

		preBuildFunc();
	}

	u32 numSuccessfulBuilds = 0;
	u32 numFailedBuilds = 0;
	u32 numSkippedBuilds = 0;

	For ( u64, configToBuildIndex, 0, configsToBuild.size() ) {
		BuildConfig *config = &configsToBuild[configToBuildIndex];

		// make sure that the binary folder and binary name are at least set to defaults
		if ( !config->binaryFolder.empty() ) {
			config->binaryFolder = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, config->binaryFolder.c_str() );
		} else {
			config->binaryFolder = context.inputFilePath.data;
		}

		{
			printf( "Building \"%s\"", config->binaryName.c_str() );

			if ( !config->name.empty() ) {
				printf( ", config \"%s\"", config->name.c_str() );
			}

			printf( ":\n" );
		}

		// make all non-absolute additional include paths relative to the build source file
		For ( u64, includeIndex, 0, config->additionalIncludes.size() ) {
			const char *additionalInclude = config->additionalIncludes[includeIndex].c_str();

			if ( !path_is_absolute( additionalInclude ) ) {
				config->additionalIncludes[includeIndex] = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalInclude );
			}
		}

		// make all non-absolute additional library paths relative to the build source file
		For ( u64, libPathIndex, 0, config->additionalLibPaths.size() ) {
			const char *additionalLibPath = config->additionalLibPaths[libPathIndex].c_str();

			if ( !path_is_absolute( additionalLibPath ) ) {
				config->additionalLibPaths[libPathIndex] = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalLibPath );
			}
		}

		// get all the "compilation units" that we are actually going to give to the compiler
		// if no source files were added in SetBuilderOptions() then assume they only want to build the same file as the one specified via the command line
		if ( config->sourceFiles.size() == 0 ) {
			config->sourceFiles.push_back( context.inputFile );
		} else {
			// otherwise the user told us to build other source files, so go find and build those instead
			// keep this as a std::vector because this gets fed back into BuilderOptions::sourceFiles
			std::vector<std::string> finalSourceFilesToBuild = BuildConfig_GetAllSourceFiles( &context, config );

			// at this point its totally acceptable for finalSourceFilesToBuild to be empty
			// this is because the compiler should be the one that tells the user they specified no valid source files to build with
			// the compiler can and will throw an error for that, so let it

			config->sourceFiles = finalSourceFilesToBuild;
		}

		// now do the actual build
		{
			float64 buildTimeStart = time_ms();

			buildResult_t buildResult = BuildBinary( &context, config, &compilerBackend, options.generateCompilationDatabase );

			float64 buildTimeEnd = time_ms();

			switch ( buildResult ) {
				case BUILD_RESULT_SUCCESS:
					numSuccessfulBuilds++;
					printf( "Finished building \"%s\", %f ms\n\n", config->binaryName.c_str(), buildTimeEnd - buildTimeStart );
					break;

				case BUILD_RESULT_FAILED:
					numFailedBuilds++;
					error( "Build failed.\n\n" );
					QUIT_ERROR();

				case BUILD_RESULT_SKIPPED:
					numSkippedBuilds++;
					printf( "Skipped!\n\n" );
					break;
			}
		}

		mem_reset_temp_storage();
	}

	if ( postBuildFunc ) {
		printf( "Running post-build code...\n" );

		const char *oldCWD = path_current_working_directory();
		path_set_current_directory( context.inputFilePath.data );
		defer( path_set_current_directory( oldCWD ) );

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

	float64 totalTimeEnd = time_ms();

	{
		using namespace hlml;

		printf( "Build finished:\n" );
		printf( "    User config build:  %f ms%s\n", userConfigBuildTimeMS, ( userConfigBuildResult == BUILD_RESULT_SKIPPED ) ? " (skipped)" : "" );
		if ( !doubleeq( compilerBackendInitTimeMS, -1.0 ) ) {
			printf( "    Compiler init time: %f ms\n", compilerBackendInitTimeMS );
		}
		if ( !doubleeq( setBuilderOptionsTimeMS, -1.0 ) ) {
			printf( "    SetBuilderOptions:  %f ms\n", setBuilderOptionsTimeMS );
		}
		if ( options.generateSolution && !isVisualStudioBuild ) {
			printf( "    Generate solution:  %f ms\n", visualStudioGenerationTimeMS );
		}
		printf( "    Total time:         %f ms\n", totalTimeEnd - totalTimeStart );
		printf( "\n" );
	}

	return 0;
}

#ifdef __linux__
#pragma clang diagnostic pop
#endif //__linux__
