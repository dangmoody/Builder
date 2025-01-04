/*
===========================================================================

Builder

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "../builder.h"

#include "core/src/core.suc.cpp"

#ifdef _WIN64
#include <Windows.h>
#include <Shlwapi.h>
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
	BUILDER_VERSION_MINOR	= 5,
	BUILDER_VERSION_PATCH	= 2,
};

#define ARG_HELP_SHORT		"-h"
#define ARG_HELP_LONG		"--help"
#define ARG_VERBOSE_SHORT	"-v"
#define ARG_VERBOSE_LONG	"--verbose"
#define ARG_NUKE			"--nuke"
#define ARG_CONFIG			"--config="

#define CLANG_VERSION	"18.1.8"

#define BUILD_INFO_FILE_EXTENSION			".build_info"
#define SET_BUILDER_OPTIONS_FUNC_NAME		"set_builder_options"
#define PRE_BUILD_FUNC_NAME					"on_pre_build"
#define POST_BUILD_FUNC_NAME				"on_post_build"

// project type guids are pre-determined by visual studio
// C++ is the only language that builder knows/cares about
#define VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID	"8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"

#define QuitError() \
	debug_break(); \
	return 1

#define CHECK_WRITE( func ) \
	do { \
		bool8 written = (func); \
		assertf( written, "Failed to write a visual studio project/solution file..\n" ); \
	} while ( 0 )

enum buildContextFlagBits_t {
	BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS	= bit( 0 ),
	BUILD_CONTEXT_FLAG_SHOW_STDOUT			= bit( 1 ),
};
typedef u32 buildContextFlags_t;

struct buildContext_t {
	//BuilderOptions			options;
	BuildConfig				config;

	// TODO(DM): 10/08/2024: does this want to be inside BuilderOptions?
	// it would give users more control over their build
	buildContextFlags_t		flags;

	const char*				fullBinaryName;
};

struct trackedSourceFile_t {
	u64						lastWriteTime;
	char*					filename;
};

struct buildInfoConfig_t {
	BuildConfig							config;
	std::vector<trackedSourceFile_t>	files;
};

#ifdef _WIN64
typedef DWORD errorCode_t;
#define ERROR_CODE_FORMAT "0x%X"
#else
#error Unrecognised platform!
#endif

static errorCode_t GetLastErrorCode() {
#ifdef _WIN64
	return GetLastError();
#else
#error Unrecognised platform!
#endif
}

static u64 maxull( const u64 x, const u64 y ) {
	return ( x < y ) ? x : y;
}

static const char* GetFileExtensionFromBinaryType( BinaryType type ) {
	switch ( type ) {
		case BINARY_TYPE_EXE:				return "exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return "dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return "lib";
	}

	assertf( false, "Something went really wrong here.\n" );

	return "ERROR";
}

static s32 RunProc( Array<const char*>* args, Array<const char*>* environmentVariables, const bool8 showArgs = false, const bool8 showStdout = false ) {
	assert( args );
	assert( args->data );
	assert( args->count >= 1 );

	if ( showArgs ) {
		For ( u64, argIndex, 0, args->count ) {
			printf( "%s ", ( *args )[argIndex] );
		}
		printf( "\n" );
	}

	Process* process = process_create( args, environmentVariables );

	// show stdout
	if ( showStdout ) {
		char buffer[1024] = { 0 };
		u64 bytesRead = U64_MAX;

		while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
			buffer[bytesRead] = 0;

			printf( "%s", buffer );
		}
	}

	s32 exitCode = process_join( &process );

	process_destroy( process );
	process = NULL;

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
		"        This file can either be a C++ code file (which is recommended) or a \"" BUILD_INFO_FILE_EXTENSION "\" file.\n"
		"\n"
		"    " ARG_CONFIG "<config> (optional):\n"
		"        Sets the config to whatever you specify.  The config value can be whatever you want.\n"
		"        By settings this here you can then use it inside \"" SET_BUILDER_OPTIONS_FUNC_NAME "\".\n"
		"\n"
		"    " ARG_NUKE " <folder> (optional):\n"
		"        Deletes every file in <folder> and all subfolders.\n"
		"\n"
	);

	return exitCode;
}

static const char* BinaryTypeToString( const BinaryType type ) {
	switch ( type ) {
		case BINARY_TYPE_EXE:				return "BINARY_TYPE_EXE";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return "BINARY_TYPE_DYNAMIC_LIBRARY";
		case BINARY_TYPE_STATIC_LIBRARY:	return "BINARY_TYPE_STATIC_LIBRARY";
	}
}

static const char* OptimizationLevelToString( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0: return "-O0";
		case OPTIMIZATION_LEVEL_O1: return "-O1";
		case OPTIMIZATION_LEVEL_O2: return "-O2";
		case OPTIMIZATION_LEVEL_O3: return "-O3";
	}
}

static s32 BuildEXE( buildContext_t* context ) {
	Array<const char*> args;
	args.reserve(
		1 +	// clang
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->config.source_files.size() +
		context->config.defines.size() +
		context->config.additional_includes.size() +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size() +
		5 +	// warning levels
		context->config.ignore_warnings.size()
	);

	args.add( tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		if ( string_ends_with( context->config.source_files[sourceFileIndex], ".cpp" ) ) {
			args.add( "-std=c++20" );
			break;
		}
	}

	if ( !context->config.remove_symbols ) {
		args.add( "-g" );
	}

	args.add( OptimizationLevelToString( context->config.optimization_level ) );

	args.add( "-o" );
	args.add( context->fullBinaryName );

	args.add_range( context->config.source_files.data(), context->config.source_files.size() );

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		args.add( tprintf( "-D%s", context->config.defines[defineIndex] ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
		args.add( tprintf( "-L%s", context->config.additional_lib_paths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
		args.add( tprintf( "-l%s", context->config.additional_libs[libIndex] ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		args.add( "-Werror" );
	}

	args.add( "-Weverything" );
	args.add( "-Wall" );
	args.add( "-Wextra" );
	args.add( "-Wpedantic" );

	if ( context->config.ignore_warnings.size() > 0 ) {
		args.add_range( context->config.ignore_warnings.data(), context->config.ignore_warnings.size() );
	}

	//args.add( NULL );

	bool8 showArgs = context->flags & BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS;
	bool8 showStdout = context->flags & BUILD_CONTEXT_FLAG_SHOW_STDOUT;
	s32 exitCode = RunProc( &args, NULL, showArgs, showStdout );

	if ( exitCode != 0 ) {
		error( "Compile failed.\n" );
	}

	return exitCode;
}

static s32 BuildDynamicLibrary( buildContext_t* context ) {
	Array<const char*> args;
	args.reserve(
		1 +	// clang
		1 +	// -shared
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->config.source_files.size() +
		context->config.defines.size() +
		context->config.additional_includes.size() +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size() +
		5 +	// warning levels
		context->config.ignore_warnings.size()
	);

	args.add( tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
	args.add( "-shared" );

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		if ( string_ends_with( context->config.source_files[sourceFileIndex], ".cpp" ) ) {
			args.add( "-std=c++20" );
			break;
		}
	}

	if ( !context->config.remove_symbols ) {
		args.add( "-g" );
	}

	args.add( OptimizationLevelToString( context->config.optimization_level ) );

	args.add( "-o" );
	args.add( context->fullBinaryName );

	args.add_range( context->config.source_files.data(), context->config.source_files.size() );

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		args.add( tprintf( "-D%s", context->config.defines[defineIndex] ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
		args.add( tprintf( "-L%s", context->config.additional_lib_paths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
		args.add( tprintf( "-l%s", context->config.additional_libs[libIndex] ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		args.add( "-Werror" );
	}

	args.add( "-Weverything" );
	args.add( "-Wall" );
	args.add( "-Wextra" );
	args.add( "-Wpedantic" );

	if ( context->config.ignore_warnings.size() > 0 ) {
		args.add_range( context->config.ignore_warnings.data(), context->config.ignore_warnings.size() );
	}

	bool8 showArgs = context->flags & BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS;
	bool8 showStdout = context->flags & BUILD_CONTEXT_FLAG_SHOW_STDOUT;
	s32 exitCode = RunProc( &args, NULL, showArgs, showStdout );

	if ( exitCode != 0 ) {
		error( "Compile failed.\n" );
	}

	return exitCode;
}

static s32 BuildStaticLibrary( buildContext_t* context ) {
	bool8 showArgs = context->flags & BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS;
	bool8 showStdout = context->flags & BUILD_CONTEXT_FLAG_SHOW_STDOUT;

	s32 exitCode = 0;

	Array<const char*> intermediateFiles;

	Array<const char*> args;
	args.reserve(
		1 +	// clang
		1 +	// -c
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->config.source_files.size() +
		context->config.defines.size() +
		context->config.additional_includes.size() +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size() +
		5 +	// warning levels
		context->config.ignore_warnings.size()
	);

	// build .o files of all compilation units
	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		const char* sourceFile = context->config.source_files[sourceFileIndex];

		args.reset();

		args.add( tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
		args.add( "-c" );

		if ( string_ends_with( context->config.source_files[sourceFileIndex], ".cpp" ) ) { // TODO(DM): 04/01/2025: IsSourceFile( sourceFile )
			args.add( "-std=c++20" );
		} else if ( string_ends_with( context->config.source_files[sourceFileIndex], ".c" ) ) {
			args.add( "-std=c99" );
		} else {
			assertf( false, "Something went really wrong.\n" );
			QuitError();
		}

		if ( !context->config.remove_symbols ) {
			args.add( "-g" );
		}

		args.add( OptimizationLevelToString( context->config.optimization_level ) );

		{
			const char* sourceFileTrimmed = context->config.source_files[sourceFileIndex];
			sourceFileTrimmed = strrchr( sourceFileTrimmed, '\\' ) + 1;

			const char* outArg = tprintf( "%s\\%s.o", context->config.binary_folder.c_str(), sourceFileTrimmed );

			args.add( "-o" );
			args.add( outArg );
			intermediateFiles.add( outArg );
		}

		args.add( sourceFile );

		For ( u32, defineIndex, 0, context->config.defines.size() ) {
			args.add( tprintf( "-D%s", context->config.defines[defineIndex] ) );
		}

		For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
			args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
		}

		// must come before ignored warnings
		if ( context->config.warnings_as_errors ) {
			args.add( "-Werror" );
		}

		args.add( "-Weverything" );
		args.add( "-Wall" );
		args.add( "-Wextra" );
		args.add( "-Wpedantic" );

		if ( context->config.ignore_warnings.size() > 0 ) {
			args.add_range( context->config.ignore_warnings.data(), context->config.ignore_warnings.size() );
		}

		exitCode = RunProc( &args, NULL, showArgs, showStdout );

		if ( exitCode != 0 ) {
			error( "Compile failed.\n" );
		}
	}

	if ( exitCode != 0 ) {
		return exitCode;
	}

	// link step
	{
		args.reset();

		args.add( "lld-link" );
		args.add( "/lib" );

		args.add( tprintf( "/OUT:%s", context->fullBinaryName ) );

		args.add_range( intermediateFiles.data, intermediateFiles.count );

		exitCode = RunProc( &args, NULL, showArgs, showStdout );

		if ( exitCode != 0 ) {
			error( "Link failed.\n" );
		}
	}

	return exitCode;
}

static bool8 FileExists( const char* filename ) {
	FileInfo fileInfo = {};
	File file = file_find_first( filename, &fileInfo );

	return file.ptr != INVALID_HANDLE_VALUE;
}

static void NukeFolder( const char* folder, const bool8 verbose ) {
	const char* searchPattern = tprintf( "%s\\*", folder );

	FileInfo fileInfo = {};
	File file = file_find_first( searchPattern, &fileInfo );

	do {
		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) || fileInfo.filename[0] == 0 ) {
			continue;
		}

		const char* fileFullPath = tprintf( "%s\\%s", folder, fileInfo.filename );

		if ( fileInfo.is_directory ) {
			if ( verbose ) printf( "Found folder %s\n", fileFullPath );

			NukeFolder( fileFullPath, verbose );

			if ( verbose ) printf( "Deleting folder %s\n", fileFullPath );

			if ( !folder_delete( fileFullPath ) ) {
				error( "Nuke failed to delete folder \"%s\".\n", fileFullPath );
			}
		} else {
			if ( verbose ) printf( "Deleting file %s\n", fileFullPath );

			if ( !file_delete( fileFullPath ) ) {
				error( "Nuke failed to delete folder \"%s\".\n", fileFullPath );
			}
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

static bool8 PathHasSlash( const char* path ) {
	bool8 hasSlash = false;

	hasSlash |= strchr( path, '/' ) != NULL;
	hasSlash |= strchr( path, '\\' ) != NULL;

	return hasSlash;
}

static const char* TryFindFile_r( const char* filename, const char* folder ) {
	const char* result = NULL;

	const char* searchPath = tprintf( "%s\\*", folder );

	const char* filenamePath = paths_remove_file_from_path( filename );

	FileInfo fileInfo;
	File firstFile = file_find_first( searchPath, &fileInfo );

	do {
		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) || fileInfo.filename[0] == 0 ) {
			continue;
		}

		const char* fullFilename = NULL;
		if ( PathHasSlash( filename ) ) {
			fullFilename = tprintf( "%s\\%s\\%s", folder, filenamePath, fileInfo.filename );
		} else {
			fullFilename = tprintf( "%s\\%s", folder, fileInfo.filename );
		}

		if ( fileInfo.is_directory ) {
			result = TryFindFile_r( filename, fullFilename );
		}

		if ( result ) {
			return result;
		}

		if ( string_ends_with( fullFilename, filename ) ) {
			result = fullFilename;
			return result;
		}
	} while ( file_find_next( &firstFile, &fileInfo ) );

	return NULL;
}

static void FindAllFilesInFolder_r( const char* basePath, const char* folder, const char* fileExtension, const bool8 recursive, Array<const char*>* outFiles ) {
	const char* fullSearchPath = NULL;
	if ( string_ends_with( basePath, "\\" ) || string_ends_with( basePath, "/" ) ) {
		if ( folder ) {
			fullSearchPath = tprintf( "%s%s*", basePath, folder );
		} else {
			fullSearchPath = tprintf( "%s*", basePath );
		}
	} else {
		if ( folder ) {
			fullSearchPath = tprintf( "%s\\%s\\*", basePath, folder );
		} else {
			fullSearchPath = tprintf( "%s\\*", basePath );
		}
	}

	FileInfo fileInfo;
	File file = file_find_first( fullSearchPath, &fileInfo );

	do {
		// assume this means folder contains no files of this type
		if ( file.ptr == INVALID_HANDLE_VALUE ) {
			continue;
		}

		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
			continue;
		}

		if ( fileInfo.is_directory && recursive ) {
			const char* subfolder = tprintf( "%s\\%s", folder, fileInfo.filename );
			FindAllFilesInFolder_r( basePath, subfolder, fileExtension, recursive, outFiles );
		}

		if ( string_ends_with( fileInfo.filename, fileExtension ) ) {
			const char* fullName = tprintf( "%s\\%s", folder, fileInfo.filename );

			outFiles->add( fullName );
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

static void GetAllSourceFiles_r( const char* basePath, const char* folder, const char* searchFilter, std::vector<const char*>* outSourceFiles ) {
	// find all files in current directory
	{
		const char* fullSearchPath = NULL;
		if ( folder ) {
			fullSearchPath = tprintf( "%s\\%s\\%s", basePath, folder, searchFilter );
		} else {
			fullSearchPath = tprintf( "%s\\%s", basePath, searchFilter );
		}

		FileInfo fileInfo;
		File firstFile = file_find_first( fullSearchPath, &fileInfo );

		if ( firstFile.ptr != INVALID_HANDLE_VALUE ) {
			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				const char* fullName = NULL;
				if ( folder ) {
					fullName = tprintf( "%s\\%s", folder, fileInfo.filename );
				} else {
					fullName = tprintf( "%s", fileInfo.filename );
				}

				outSourceFiles->push_back( fullName );
			} while ( file_find_next( &firstFile, &fileInfo ) );
		}
	}

	// now go through all other directories
	{
		const char* fullSearchPath = NULL;
		if ( folder ) {
			fullSearchPath = tprintf( "%s\\%s\\*", basePath, folder );
		} else {
			fullSearchPath = tprintf( "%s\\*", basePath );
		}

		FileInfo fileInfo;
		File firstFile = file_find_first( fullSearchPath, &fileInfo );

		if ( firstFile.ptr != INVALID_HANDLE_VALUE ) {
			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( fileInfo.is_directory ) {
					const char* subfolder = NULL;
					if ( folder ) {
						subfolder = tprintf( "%s\\%s", folder, fileInfo.filename );
					} else {
						subfolder = tprintf( "%s", fileInfo.filename );
					}

					GetAllSourceFiles_r( basePath, subfolder, searchFilter, outSourceFiles );
				}
			} while ( file_find_next( &firstFile, &fileInfo ) );
		}
	}
}

static void GetAllSubfolders_r( const char* basePath, const char* folder, Array<const char*>* outSubfolders ) {
	const char* fullSearchPath = NULL;
	if ( string_ends_with( basePath, "\\" ) || string_ends_with( basePath, "/" ) ) {
		if ( folder ) {
			fullSearchPath = tprintf( "%s\\%s*", basePath, folder );
		} else {
			fullSearchPath = tprintf( "%s*", basePath );
		}
	} else {
		if ( folder ) {
			fullSearchPath = tprintf( "%s\\%s\\*", basePath, folder );
		} else {
			fullSearchPath = tprintf( "%s\\*", basePath );
		}
	}

	FileInfo fileInfo;
	File file = file_find_first( fullSearchPath, &fileInfo );

	do {
		// assume this means folder contains no files of this type
		if ( file.ptr == INVALID_HANDLE_VALUE ) {
			continue;
		}

		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
			continue;
		}

		const char* fullName = NULL;
		if ( folder ) {
			fullName = tprintf( "%s\\%s", folder, fileInfo.filename );
		} else {
			fullName = tprintf( "%s", fileInfo.filename );
		}

		if ( fileInfo.is_directory ) {
			outSubfolders->add( fullName );

			GetAllSubfolders_r( basePath, fullName, outSubfolders );
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

static std::vector<const char*> BuildConfig_GetAllSourceFiles( const BuildConfig* config, const char* inputFile ) {
	const char* inputFilePath = paths_remove_file_from_path( inputFile );

	std::vector<const char*> sourceFiles;

	For ( u64, sourceFileIndex, 0, config->source_files.size() ) {
		const char* sourceFile = config->source_files[sourceFileIndex];

		bool8 inputFileIsSameAsSourceFile = string_equals( sourceFile, inputFile );

		const char* sourceFileNoPath = paths_remove_path_from_file( sourceFile );

		if ( inputFileIsSameAsSourceFile ) {
			GetAllSourceFiles_r( inputFilePath, NULL, sourceFileNoPath, &sourceFiles );
		} else {
			const char* sourceFilePath = paths_remove_file_from_path( sourceFile );

			GetAllSourceFiles_r( inputFilePath, sourceFilePath, sourceFileNoPath, &sourceFiles );
		}
	}

	return sourceFiles;
}

// include paths can be wrong and therefore those files cant be resolved (the user writes a path to an include that doesnt exist) but we shouldnt crash if thats the case
// that should be handled by the compiler which throws the proper error
static void GetAllIncludedFiles( const BuildConfig* config, const char* inputFile, const bool8 verbose, Array<const char*>* outBuildInfoFiles ) {
	const char* inputFilePath = paths_remove_file_from_path( inputFile );

	std::vector<const char*> sourceFiles = BuildConfig_GetAllSourceFiles( config, inputFile );

	outBuildInfoFiles->reserve( sourceFiles.size() );

	// for each file, open it and get every include inside it
	// then go through _those_ included files
	For ( u64, sourceFileIndex, 0, sourceFiles.size() ) {
		const char* sourceFile = sourceFiles[sourceFileIndex];

		const char* sourceFileWithInputFilePath = NULL;
		if ( paths_is_path_absolute( sourceFile ) ) {
			sourceFileWithInputFilePath = sourceFile;
		} else {
			sourceFileWithInputFilePath = tprintf( "%s\\%s", inputFilePath, sourceFile );
		}

		//const char* sourceFilePath = paths_remove_file_from_path( sourceFileWithInputFilePath );
		const char* sourceFilePath = paths_remove_file_from_path( sourceFile );
		const char* sourceFileNoPath = paths_remove_path_from_file( sourceFileWithInputFilePath );

		char* fileBuffer = NULL;
		u64 fileLength = 0;
		bool8 read = file_read_entire( sourceFileWithInputFilePath, &fileBuffer, &fileLength );

		if ( !read ) {
			if ( verbose ) {
				warning( "Tried to read the file \"%s\", but I couldn't.  Therefore I can't resolve includes for this file.\n", sourceFiles[sourceFileIndex] );
			}
			continue;
		}

		defer( file_free_buffer( &fileBuffer ) );

		outBuildInfoFiles->add( sourceFile );

		char* seek = fileBuffer;

		while ( *seek ) {
			// ignore includes that are commented out
			if ( string_starts_with( seek, "//" ) ) {
				seek += 2;	// skip past comment

				while ( *seek == ' ' ) {
					seek++;
				}

				if ( string_starts_with( seek, "#include" ) ) {
					seek += strlen( "#include" );
				}
			}

			if ( string_starts_with( seek, "#include" ) ) {
				seek += strlen( "#include" );

				while ( *seek == ' ' ) {
					seek++;
				}

				if ( *seek == '"' ) {
					// "local" include, so scan from where we are
					const char* includeStart = seek;
					includeStart++;

					const char* includeEnd = strchr( includeStart, '"' );

					u64 filenameLength = cast( u64 ) includeEnd - cast( u64 ) includeStart;

					char* filename = cast( char* ) mem_temp_alloc( ( filenameLength + 1 ) * sizeof( char ) );
					strncpy( filename, includeStart, filenameLength * sizeof( char ) );
					filename[filenameLength] = 0;

					filename = tprintf( "%s\\%s", sourceFilePath, filename );
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
					filename = cast( char* ) paths_canonicalise_path( filename );
#pragma clang diagnostic push

					bool8 found = false;
					For ( u64, fileIndex, 0, sourceFiles.size() ) {
						if ( string_equals( sourceFiles[fileIndex], filename ) ) {
							found = true;
							break;
						}
					}

					if ( !found ) {
						sourceFiles.push_back( filename );
					}
				} else if ( *seek == '<' ) {
					// "external" include, so scan all the external include folders that we know about
					const char* includeStart = seek;
					includeStart++;

					const char* includeEnd = strchr( includeStart, '>' );

					u64 filenameLength = cast( u64 ) includeEnd - cast( u64 ) includeStart;
					filenameLength++;

					char* filename = cast( char* ) mem_temp_alloc( filenameLength * sizeof( char ) );
					strncpy( filename, includeStart, filenameLength * sizeof( char ) );
					filename[filenameLength - 1] = 0;

					const char* fullFilename = NULL;

					For ( u64, includePathIndex, 0, config->additional_includes.size() ) {
						const char* includePath = config->additional_includes[includePathIndex].c_str();

						fullFilename = TryFindFile_r( filename, includePath );

						if ( fullFilename != NULL ) {
							break;
						}
					}

					if ( fullFilename ) {
						bool8 found = false;
						For ( u64, fileIndex, 0, sourceFiles.size() ) {
							if ( string_equals( sourceFiles[fileIndex], fullFilename ) ) {
								found = true;
								break;
							}
						}

						if ( !found ) {
							sourceFiles.push_back( fullFilename );
						}
					} else {
						if ( verbose ) {
							warning( "Tried to find external include \"%s\" from any of the additional include directories, but couldn't.  This file won't be tracked in the %s file.\n", filename, BUILD_INFO_FILE_EXTENSION );
						}
					}
				}
			}

			seek += 1;
		}
	}
}

static void SerializeU64( File* file, const u64 x ) {
	CHECK_WRITE( file_write( file, &x, sizeof( u64 ) ) );
}

static void SerializeCStringArray( File* file, const std::vector<const char*>& array, const char* name ) {
	CHECK_WRITE( file_write( file, tprintf( "%s\n", name ) ) );
	SerializeU64( file, array.size() );

	For ( u64, i, 0, array.size() ) {
		CHECK_WRITE( file_write( file, tprintf( "%s\n", array[i] ) ) );
	}
}

static void SerializeSTDStringArray( File* file, const std::vector<std::string>& array, const char* name ) {
	CHECK_WRITE( file_write( file, tprintf( "%s\n", name ) ) );
	SerializeU64( file, array.size() );

	For ( u64, i, 0, array.size() ) {
		CHECK_WRITE( file_write( file, tprintf( "%s\n", array[i].c_str() ) ) );
	}
}

static void SerializeBuildInfo( const char* filename, const std::vector<BuildConfig>& configs, const char* inputFile, const char* userConfigSourceFilename, const char* userConfigDLLFilename, const bool8 verbose ) {
	const char* inputFilePath = paths_remove_file_from_path( inputFile );

	File file = file_open_or_create( filename );
	defer( file_close( &file ) );

	CHECK_WRITE( file_write( &file, tprintf( "build_source_file: %s\n", userConfigSourceFilename ) ) );
	CHECK_WRITE( file_write( &file, tprintf( "DLL: %s\n", userConfigDLLFilename ) ) );

	SerializeU64( &file, configs.size() );

	For ( u64, builderOptionsIndex, 0, configs.size() ) {
		const BuildConfig* config = &configs[builderOptionsIndex];

		CHECK_WRITE( file_write( &file, tprintf( "config: %s\n", config->name.c_str() ) ) );

		u64 configNameHash = hash_string( config->name.c_str(), 0 );
		SerializeU64( &file, configNameHash );

		// serialize names of all build dependencies
		{
			CHECK_WRITE( file_write( &file, tprintf( "depends_on\n" ) ) );
			SerializeU64( &file, config->depends_on.size() );

			For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
				CHECK_WRITE( file_write( &file, tprintf( "%s\n", config->depends_on[dependencyIndex].name.c_str() ) ) );
			}
		}

		SerializeCStringArray( &file, config->source_files, "source_files" );
		SerializeCStringArray( &file, config->defines, "defines" );
		SerializeSTDStringArray( &file, config->additional_includes, "additional_includes" );
		SerializeSTDStringArray( &file, config->additional_lib_paths, "additional_lib_paths" );
		SerializeCStringArray( &file, config->additional_libs, "additional_libs" );
		SerializeCStringArray( &file, config->ignore_warnings, "ignore_warnings" );

		CHECK_WRITE( file_write( &file, tprintf( "binary_name: %s\n", config->binary_name.c_str() ) ) );
		CHECK_WRITE( file_write( &file, tprintf( "binary_folder: %s\n", config->binary_folder.c_str() ) ) );

		CHECK_WRITE( file_write( &file, &config->binary_type, sizeof( BinaryType ) ) );
		CHECK_WRITE( file_write( &file, &config->optimization_level, sizeof( OptimizationLevel ) ) );
		CHECK_WRITE( file_write( &file, &config->remove_symbols, sizeof( bool8 ) ) );
		CHECK_WRITE( file_write( &file, &config->remove_file_extension, sizeof( bool8 ) ) );

		// serialize all included files and their last write time
		{
			Array<const char*> buildInfoFiles;
			GetAllIncludedFiles( config, inputFile, verbose, &buildInfoFiles );

			CHECK_WRITE( file_write( &file, "tracked_source_files\n" ) );
			SerializeU64( &file, buildInfoFiles.count );

			For ( u64, buildInfoFileIndex, 0, buildInfoFiles.count ) {
				const char* buildInfoFile = buildInfoFiles[buildInfoFileIndex];

				const char* buildInfoFileAndPath = buildInfoFile;
				if ( !paths_is_path_absolute( buildInfoFileAndPath ) ) {
					buildInfoFileAndPath = tprintf( "%s\\%s", inputFilePath, buildInfoFile );
				}

				FileInfo fileInfo;
				File foundFile = file_find_first( buildInfoFileAndPath, &fileInfo );

				assert( foundFile.ptr != INVALID_HANDLE_VALUE );

				// dont write full path of the filename
				// the path to the source file can change depending on whether we are building from visual studio or the command line
				// so just write the filename relative to the source folder and let Builder reconstruct the appropriate path as and when it needs to
				CHECK_WRITE( file_write( &file, tprintf( "%s\n", buildInfoFile ) ) );
				SerializeU64( &file, fileInfo.last_write_time );
			}
		}

		CHECK_WRITE( file_write( &file, "\n" ) );
	}
}

struct parser_t {
	u64		fileLength;
	char*	bufferStart;
	char*	bufferPos;
};

static bool8 Parser_Init( parser_t* parser, const char* filename ) {
	memset( parser, 0, sizeof( parser_t ) );

	bool8 read = file_read_entire( filename, &parser->bufferStart, &parser->fileLength );

	parser->bufferPos = parser->bufferStart;

	return read;
}

static void Parser_Shutdown( parser_t* parser ) {
	file_free_buffer( &parser->bufferStart );
}

static void Parser_SkipTo( parser_t* parser, const char c ) {
	while ( *parser->bufferPos != c ) {
		parser->bufferPos++;
	}
}

static void Parser_SkipPast( parser_t* parser, const char c ) {
	Parser_SkipTo( parser, c );
	parser->bufferPos++;
}

static bool8 Parser_ParseBool( parser_t* parser ) {
	bool8* x = cast( bool8* ) parser->bufferPos;

	parser->bufferPos += sizeof( bool8 );

	return *x;
}

static s32 Parser_ParseS32( parser_t* parser ) {
	s32* x = cast( s32* ) parser->bufferPos;

	parser->bufferPos += sizeof( s32 );

	return *x;
}

static u64 Parser_ParseU64( parser_t* parser ) {
	u64* x = cast( u64* ) parser->bufferPos;

	parser->bufferPos += sizeof( u64 );

	return *x;
}

static char* Parser_ParseLine( parser_t* parser ) {
	const char* lineEnd = cast( const char* ) memchr( parser->bufferPos, '\n', parser->fileLength );
	u64 stringLength = cast( u64 ) lineEnd - cast( u64 ) parser->bufferPos;

	char* string = cast( char* ) mem_alloc( ( stringLength + 1 ) * sizeof( char ) );
	memcpy( string, parser->bufferPos, stringLength * sizeof( char ) );
	string[stringLength] = 0;

	Parser_SkipPast( parser, '\n' );

	return string;
}

// a "string field" is just a key value pair and is laid out like this:
//
//	key: value
//
// where "key" and "value" are both strings
static void Parser_ParseStringField( parser_t* parser, char** outKey, std::string* outValue ) {
	const char* colon = cast( const char* ) memchr( parser->bufferPos, ':', parser->fileLength );
	u64 keyLength = cast( u64 ) colon - cast( u64 ) parser->bufferPos;

	if ( outKey ) {
		*outKey = cast( char* ) mem_alloc( ( keyLength + 1 ) * sizeof( char ) );
		memcpy( *outKey, parser->bufferPos, keyLength * sizeof( char ) );
		*outKey[keyLength] = 0;
	}

	parser->bufferPos += keyLength;	// skip to the colon
	parser->bufferPos += 1;			// skip past the colon
	parser->bufferPos += 1;			// skip past following whitespace

	char* value = Parser_ParseLine( parser );

	if ( outValue ) {
		*outValue = value;
	}
}

static std::vector<const char*> Parser_ParseCStringArray( parser_t* parser ) {
	Parser_SkipPast( parser, '\n' );	// skip the name of the array

	u64 arrayCount = Parser_ParseU64( parser );
	std::vector<const char*> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = Parser_ParseLine( parser );
	}

	return result;
}

static std::vector<std::string> Parser_ParseSTDStringArray( parser_t* parser ) {
	Parser_SkipPast( parser, '\n' );	// skip the name of the array

	u64 arrayCount = Parser_ParseU64( parser );
	std::vector<std::string> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = Parser_ParseLine( parser );
	}

	return result;
}

struct buildInfoFileData_t {
	std::vector<buildInfoConfig_t>	configs;
	std::string						userConfigSourceFilename;
	std::string						userConfigDLLFilename;
};

static bool8 Parser_ParseBuildInfo( const char* buildInfoFilename, buildInfoFileData_t* outData ) {
	parser_t parser = {};
	bool8 read = Parser_Init( &parser, buildInfoFilename );

	if ( !read ) {
		return false;
	}

	defer( Parser_Shutdown( &parser ) );

	Parser_ParseStringField( &parser, NULL, &outData->userConfigSourceFilename );
	Parser_ParseStringField( &parser, NULL, &outData->userConfigDLLFilename );

	// parse all BuilderOptions
	{
		u64 totalNumConfigs = Parser_ParseU64( &parser );

		outData->configs.resize( totalNumConfigs );

		For ( u64, configIndex, 0, outData->configs.size() ) {
			buildInfoConfig_t* buildInfoConfig = &outData->configs[configIndex];

			// parse the config itself
			{
				BuildConfig* config = &buildInfoConfig->config;

				Parser_ParseStringField( &parser, NULL, &config->name );
				Parser_ParseU64( &parser );	// name hash, skip

				{
					std::vector<const char*> dependencyNames = Parser_ParseCStringArray( &parser );

					config->depends_on.resize( dependencyNames.size() );

					For ( u64, dependencyIndex, 0, dependencyNames.size() ) {
						config->depends_on[dependencyIndex].name = dependencyNames[dependencyIndex];
					}
				}

				config->source_files = Parser_ParseCStringArray( &parser );
				config->defines = Parser_ParseCStringArray( &parser );
				config->additional_includes = Parser_ParseSTDStringArray( &parser );
				config->additional_lib_paths = Parser_ParseSTDStringArray( &parser );
				config->additional_libs = Parser_ParseCStringArray( &parser );
				config->ignore_warnings = Parser_ParseCStringArray( &parser );

				Parser_ParseStringField( &parser, NULL, &config->binary_name );
				Parser_ParseStringField( &parser, NULL, &config->binary_folder );

				config->binary_type = cast( BinaryType ) Parser_ParseS32( &parser );
				config->optimization_level = cast( OptimizationLevel ) Parser_ParseS32( &parser );
				config->remove_symbols = Parser_ParseBool( &parser );
				config->remove_file_extension = Parser_ParseBool( &parser );
			}

			// parse tracked source files
			{
				Parser_SkipPast( &parser, '\n' );

				u64 count = Parser_ParseU64( &parser );
				buildInfoConfig->files.reserve( count );

				For ( u64, fileIndex, 0, count ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder-init-list"
					trackedSourceFile_t buildInfoFile = {};
					buildInfoFile.filename = Parser_ParseLine( &parser );
					buildInfoFile.lastWriteTime = Parser_ParseU64( &parser );
#pragma clang diagnostic pop

					buildInfoConfig->files.push_back( buildInfoFile );
				}
			}

			Parser_SkipPast( &parser, '\n' );
		}
	}

	// reconstruct all build dependencies after we have deserialized them all from the file
	For ( u64, configIndex, 0, outData->configs.size() ) {
		BuildConfig* config = &outData->configs[configIndex].config;

		For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
			BuildConfig* dependency = &config->depends_on[dependencyIndex];

			bool8 found = false;

			For ( u64, otherConfigIndex, 0, outData->configs.size() ) {
				BuildConfig* otherConfig = &outData->configs[otherConfigIndex].config;
				if ( otherConfig->name == dependency->name ) {
					config->depends_on[dependencyIndex] = *otherConfig;

					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "Failed to find the build dependency \"%s\".\n", dependency->name.c_str() );	// TODO(DM): 26/11/2024: better error msg here!
				return false;
			}
		}
	}

	return true;
}

static const char* BuildConfig_ToString( const BuildConfig* config ) {
	StringBuilder builder = {};
	string_builder_reset( &builder );

	string_builder_appendf( &builder, "%s: {\n", config->name.c_str() );

	auto PrintCStringArray = [&builder]( const char* name, const std::vector<const char*>& array ) {
		if ( array.size() > 0 ) {
			string_builder_appendf( &builder, "\t%s: { ", name );
			For ( u64, i, 0, array.size() ) {
				string_builder_appendf( &builder, "%s", array[i] );

				if ( i < array.size() - 1 ) {
					string_builder_appendf( &builder, ", " );
				}
			}
			string_builder_appendf( &builder, " }\n" );
		}
	};

	auto PrintSTDStringArray = [&builder]( const char* name, const std::vector<std::string>& array ) {
		if ( array.size() > 0 ) {
			string_builder_appendf( &builder, "\t%s: { ", name );
			For ( u64, i, 0, array.size() ) {
				string_builder_appendf( &builder, "%s", array[i].c_str() );

				if ( i < array.size() - 1 ) {
					string_builder_appendf( &builder, ", " );
				}
			}
			string_builder_appendf( &builder, " }\n" );
		}
	};

	auto PrintField = [&builder]( const char* key, const char* value ) {
		string_builder_appendf( &builder, "\t%s: %s\n", key, value );
	};

	if ( config->depends_on.size() > 0 ) {
		string_builder_appendf( &builder, "\tdepends_on: { " );
		For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
			string_builder_appendf( &builder, "%s", config->depends_on[dependencyIndex].name.c_str() );

			if ( dependencyIndex < config->depends_on.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	}

	PrintCStringArray( "source_files", config->source_files );
	PrintCStringArray( "defines", config->defines );
	PrintSTDStringArray( "additional_includes", config->additional_includes );
	PrintSTDStringArray( "additional_lib_paths", config->additional_lib_paths );
	PrintCStringArray( "additional_libs", config->additional_libs );
	PrintCStringArray( "ignore_warnings", config->ignore_warnings );

	PrintField( "binary_name", config->binary_name.c_str() );
	PrintField( "binary_folder", config->binary_folder.c_str() );
	PrintField( "binary_type", BinaryTypeToString( config->binary_type ) );
	PrintField( "optimization_level", OptimizationLevelToString( config->optimization_level ) );
	PrintField( "remove_symbols", config->remove_symbols ? "true" : "false" );
	PrintField( "remove_file_extension", config->remove_file_extension ? "true" : "false" );
	PrintField( "warnings_as_errors", config->warnings_as_errors ? "true" : "false" );

	string_builder_appendf( &builder, "}\n" );

	return string_builder_to_string( &builder );
}

static void BuildConfig_AddDefaults( BuildConfig* outConfig ) {
	// add the folder that builder lives in as an additional include path
	// so that people can just include builder.h without having to add the include path manually every time
	outConfig->additional_includes.push_back( paths_get_app_path() );

	outConfig->additional_includes.push_back( "." );

#if defined( _WIN64 )
	outConfig->additional_libs.push_back( "user32.lib" );

	// MSVCRT is needed for ABI compatibility between builder and the user config DLL
#if defined( _DEBUG )
	outConfig->additional_libs.push_back( "msvcrtd.lib" );
#elif defined( NDEBUG )
	outConfig->additional_libs.push_back( "msvcrt.lib" );
#endif
#endif // defined( _WIN64 )

	outConfig->ignore_warnings.push_back( "-Wno-newline-eof" );
	outConfig->ignore_warnings.push_back( "-Wno-pointer-integer-compare" );
	outConfig->ignore_warnings.push_back( "-Wno-declaration-after-statement" );
	outConfig->ignore_warnings.push_back( "-Wno-gnu-zero-variadic-macro-arguments" );
	outConfig->ignore_warnings.push_back( "-Wno-cast-align" );
	outConfig->ignore_warnings.push_back( "-Wno-bad-function-cast" );
	outConfig->ignore_warnings.push_back( "-Wno-format-nonliteral" );
	outConfig->ignore_warnings.push_back( "-Wno-missing-braces" );
	outConfig->ignore_warnings.push_back( "-Wno-switch-enum" );
	outConfig->ignore_warnings.push_back( "-Wno-covered-switch-default" );
	outConfig->ignore_warnings.push_back( "-Wno-double-promotion" );
	outConfig->ignore_warnings.push_back( "-Wno-cast-qual" );
	outConfig->ignore_warnings.push_back( "-Wno-unused-variable" );
	outConfig->ignore_warnings.push_back( "-Wno-unused-function" );
	outConfig->ignore_warnings.push_back( "-Wno-empty-translation-unit" );
	outConfig->ignore_warnings.push_back( "-Wno-zero-as-null-pointer-constant" );
	outConfig->ignore_warnings.push_back( "-Wno-c++98-compat-pedantic" );
	outConfig->ignore_warnings.push_back( "-Wno-unused-macros" );
	outConfig->ignore_warnings.push_back( "-Wno-unsafe-buffer-usage" );			// LLVM 17.0.1
	outConfig->ignore_warnings.push_back( "-Wno-reorder-init-list" );			// C++: "designated initializers must be in order"
	outConfig->ignore_warnings.push_back( "-Wno-old-style-cast" );				// C++: "C-style casts are banned"
	outConfig->ignore_warnings.push_back( "-Wno-global-constructors" );			// C++: "declaration requires a global destructor"
	outConfig->ignore_warnings.push_back( "-Wno-exit-time-destructors" );		// C++: "declaration requires an exit-time destructor" (same as the above, basically)
	outConfig->ignore_warnings.push_back( "-Wno-missing-field-initializers" );	// LLVM 18.1.8
}

static const char* BuildConfig_GetFullBinaryName( BuildConfig* config ) {
	assert( !config->binary_folder.empty() );
	assert( !config->binary_name.empty() );

	StringBuilder builder = {};
	string_builder_reset( &builder );

	string_builder_appendf( &builder, "%s\\%s", config->binary_folder.c_str(), config->binary_name.c_str() );

	if ( !config->remove_file_extension ) {
		string_builder_appendf( &builder, ".%s", GetFileExtensionFromBinaryType( config->binary_type ) );
	}

	return string_builder_to_string( &builder );
}

// data layout comes from: https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
static const char* CreateVisualStudioGuid() {
	GUID guid;
	HRESULT hr = CoCreateGuid( &guid );
	assert( hr == S_OK );

	return tprintf( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}

static bool8 GenerateVisualStudioSolution( VisualStudioSolution* solution, const char* inputFilename, const char* userConfigSourceFilename, const char* userConfigBuildDLLFilename, const bool8 verbose ) {
	assert( solution );

	const char* inputFilePath = paths_remove_file_from_path( inputFilename );

	// TODO(DM): 18/11/2024: dont use abs path here
	const char* buildInfoFilename = tprintf( "%s\\.builder\\%s%s", inputFilePath, solution->name, BUILD_INFO_FILE_EXTENSION );

	// validate the solution
	{
		if ( solution->name == NULL ) {
			error( "You never set the name of the solution.  I need that.\n" );
			return false;
		}

		if ( solution->platforms.size() < 1 ) {
			error( "You must set at least one platform when generating a Visual Studio Solution.\n" );
			return false;
		}

		if ( solution->projects.size() < 1 ) {
			error( "As well as a Solution, you must also generate at least one Visual Studio Project to go with it.\n" );
			return false;
		}

		// validate platforms
		// turns out visual studio REALLY cares what the names of the platforms are
		// if you specify "Win32" or "x64" as a platform name then VS is able to generate defaults for your project which include things like Windows SDK directories, and your PATH
		{
			bool8 foundDefaultPlatformName = false;

			const char* defaultPlatformNames[] = {
				"Win32",
				"x64"
			};

			For ( u64, platformIndex, 0, solution->platforms.size() ) {
				const char* platform = solution->platforms[platformIndex];

				For ( u64, defaultPlatformIndex, 0, count_of( defaultPlatformNames ) ) {
					const char* defaultPlatformName = defaultPlatformNames[defaultPlatformIndex];

					if ( string_equals( platform, defaultPlatformName ) ) {
						foundDefaultPlatformName = true;
						break;
					}
				}
			}

			if ( !foundDefaultPlatformName ) {
				StringBuilder builder = {};
				string_builder_reset( &builder );
				string_builder_appendf( &builder, "None of your platform names are any of the Visual Studio recognized defaults:\n" );
				For ( u64, platformIndex, 0, count_of( defaultPlatformNames ) ) {
					string_builder_appendf( &builder, "\t- %s\n", defaultPlatformNames[platformIndex] );
				}
				string_builder_appendf( &builder, "Visual Studio relies on those specific names in order to generate fields like \"Executable Path\" properly (for example).\n" );
				string_builder_appendf( &builder, "Builder will still generate the solution, but know that not setting at least one platform name to any of these defaults will cause certain fields in the property pages of your Visual Studio project to not be correct.\n" );
				string_builder_appendf( &builder, "You have been warned.\n" );

				warning( string_builder_to_string( &builder ) );
			}
		}
	}

	const char* visualStudioProjectFilesPath = NULL;
	if ( solution->path ) {
		visualStudioProjectFilesPath = tprintf( "%s\\%s", inputFilePath, solution->path );
	} else {
		visualStudioProjectFilesPath = inputFilePath;
	}
	visualStudioProjectFilesPath = paths_canonicalise_path( visualStudioProjectFilesPath );

	const char* solutionFilename = tprintf( "%s\\%s.sln", visualStudioProjectFilesPath, solution->name );

	// give each project a guid
	Array<const char*> projectGuids;
	projectGuids.resize( solution->projects.size() );

	For ( u64, guidIndex, 0, projectGuids.count ) {
		projectGuids[guidIndex] = CreateVisualStudioGuid();
	}

	if ( !folder_create_if_it_doesnt_exist( visualStudioProjectFilesPath ) ) {
		errorCode_t errorCode = GetLastErrorCode();
		error( "Failed to create the Visual Studio Solution folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );

		return false;
	}

	// generate each .vcxproj
	For ( u64, projectIndex, 0, solution->projects.size() ) {
		VisualStudioProject* project = &solution->projects[projectIndex];

		// validate the project
		{
			if ( project->name == NULL ) {
				error( "There is a Visual Studio Project that doesn't have a name here.  You need to fill that in.\n" );
				return false;
			}

			if ( project->code_folders.size() == 0 ) {
				error( "No code folders were provided for project \"%s\".  You need at least one.\n", project->name );
				return false;
			}

			if ( project->file_extensions.size() == 0 ) {
				error( "No file extensions/file types were provided for project \"%s\".  You need at least one.\n", project->name );
				return false;
			}

			// validate each config
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				if ( config->name == NULL ) {
					error( "There is a config for project \"%s\" that doesn't have a name here.  You need to fill that in.\n", project->name );
					return false;
				}

				if ( config->options.name.empty() ) {
					error( "There is a config for project \"%s\" that doesn't have a name set in it's BuildConfig.  You need to fill that in.\n", project->name );
					return false;
				}

				if ( config->options.binary_type == BINARY_TYPE_EXE && config->options.binary_folder.empty() ) {
					error(
						"Build config \"%s\" is an executable, but you never specified an output directory when generating the Visual Studio project \"%s\", config \"%s\".\n"
						"Visual Studio needs this in order to know where to run the executable from when debugging.  You need to set this.\n"
						, config->options.name.c_str(), project->name, config->name
					);
					return false;
				}
			}
		}

		// .vcxproj
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj", visualStudioProjectFilesPath, project->name );

			printf( "Generating %s ... ", projectPath );

			File vcxproj = file_open_or_create( projectPath );
			defer( file_close( &vcxproj ) );

			CHECK_WRITE( file_write_line( &vcxproj, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" ) );

			// generate every single config and platform pairing
			{
				CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup Label=\"ProjectConfigurations\">" ) );
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, solution->platforms.size() ) {
						const char* platform = solution->platforms[platformIndex];

						// TODO: Alternative targets
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ProjectConfiguration Include=\"%s|%s\">", config->name, platform ) ) );
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<Configuration>%s</Configuration>", config->name ) ) );
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<Platform>%s</Platform>", platform ) ) );
						CHECK_WRITE( file_write_line( &vcxproj,          "\t\t</ProjectConfiguration>" ) );
					}
				}
				CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
			}

			// project globals
			{
				CHECK_WRITE( file_write_line( &vcxproj,          "\t<PropertyGroup Label=\"Globals\">" ) );
				CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<VCProjectVersion>17.0</VCProjectVersion>" ) );
				CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ProjectGuid>{%s}</ProjectGuid>", projectGuids[projectIndex] ) ) );
				CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>" ) );
				CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<Keyword>MakeFileProj</Keyword>" ) );
				CHECK_WRITE( file_write_line( &vcxproj,          "\t</PropertyGroup>" ) );
			}

			CHECK_WRITE( file_write_line( &vcxproj, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />" ) );

			// for each config and platform, define config type, toolset, out dir, and intermediate dir
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				const char* fullBinaryName = BuildConfig_GetFullBinaryName( &config->options );

				const char* from = solutionFilename;
				const char* to = tprintf( "%s\\%s", inputFilePath, fullBinaryName );
				to = paths_canonicalise_path( to );

				char* pathFromSolutionToBinary = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
				memset( pathFromSolutionToBinary, 0, MAX_PATH * sizeof( char ) );
				PathRelativePathTo( pathFromSolutionToBinary, from, FILE_ATTRIBUTE_NORMAL, to, FILE_ATTRIBUTE_DIRECTORY );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
				pathFromSolutionToBinary = cast( char* ) paths_remove_file_from_path( pathFromSolutionToBinary );
#pragma clang diagnostic pop

				For ( u64, platformIndex, 0, solution->platforms.size() ) {
					const char* platform = solution->platforms[platformIndex];

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\" Label=\"Configuration\">", config->name, platform ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<ConfigurationType>Makefile</ConfigurationType>" ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<UseDebugLibraries>false</UseDebugLibraries>" ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<PlatformToolset>v143</PlatformToolset>" ) );

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<OutDir>%s</OutDir>", pathFromSolutionToBinary ) ) );
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<IntDir>%s</IntDir>", tprintf( "%s\\intermediate", config->options.binary_folder.c_str() ) ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t</PropertyGroup>" ) );
				}
			}

			CHECK_WRITE( file_write_line( &vcxproj, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />" ) );

			// not sure what this is or why we need this one but visual studio seems to want it
			CHECK_WRITE( file_write_line( &vcxproj, "\t<ImportGroup Label=\"ExtensionSettings\">" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "\t</ImportGroup>" ) );

			// for each config and platform, import the property sheets that visual studio requires
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, solution->platforms.size() ) {
					const char* platform = solution->platforms[platformIndex];

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<ImportGroup Label=\"PropertySheets\" Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">", config->name, platform ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />" ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t</ImportGroup>" ) );
				}
			}

			// not sure what this is or why we need this one but visual studio seems to want it
			CHECK_WRITE( file_write_line( &vcxproj, "\t<PropertyGroup Label=\"UserMacros\" />" ) );
		
			// for each config and platform, set the following:
			//	external include paths
			//	external library paths
			//	output path
			//	build command
			//	rebuild command
			//	clean command
			//	preprocessor definitions
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, solution->platforms.size() ) {
					const char* platform = solution->platforms[platformIndex];

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">", config->name, platform ) ) );

					// external include paths
					CHECK_WRITE( file_write( &vcxproj, "\t\t<ExternalIncludePath>" ) );
					For ( u64, includePathIndex, 0, config->options.additional_includes.size() ) {
						CHECK_WRITE( file_write( &vcxproj, tprintf( "%s;", config->options.additional_includes[includePathIndex].c_str() ) ) );
					}
					CHECK_WRITE( file_write( &vcxproj, "$(ExternalIncludePath)" ) );
					CHECK_WRITE( file_write( &vcxproj, "</ExternalIncludePath>\n" ) );

					// external library paths
					CHECK_WRITE( file_write( &vcxproj, "\t\t<LibraryPath>" ) );
					For ( u64, libPathIndex, 0, config->options.additional_lib_paths.size() ) {
						CHECK_WRITE( file_write( &vcxproj, tprintf( "%s;", config->options.additional_lib_paths[libPathIndex].c_str() ) ) );
					}
					CHECK_WRITE( file_write( &vcxproj, "$(LibraryPath)" ) );
					CHECK_WRITE( file_write( &vcxproj, "</LibraryPath>\n" ) );

					// output path
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeOutput>%s</NMakeOutput>", config->options.binary_folder.c_str() ) ) );

					char* pathFromSolutionToCode = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
					memset( pathFromSolutionToCode, 0, MAX_PATH * sizeof( char ) );
					PathRelativePathTo( pathFromSolutionToCode, solutionFilename, FILE_ATTRIBUTE_NORMAL, paths_fix_slashes( inputFilename ), FILE_ATTRIBUTE_DIRECTORY );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
					pathFromSolutionToCode = cast( char* ) paths_remove_file_from_path( pathFromSolutionToCode );
#pragma clang diagnostic pop

					const char* buildInfoFileRelative = tprintf( "%s\\.builder\\%s%s", pathFromSolutionToCode, solution->name, BUILD_INFO_FILE_EXTENSION );

					const char* fullConfigName = config->options.name.c_str();

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeBuildCommandLine>%s\\builder.exe %s %s%s</NMakeBuildCommandLine>", paths_get_app_path(), buildInfoFileRelative, ARG_CONFIG, fullConfigName ) ) );
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeReBuildCommandLine>%s\\builder.exe %s %s%s</NMakeReBuildCommandLine>", paths_get_app_path(), buildInfoFileRelative, ARG_CONFIG, fullConfigName ) ) );

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeCleanCommandLine>%s\\builder.exe --nuke %s</NMakeCleanCommandLine>", paths_get_app_path(), config->options.binary_folder.c_str() ) ) );

					// preprocessor definitions
					CHECK_WRITE( file_write( &vcxproj, "\t\t<NMakePreprocessorDefinitions>" ) );
					For ( u64, definitionIndex, 0, config->options.defines.size() ) {
						CHECK_WRITE( file_write( &vcxproj, tprintf( "%s;", config->options.defines[definitionIndex] ) ) );
					}
					CHECK_WRITE( file_write( &vcxproj, "$(NMakePreprocessorDefinitions)" ) );
					CHECK_WRITE( file_write( &vcxproj, "</NMakePreprocessorDefinitions>\n" ) );

					CHECK_WRITE( file_write_line( &vcxproj, "\t</PropertyGroup>" ) );
				}
			}

			CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemDefinitionGroup>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemDefinitionGroup>" ) );

			// tell visual studio what files we have in this project
			// this is typically done via a filter (E.G: src/*.cpp)
			{
				For ( u64, folderIndex, 0, project->code_folders.size() ) {
					const char* folder = project->code_folders[folderIndex];

					For ( u64, fileExtensionIndex, 0, project->file_extensions.size() ) {
						const char* fileExtension = project->file_extensions[fileExtensionIndex];

						if ( string_equals( fileExtension, "cpp" ) ) {	// TODO(DM): 01/01/2025: FileExtensionIsSourceFile( fileExtension )
							Array<const char*> files;
							FindAllFilesInFolder_r( inputFilePath, folder, fileExtension, true, &files );

							if ( files.count > 0 ) {
								CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

								For ( u64, fileIndex, 0, files.count ) {
									CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClCompile Include=\"%s\" />", files[fileIndex] ) ) );
								}

								CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
							}
						} else if ( string_equals( fileExtension, "h" ) ) {
							Array<const char*> files;
							FindAllFilesInFolder_r( inputFilePath, folder, fileExtension, true, &files );

							if ( files.count > 0 ) {
								CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

								For ( u64, fileIndex, 0, files.count ) {
									CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClInclude Include=\"%s\" />", files[fileIndex] ) ) );
								}

								CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
							}
						} else {
							Array<const char*> files;
							FindAllFilesInFolder_r( inputFilePath, folder, fileExtension, true, &files );

							if ( files.count > 0 ) {
								CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

								For ( u64, fileIndex, 0, files.count ) {
									CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<None Include=\"%s\" />", files[fileIndex] ) ) );
								}

								CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
							}
						}
					}
				}
			}

			CHECK_WRITE( file_write_line( &vcxproj, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />" ) );

			// not sure what this is or why we need this one but visual studio seems to want it
			CHECK_WRITE( file_write_line( &vcxproj, "\t<ImportGroup Label=\"ExtensionTargets\">" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "\t</ImportGroup>" ) );

			CHECK_WRITE( file_write_line( &vcxproj, "</Project>" ) );

			printf( "Done\n" );
		}

		// .vcxproj.user
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj.user", visualStudioProjectFilesPath, project->name );

			printf( "Generating %s ... ", projectPath );

			File vcxproj = file_open_or_create( projectPath );
			defer( file_close( &vcxproj ) );

			CHECK_WRITE( file_write_line( &vcxproj, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "<Project ToolsVersion=\"Current\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" ) );

			CHECK_WRITE( file_write_line( &vcxproj, "\t<PropertyGroup>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "\t\t<ShowAllFiles>false</ShowAllFiles>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "\t</PropertyGroup>" ) );

			// for each config and platform, generate the debugger settings
			{
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					const char* fullBinaryName = BuildConfig_GetFullBinaryName( &config->options );

					const char* from = solutionFilename;
					const char* to = tprintf( "%s\\%s", inputFilePath, fullBinaryName );
					to = paths_canonicalise_path( to );

					char* pathFromSolutionToBinary = cast( char* ) mem_temp_alloc( MAX_PATH * sizeof( char ) );
					memset( pathFromSolutionToBinary, 0, MAX_PATH * sizeof( char ) );
					PathRelativePathTo( pathFromSolutionToBinary, from, FILE_ATTRIBUTE_NORMAL, to, FILE_ATTRIBUTE_DIRECTORY );

					For ( u64, platformIndex, 0, solution->platforms.size() ) {
						const char* platform = solution->platforms[platformIndex];

						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">", config->name, platform ) ) );
						CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>" ) );	// TODO(DM): do want to include the other debugger types?
						CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<LocalDebuggerDebuggerType>Auto</LocalDebuggerDebuggerType>" ) );
						CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<LocalDebuggerAttach>false</LocalDebuggerAttach>" ) );
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<LocalDebuggerCommand>%s</LocalDebuggerCommand>", pathFromSolutionToBinary ) ) );
						CHECK_WRITE( file_write_line( &vcxproj,          "\t\t<LocalDebuggerWorkingDirectory>$(SolutionDir)</LocalDebuggerWorkingDirectory>" ) );

						// if debugger arguments were specified, put those in
						if ( config->debugger_arguments.size() > 0 ) {
							CHECK_WRITE( file_write( &vcxproj, "\t\t<LocalDebuggerCommandArguments>" ) );
							For ( u64, argIndex, 0, config->debugger_arguments.size() ) {
								CHECK_WRITE( file_write( &vcxproj, tprintf( "%s " ) ) );
							}
							CHECK_WRITE( file_write( &vcxproj, "</LocalDebuggerCommandArguments>\n" ) );
						}

						CHECK_WRITE( file_write_line( &vcxproj, "\t</PropertyGroup>" ) );
					}
				}
			}

			CHECK_WRITE( file_write_line( &vcxproj, "</Project>" ) );

			printf( "Done\n" );
		}

		// .vcxproj.filter
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj.filters", visualStudioProjectFilesPath, project->name );

			printf( "Generating %s ... ", projectPath );

			File vcxproj = file_open_or_create( projectPath );
			defer( file_close( &vcxproj ) );

			CHECK_WRITE( file_write_line( &vcxproj, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" ) );
			CHECK_WRITE( file_write_line( &vcxproj, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" ) );

			// write all filter guids
			For ( u64, folderIndex, 0, project->code_folders.size() ) {
				const char* folder = tprintf( "%s\\%s", inputFilePath, project->code_folders[folderIndex] );

				Array<const char*> subfolders;
				GetAllSubfolders_r( folder, NULL, &subfolders );

				CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

				For ( u64, subfolderIndex, 0, subfolders.count ) {
					const char* subfolder = subfolders[subfolderIndex];

					const char* guid = CreateVisualStudioGuid();

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<Filter Include=\"%s\">", subfolder ) ) );
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<UniqueIdentifier>{%s}</UniqueIdentifier>", guid ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,          "\t\t</Filter>" ) );
				}

				CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
			}

			// now put all files in the filter
			// visual studio requires that we list each file by type
			For ( u64, fileExtensionIndex, 0, project->file_extensions.size() ) {
				const char* fileExtension = project->file_extensions[fileExtensionIndex];

				For ( u64, folderIndex, 0, project->code_folders.size() ) {
					const char* folder = project->code_folders[folderIndex];

					CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

					Array<const char*> files;
					FindAllFilesInFolder_r( inputFilePath, folder, fileExtension, false, &files );

					if ( string_equals( fileExtension, "cpp" ) ) {	// TODO(DM): 01/01/2025: FileExtensionIsSourceFile( fileExtension )
						For ( u64, fileIndex, 0, files.count ) {
							CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClCompile Include=\"%s\" />", files[fileIndex] ) ) );
						}
					} else if ( string_equals( fileExtension, "h" ) ) {
						For ( u64, fileIndex, 0, files.count ) {
							CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClInclude Include=\"%s\" />", files[fileIndex] ) ) );
						}
					} else if ( string_equals( fileExtension, "inl" ) ) {
						For ( u64, fileIndex, 0, files.count ) {
							CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<None Include=\"%s\" />", files[fileIndex] ) ) );
						}
					}

					Array<const char*> subfolders;
					GetAllSubfolders_r( inputFilePath, NULL, &subfolders );

					For ( u64, subfolderIndex, 0, subfolders.count ) {
						const char* subfolder = subfolders[subfolderIndex];

						Array<const char*> filterFiles;
						FindAllFilesInFolder_r( inputFilePath, subfolder, fileExtension, false, &filterFiles );

						if ( string_equals( fileExtension, "cpp" ) ) {	// TODO(DM): 01/01/2025: FileExtensionIsSourceFile( fileExtension )
							For ( u64, fileIndex, 0, filterFiles.count ) {
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClCompile Include=\"%s\\%s\">", folder, filterFiles[fileIndex] ) ) );
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<Filter>%s</Filter>", subfolder ) ) );
								CHECK_WRITE( file_write_line( &vcxproj,          "\t\t</ClCompile>" ) );
							}
						} else if ( string_equals( fileExtension, "h" ) ) {
							For ( u64, fileIndex, 0, filterFiles.count ) {
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClInclude Include=\"%s\\%s\">", folder, filterFiles[fileIndex] ) ) );
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<Filter>%s</Filter>", subfolder ) ) );
								CHECK_WRITE( file_write_line( &vcxproj,          "\t\t</ClInclude>" ) );
							}
						} else {
							For ( u64, fileIndex, 0, filterFiles.count ) {
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<None Include=\"%s\\%s\">", folder, filterFiles[fileIndex] ) ) );
								CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t\t<Filter>%s</Filter>", subfolder ) ) );
								CHECK_WRITE( file_write_line( &vcxproj,          "\t\t</None>" ) );
							}
						}
					}

					CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
				}
			}

			CHECK_WRITE( file_write_line( &vcxproj, "</Project>" ) );
		}

		printf( "Done\n" );
	}

	// now generate .sln file
	{
		printf( "Generating %s ... ", solutionFilename );

		File sln = file_open_or_create( solutionFilename );
		defer( file_close( &sln ) );

		CHECK_WRITE( file_write(      &sln, "\n" ) );
		CHECK_WRITE( file_write_line( &sln, "Microsoft Visual Studio Solution File, Format Version 12.00" ) );
		CHECK_WRITE( file_write_line( &sln, "# Visual Studio Version 17" ) );
		CHECK_WRITE( file_write_line( &sln, "VisualStudioVersion = 17.7.34202.233" ) );			// TODO(DM): how do we query windows for this?
		CHECK_WRITE( file_write_line( &sln, "MinimunVisualStudioVersion = 10.0.40219.1" ) );	// TODO(DM): how do we query windows for this?

		// generate project dependencies
		For ( u64, projectIndex, 0, solution->projects.size() ) {
			VisualStudioProject* project = &solution->projects[projectIndex];

			CHECK_WRITE( file_write_line( &sln, tprintf( "Project(\"{%s}\") = \"%s\", \"%s.vcxproj\", \"{%s}\"", VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID, project->name, project->name, projectGuids[projectIndex] ) ) );
			// TODO: project dependencies go here and look like this:
			//	ProjectSection(ProjectDependencies) = postProject
			//		{NAME_GUID} = {NAME_GUID}
			//	EndProjectSection
			CHECK_WRITE( file_write_line( &sln, "EndProject" ) );
		}

		CHECK_WRITE( file_write_line( &sln, "Global" ) );
		{
			// which config|platform maps to which config|platform?
			CHECK_WRITE( file_write_line( &sln, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution" ) );
			For ( u64, projectIndex, 0, solution->projects.size() ) {
				VisualStudioProject* project = &solution->projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, solution->platforms.size() ) {
						const char* platform = solution->platforms[platformIndex];

						CHECK_WRITE( file_write_line( &sln, tprintf( "\t\t%s|%s = %s|%s", config->name, platform, config->name, platform ) ) );
					}
				}
			}
			CHECK_WRITE( file_write_line( &sln, "\tEndGlobalSection" ) );

			// which project config|platform is active?
			CHECK_WRITE( file_write_line( &sln, "\tGlobalSection(SolutionConfigurationPlatforms) = postSolution" ) );
			For ( u64, projectIndex, 0, solution->projects.size() ) {
				VisualStudioProject* project = &solution->projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, solution->platforms.size() ) {
						const char* platform = solution->platforms[platformIndex];

						// TODO: the first config and platform in this line are actually the ones that the PROJECT has, not the SOLUTION
						// but we dont use those, and we should
						CHECK_WRITE( file_write_line( &sln, tprintf( "\t\t{%s}.%s|%s.ActiveCfg = %s|%s", projectGuids[projectIndex], config->name, platform, config->name, platform ) ) );
						CHECK_WRITE( file_write_line( &sln, tprintf( "\t\t{%s}.%s|%s.Build.0 = %s|%s", projectGuids[projectIndex], config->name, platform, config->name, platform ) ) );
					}
				}
			}
			CHECK_WRITE( file_write_line( &sln, "\tEndGlobalSection" ) );

			// tell visual studio to not hide the solution node in the Solution Explorer
			// why would you ever want it to be hidden?
			CHECK_WRITE( file_write_line( &sln, "\tGlobalSection(SolutionProperties) = preSolution" ) );
			CHECK_WRITE( file_write_line( &sln, "\t\tHideSolutionNode = FALSE" ) );
			CHECK_WRITE( file_write_line( &sln, "\tEndGlobalSection" ) );

			//const char* solutionGUID = CreateVisualStudioGuid();

			//// we need to tell visual studio what the GUID of the solution is, apparently
			//// and we also need to do it in this really roundabout way...for some reason
			//CHECK_WRITE( file_write_line( &sln,          "\tGlobalSection(ExtensibilityGlobals) = postSolution" ) );
			//CHECK_WRITE( file_write_line( &sln, tprintf( "\t\tSolutionGuid = {%s}", solutionGUID ) ) );
			//CHECK_WRITE( file_write_line( &sln,          "\tEndGlobalSection" ) );
		}
		CHECK_WRITE( file_write_line( &sln, "EndGlobal" ) );

		printf( "Done\n" );
	}

	// generate .build_info file
	{
		BuildConfig defaultBuildConfig;
		BuildConfig_AddDefaults( &defaultBuildConfig );

		std::vector<BuildConfig> allBuildConfigs;

		For ( u64, platformIndex, 0, solution->platforms.size() ) {
			const char* platform = solution->platforms[platformIndex];

			For ( u64, projectIndex, 0, solution->projects.size() ) {
				VisualStudioProject* project = &solution->projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					// TODO(DM): 25/10/2024: this whole thing feels like a massive hack
					// users dont get the default BuilderOptions because they have to create their own ones inside set_visual_studio_options
					// so here we have to "merge" the defaults into what they specified
					// the user should have the defaults by default so that they can see what they have from the beginning
					// being transparent with the user is good, mkay?
					{
						config->options.additional_includes.insert( config->options.additional_includes.end(), defaultBuildConfig.additional_includes.begin(), defaultBuildConfig.additional_includes.end() );
						config->options.additional_libs.insert( config->options.additional_libs.end(), defaultBuildConfig.additional_libs.begin(), defaultBuildConfig.additional_libs.end() );
						config->options.ignore_warnings.insert( config->options.ignore_warnings.end(), defaultBuildConfig.ignore_warnings.begin(), defaultBuildConfig.ignore_warnings.end() );
					}

					allBuildConfigs.push_back( config->options );
				}
			}
		}

		SerializeBuildInfo( buildInfoFilename, allBuildConfigs, inputFilename, userConfigSourceFilename, userConfigBuildDLLFilename, verbose );
	}

	return true;
}

static void AddBuildConfigAndDependencies( BuildConfig* config, std::vector<BuildConfig>& outConfigs ) {
	For ( size_t, dependencyIndex, 0, config->depends_on.size() ) {
		AddBuildConfigAndDependencies( &config->depends_on[dependencyIndex], outConfigs );
	}

	add_build_config_unique( config, outConfigs );
}

int main( int argc, char** argv ) {
	float64 buildStart = time_ms();
	defer(
		float64 buildEnd = time_ms();
		printf( "Build finished: %f ms\n\n", buildEnd - buildStart );
	);

	core_init( MEM_KILOBYTES( 1 ), MEM_MEGABYTES( 64 ) );
	defer( core_shutdown() );

	// TODO(DM): 23/10/2024: we dont use this?
	set_command_line_args( argc, argv );

	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = {};
	context.flags |= BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS | BUILD_CONTEXT_FLAG_SHOW_STDOUT;

	// check if we need to perform first time setup
	{
		bool8 doFirstTimeSetup = false;

		// on exit set the CWD back to what we had before
		const char* oldCWD = paths_get_current_working_directory();
		defer( SetCurrentDirectory( oldCWD ) );

		// set CWD to whereever builder lives for first time setup
		SetCurrentDirectory( paths_get_app_path() );

		{
			const char* clangAbsolutePath = tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() );

			// if we cant find our copy of clang then we definitely need to run first time setup
			if ( !FileExists( clangAbsolutePath ) ) {
				doFirstTimeSetup = true;
			} else {
				// otherwise if we have clang but the version doesnt match, then we still need to re-download and re-install it
				bool8 correctClangVersion = false;

				Array<const char*> args;
				args.add( clangAbsolutePath );
				args.add( "-v" );

				Process* clangVersionCheck = process_create( &args, NULL );
				defer( process_destroy( clangVersionCheck ) );

				// this string is at the very start of "clang -v"
				const char* clangVersionString = tprintf( "clang version %s", CLANG_VERSION );

				char buffer[1024];
				while ( process_read_stdout( clangVersionCheck, buffer, 1024 ) ) {
					if ( string_contains( buffer, clangVersionString ) ) {
						correctClangVersion = true;
						break;
					}
				}

				s32 exitCode = process_join( &clangVersionCheck );
				assertf( exitCode == 0, "Something went terribly wrong..\n" );

				if ( !correctClangVersion ) {
					warning( "Required Clang version not found.  I will need to re-download and re-install Clang.\n" );
					doFirstTimeSetup = true;
				}
			}
		}

		if ( doFirstTimeSetup ) {
			printf( "Performing first time setup...\n" );

			const char* clangInstallerFilename = tprintf( "LLVM-%s-win64.exe", CLANG_VERSION );

			if ( !folder_create_if_it_doesnt_exist( ".\\temp" ) ) {
				errorCode_t errorCode = GetLastErrorCode();
				error( "Failed to create the temp folder that the Clang install uses.  Is it possible you have whacky user permissions? Error code: " ERROR_CODE_FORMAT "\n", errorCode );
				QuitError();
			}

			// download clang
			{
				printf( "Downloading Clang (please wait) ...\n" );

				Array<const char*> args;
				args.reserve( 4 );
				args.add( "curl" );
				args.add( "-o" );
				args.add( "temp\\clang_installer.exe" );
				args.add( "-L" );
				args.add( tprintf( "https://github.com/llvm/llvm-project/releases/download/llvmorg-%s/%s", CLANG_VERSION, clangInstallerFilename ) );

				s32 exitCode = RunProc( &args, NULL, false, true );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to download Clang.  The CURL HTTP request failed.\n" );
					QuitError();
				}
			}

			// install clang
			{
				printf( "Installing Clang (please wait) ... " );

				const char* clangInstallFolder = "clang";

				if ( !folder_create_if_it_doesnt_exist( clangInstallFolder ) ) {
					errorCode_t errorCode = GetLastErrorCode();
					error( "Failed to create the clang install folder \"%s\".  Is it possible you have some whacky user permissions? Error code: " ERROR_CODE_FORMAT "\n", clangInstallFolder, errorCode );
					QuitError();
				}

				// set clang installer command line arguments
				// taken from: https://discourse.llvm.org/t/using-clang-windows-installer-from-command-line/49698/2 which references https://nsis.sourceforge.io/Docs/Chapter3.html#installerusagecommon
				Array<const char*> args;
				args.reserve( 4 );
				args.add( ".\\temp\\clang_installer.exe" );
				args.add( "/S" );		// install in silent mode
				args.add( tprintf( "/D=%s\\%s", paths_get_app_path(), clangInstallFolder ) );	// set the install directory, absolute paths only

				Array<const char*> envVars;
				envVars.add( "__compat_layer=RunAsInvoker" );	// this tricks the subprocess into thinking we are running with elevation

				s32 exitCode = RunProc( &args, &envVars );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to install Clang.\n" );
					QuitError();
				}
			}

			// clean up temp files
			{
				printf( "Cleaning up ... " );

				NukeFolder( "temp", true );
				if ( !folder_delete( "temp" ) ) {
					errorCode_t errorCode = GetLastErrorCode();
					warning( "Failed to fully delete the temp folder after installing Clang.  You are safe to delete this yourself.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );
				}
			}

			doFirstTimeSetup = false;
		}
	}

	const char* inputFile = NULL;

	const char* inputConfigName = NULL;
	u64 inputConfigNameHash = 0;

#ifdef _DEBUG
	bool8 verbose = true;
#else
	bool8 verbose = false;
#endif

	// TODO(DM): 23/10/2024: feel like we shouldnt need these
	// I think this is just hiding the fact that we dont use builder.exes or the input files CWD when we need to
	bool8 doingBuildFromSourceFile = false;
	bool8 doingBuildFromBuildInfo = false;

	for ( s32 argIndex = 1; argIndex < argc; argIndex++ ) {
		const char* arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( string_equals( arg, ARG_HELP_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( string_equals( arg, ARG_VERBOSE_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			verbose = true;
			continue;
		}

		if ( string_ends_with( arg, ".c" ) || string_ends_with( arg, ".cpp" ) ) {
			if ( inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QuitError();
			}

			inputFile = arg;
			doingBuildFromSourceFile = true;

			continue;
		}

		if ( string_ends_with( arg, BUILD_INFO_FILE_EXTENSION ) ) {
			if ( inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QuitError();
			}

			inputFile = arg;
			doingBuildFromBuildInfo = true;

			continue;
		}

		if ( string_starts_with( arg, ARG_CONFIG ) ) {
			const char* equals = strchr( arg, '=' );

			if ( !equals ) {
				error( "I detected that you want to set a config, but you never gave me the equals (=) immediately after it.  You need to do that.\n" );

				return ShowUsage( 1 );
			}

			const char* configName = equals + 1;

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
				QuitError();
			}

			const char* folderToNuke = argv[argIndex + 1];

			printf( "Nuking \"%s\"\n", folderToNuke );

			float64 startTime = time_ms();

			NukeFolder( folderToNuke, true );

			float64 endTime = time_ms();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		// unrecognised arg, show error
		error( "Unrecognised argument \"%s\".\n", arg );
		QuitError();
	}

	// validate input file
	if ( inputFile == NULL ) {
		error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Run builder " ARG_HELP_LONG " if you need help.\n"
		);

		QuitError();
	}

	u64 inputFileLength = strlen( inputFile );

	// the default binary folder is the same folder as the source file
	const char* inputFilePath = paths_remove_file_from_path( inputFile );
	if ( !inputFilePath ) {
		inputFilePath = paths_get_current_working_directory();
	}

	const char* inputFileNoPath = paths_remove_path_from_file( inputFile );
	const char* inputFileNoPathOrExtension = paths_remove_file_extension( inputFileNoPath );

	const char* dotBuilderFolder = NULL;
	const char* buildInfoFilename = NULL;

	if ( doingBuildFromBuildInfo ) {
		dotBuilderFolder = inputFilePath;
		buildInfoFilename = inputFile;

		inputFilePath = tprintf( "%s\\..", inputFilePath );
	} else {
		dotBuilderFolder = tprintf( "%s\\.builder", inputFilePath );
		buildInfoFilename = tprintf( "%s\\%s%s", dotBuilderFolder, inputFileNoPathOrExtension, BUILD_INFO_FILE_EXTENSION );
	}

	BuilderOptions options = {};

	buildInfoFileData_t parsedBuildInfoData = {};
	bool8 readBuildInfo = Parser_ParseBuildInfo( buildInfoFilename, &parsedBuildInfoData );

	if ( doingBuildFromBuildInfo ) {
		// TODO(DM): 24/11/2024: this is stupid and slow
		// need to think of a better way to do this
		For ( u64, configIndex, 0, parsedBuildInfoData.configs.size() ) {
			options.configs.push_back( parsedBuildInfoData.configs[configIndex].config );
		}

		if ( !readBuildInfo ) {
			error( "Can't find \"%s\".  Does this file exist? Did you type it in correctly?\n", buildInfoFilename );
			QuitError();
		}
	}

	s32 exitCode = 0;

	Library library = {};
	defer( if ( library.ptr ) library_unload( &library ) );

	typedef void ( *setBuilderOptionsFunc_t )( BuilderOptions* options );
	typedef void ( *preBuildFunc_t )();
	typedef void ( *postBuildFunc_t )();

	preBuildFunc_t preBuildFunc = NULL;
	postBuildFunc_t postBuildFunc = NULL;

	const char* userConfigSourceFilename = NULL;
	const char* userConfigBuildDLLFilename = NULL;

	bool8 doUserConfigBuild = doingBuildFromSourceFile;

	if ( doingBuildFromBuildInfo ) {
		userConfigSourceFilename = parsedBuildInfoData.userConfigSourceFilename.c_str();
		userConfigBuildDLLFilename = parsedBuildInfoData.userConfigDLLFilename.c_str();

		library = library_load( userConfigBuildDLLFilename );

		if ( library.ptr == INVALID_HANDLE_VALUE ) {
			doUserConfigBuild = true;
		}
	}

	// build config step
	// see if they have set_builder_options() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	if ( doUserConfigBuild ) {
		buildContext_t userConfigBuildContext = {};
		BuildConfig_AddDefaults( &userConfigBuildContext.config );
		userConfigBuildContext.flags = BUILD_CONTEXT_FLAG_SHOW_STDOUT;

		if ( verbose ) {
			userConfigBuildContext.flags |= BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS;
		}

		userConfigSourceFilename = doingBuildFromSourceFile ? inputFile : parsedBuildInfoData.userConfigSourceFilename.c_str();

		userConfigBuildContext.config.source_files.push_back( userConfigSourceFilename );

		userConfigBuildContext.config.binary_name = tprintf( "%s.dll", paths_remove_path_from_file( paths_remove_file_extension( userConfigSourceFilename ) ) );
		userConfigBuildContext.config.binary_folder = dotBuilderFolder;
		userConfigBuildContext.config.defines = {
			"_CRT_SECURE_NO_WARNINGS",
			"BUILDER_DOING_USER_CONFIG_BUILD"
		};

		// this is needed because this tells the compiler what to set _ITERATOR_DEBUG_LEVEL to
		// ABI compatibility will be broken if this is not the same between all binaries
#if defined( _DEBUG )
		userConfigBuildContext.config.defines.push_back( "_DEBUG" );
		userConfigBuildContext.config.optimization_level = OPTIMIZATION_LEVEL_O0;
#elif defined( NDEBUG )
		userConfigBuildContext.config.defines.push_back( "NDEBUG" );
		userConfigBuildContext.config.optimization_level = OPTIMIZATION_LEVEL_O3;
#endif

		userConfigBuildContext.config.ignore_warnings.push_back( "-Wno-missing-prototypes" );	// otherwise the user has to forward declare functions like set_builder_options and thats annoying

		userConfigBuildContext.fullBinaryName = tprintf( "%s\\%s", userConfigBuildContext.config.binary_folder.c_str(), userConfigBuildContext.config.binary_name.c_str() );

		userConfigBuildDLLFilename = userConfigBuildContext.fullBinaryName;

		if ( !folder_create_if_it_doesnt_exist( userConfigBuildContext.config.binary_folder.c_str() ) ) {
			errorCode_t errorCode = GetLastErrorCode();
			error( "Failed to create the .builder folder.  Error code " ERROR_CODE_FORMAT "\n", userConfigBuildContext.config.binary_folder.c_str(), errorCode );
			QuitError();
		}

		exitCode = BuildDynamicLibrary( &userConfigBuildContext );

		if ( exitCode != 0 ) {
			error( "Pre-build failed!\n" );
			QuitError();
		}
	}

	printf( "\n" );

	{
		if ( library.ptr == INVALID_HANDLE_VALUE || library.ptr == NULL ) {
			library = library_load( userConfigBuildDLLFilename );
			assert( library.ptr != INVALID_HANDLE_VALUE );
		}

		preBuildFunc = cast( preBuildFunc_t ) library_get_proc_address( library, PRE_BUILD_FUNC_NAME );
		postBuildFunc = cast( postBuildFunc_t ) library_get_proc_address( library, POST_BUILD_FUNC_NAME );

		if ( doingBuildFromSourceFile ) {
			// now get the user-specified options
			setBuilderOptionsFunc_t setBuilderOptionsFunc = cast( setBuilderOptionsFunc_t ) library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME );
			if ( setBuilderOptionsFunc ) {
				setBuilderOptionsFunc( &options );

				// if the user wants to generate a visual studio solution then do that now
				if ( options.generate_solution ) {
					// make sure BuilderOptions::configs and configs from visual studio match
					// we will need this list later for validation
					options.configs.clear();
					For ( u64, projectIndex, 0, options.solution.projects.size() ) {
						VisualStudioProject* project = &options.solution.projects[projectIndex];

						For ( u64, configIndex, 0, project->configs.size() ) {
							VisualStudioConfig* config = &project->configs[configIndex];

							options.configs.push_back( config->options );
						}
					}

					printf( "Generating Visual Studio files\n" );

					bool8 generated = GenerateVisualStudioSolution( &options.solution, inputFile, userConfigSourceFilename, userConfigBuildDLLFilename, verbose );

					if ( !generated ) {
						error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
						QuitError();
					}

					printf( "Done.\n" );

					return 0;
				}
			}
		}
	}

	// none of the configs can have the same name
	// TODO(DM): 14/11/2024: can we do better than o(n^2) here?
	For ( size_t, configIndexA, 0, options.configs.size() ) {
		const char* configNameA = options.configs[configIndexA].name.c_str();
		u64 configNameHashA = hash_string( configNameA, 0 );

		For ( size_t, configIndexB, 0, options.configs.size() ) {
			if ( configIndexA == configIndexB ) {
				continue;
			}

			u64 configNameHashB = hash_string( options.configs[configIndexB].name.c_str(), 0 );

			if ( configNameHashA == configNameHashB ) {
				error( "I found multiple configs with the name \"%s\".  All config names MUST be unique, otherwise I don't know which specific config you want me to build.\n", configNameA );
				QuitError();
			}
		}
	}

	std::vector<BuildConfig> configsToBuild;

	// of all the configs that the user filled out inside set_builder_options
	// find the one the user asked for in the command line
	if ( inputConfigName ) {
		bool8 foundConfig = false;
		For ( u64, configIndex, 0, options.configs.size() ) {
			BuildConfig* config = &options.configs[configIndex];

			if ( hash_string( config->name.c_str(), 0 ) == inputConfigNameHash ) {
				For ( size_t, dependencyIndex, 0, config->depends_on.size() ) {
					AddBuildConfigAndDependencies( &config->depends_on[dependencyIndex], configsToBuild );
				}

				add_build_config_unique( config, configsToBuild );

				foundConfig = true;

				break;
			}
		}

		if ( !foundConfig ) {
			error( "You passed the config name \"%s\" via the command line, but I never found a config with that name inside %s.  Make sure they match.\n", inputConfigName, SET_BUILDER_OPTIONS_FUNC_NAME );
			QuitError();
		}
	}

	// if no configs were manually added then assume we are just doing a default build with no user-specified options
	if ( options.configs.size() == 0 ) {
		BuildConfig config = {
			.source_files = { inputFile }
		};

		options.configs.push_back( config );
	}

	// if only one config was added (either by user or as a default build) then we know we just want that one, no config command line arg is needed
	if ( options.configs.size() == 1 ) {
		BuildConfig* config = &options.configs[0];

		For ( size_t, dependencyIndex, 0, config->depends_on.size() ) {
			AddBuildConfigAndDependencies( &config->depends_on[dependencyIndex], configsToBuild );
		}

		add_build_config_unique( config, configsToBuild );
	} else {
		if ( !inputConfigName ) {
			error(
				"This build has multiple configs, but you never specified a config name.\n"
				"You must pass in a config name via " ARG_CONFIG "\n"
				"Run builder " ARG_HELP_LONG " if you need help.\n"
			);
			QuitError();
		}
	}

	if ( preBuildFunc ) {
		printf( "Running pre-build code...\n" );
		preBuildFunc();
	}

	bool8 allBuildsWereSkipped = true;

	For ( u64, configToBuildIndex, 0, configsToBuild.size() ) {
		BuildConfig* config = &configsToBuild[configToBuildIndex];

		if ( doingBuildFromSourceFile ) {
			BuildConfig_AddDefaults( config );
		}

		// make sure that the binary folder and binary name are at least set to defaults
		{
			if ( !config->binary_folder.empty() ) {
				config->binary_folder = tprintf( "%s\\%s", inputFilePath, config->binary_folder.c_str() );
			} else {
				config->binary_folder = inputFilePath;
			}

			// user didnt override the binary name via the callback
			// so give them a binary name based off the first source file
			if ( config->binary_name.empty() ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
				char* inputFileWithoutExtension = cast( char* ) mem_temp_alloc( ( inputFileLength + 1 ) * sizeof( char ) );
				strncpy( inputFileWithoutExtension, inputFile, inputFileLength * sizeof( char ) );
				inputFileWithoutExtension[inputFileLength] = 0;

				inputFileWithoutExtension = cast( char* ) paths_remove_file_extension( paths_remove_path_from_file( inputFileWithoutExtension ) );

				config->binary_name = inputFileWithoutExtension;
#pragma clang diagnostic pop
			}
		}

		context.config = *config;
		context.fullBinaryName = BuildConfig_GetFullBinaryName( &context.config );

		bool8 shouldSkipBuild = true;

		// figure out if we need to even rebuild
		// if the binary doesnt exist, we definitely need to rebuild
		if ( !FileExists( context.fullBinaryName ) ) {
			shouldSkipBuild = false;
		}
		
		// get all the code files from the .build_info file
		// if none of the code files have changed since we last checked then we do not need to rebuild
		{
			std::vector<trackedSourceFile_t> trackedSourceFiles;

			For ( u64, configIndex, 0, parsedBuildInfoData.configs.size() ) {
				buildInfoConfig_t* buildInfoConfig = &parsedBuildInfoData.configs[configIndex];

				if ( context.config.name == buildInfoConfig->config.name ) {
					trackedSourceFiles = buildInfoConfig->files;
					break;
				}
			}

			For ( u64, sourceFileIndex, 0, trackedSourceFiles.size() ) {
				trackedSourceFile_t* trackedSourceFile = &trackedSourceFiles[sourceFileIndex];

				const char* trackedSourceFileAndPath = tprintf( "%s\\%s", inputFilePath, trackedSourceFile->filename );

				FileInfo fileInfo = {};
				File file = file_find_first( trackedSourceFileAndPath, &fileInfo );

				assert( file.ptr != INVALID_HANDLE_VALUE );

				if ( fileInfo.last_write_time != trackedSourceFile->lastWriteTime ) {
					shouldSkipBuild = false;
					break;
				}
			}
		}

		if ( shouldSkipBuild ) {
			printf( "Skipped \"%s\".\n", context.config.binary_name.c_str() );
			continue;
		} else {
			printf( "Building \"%s\"", context.config.binary_name.c_str() );

			if ( !context.config.name.empty() ) {
				printf( ", config \"%s\"", context.config.name.c_str() );
			}

			printf( "\n" );
		}

		allBuildsWereSkipped = false;

		if ( !folder_create_if_it_doesnt_exist( context.config.binary_folder.c_str() ) ) {
			errorCode_t errorCode = GetLastErrorCode();
			fatal_error( "Failed to create the binary folder you specified inside %s: \"%s\".  Error code: " ERROR_CODE_FORMAT "\n", SET_BUILDER_OPTIONS_FUNC_NAME, context.config.binary_folder.c_str(), errorCode );
			return 1;
		}

		// make all non-absolute additional include paths relative to the build source file
		For ( u64, includeIndex, 0, context.config.additional_includes.size() ) {
			const char* additionalInclude = context.config.additional_includes[includeIndex].c_str();

			if ( paths_is_path_absolute( additionalInclude ) ) {
				context.config.additional_includes[includeIndex] = additionalInclude;
			} else {
				context.config.additional_includes[includeIndex] = tprintf( "%s\\%s", inputFilePath, additionalInclude );
			}
		}

		// make all non-absolute additional library paths relative to the build source file
		For ( u64, libPathIndex, 0, context.config.additional_lib_paths.size() ) {
			const char* additionalLibPath = context.config.additional_lib_paths[libPathIndex].c_str();

			if ( paths_is_path_absolute( additionalLibPath ) ) {
				context.config.additional_lib_paths[libPathIndex] = additionalLibPath;
			} else {
				context.config.additional_lib_paths[libPathIndex] = tprintf( "%s\\%s", inputFilePath, additionalLibPath );
			}
		}

		// get all the "compilation units" that we are actually going to give to the compiler
		// if no source files were added in set_builder_options() then assume they only want to build the same file as the one specified via the command line
		if ( context.config.source_files.size() == 0 ) {
			context.config.source_files.push_back( inputFile );
		} else {
			// otherwise the user told us to build other source files, so go find and build those instead
			// keep this as a std::vector because this gets fed back into BuilderOptions::source_files
			std::vector<const char*> finalSourceFilesToBuild = BuildConfig_GetAllSourceFiles( &context.config, inputFile );

			// at this point its totally acceptable for finalSourceFilesToBuild to be empty
			// this is because the compiler should be the one that tells the user they specified no valid source files to build with
			// the compiler can and will throw an error for that, so let it

			// the .build_info file wont store the full paths to the source files because the input path can change depending on whether we're building via visual studio or from the command line
			// so we need to reconstruct the full paths to the source files ourselves
			For ( u64, sourceFileIndex, 0, finalSourceFilesToBuild.size() ) {
				finalSourceFilesToBuild[sourceFileIndex] = tprintf( "%s\\%s", inputFilePath, finalSourceFilesToBuild[sourceFileIndex] );
			}

			context.config.source_files = finalSourceFilesToBuild;
		}

		// now do the actual build
		switch ( config->binary_type ) {
			case BINARY_TYPE_EXE:
				exitCode = BuildEXE( &context );
				break;

			case BINARY_TYPE_DYNAMIC_LIBRARY:
				exitCode = BuildDynamicLibrary( &context );
				break;

			case BINARY_TYPE_STATIC_LIBRARY:
				exitCode = BuildStaticLibrary( &context );
				break;
		}

		// if the build was successful, write the new .build_info file now
		if ( exitCode != 0 ) {
			error( "Build failed.\n" );
			QuitError();
		}

		printf( "\n" );
	}

	if ( postBuildFunc ) {
		printf( "Running post-build code...\n" );
		postBuildFunc();
	}

	if ( !allBuildsWereSkipped ) {
		SerializeBuildInfo( buildInfoFilename, options.configs, inputFile, userConfigSourceFilename, userConfigBuildDLLFilename, verbose );
	}

	return 0;
}