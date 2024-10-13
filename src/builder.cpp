/*
===========================================================================

Builder

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "../builder.h"

#include "core/core.cpp"

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
	BUILDER_VERSION_MINOR	= 1,
	BUILDER_VERSION_PATCH	= 2,
};

#define ARG_HELP_SHORT	"-h"
#define ARG_HELP_LONG	"--help"
#define ARG_NUKE		"--nuke"
#define ARG_CONFIG		"--config="

#define CLANG_VERSION	"18.1.8"

#define BUILD_INFO_FILE_EXTENSION			"build_info"
#define SET_BUILDER_OPTIONS_FUNC_NAME		"set_builder_options"
#define PRE_BUILD_FUNC_NAME					"on_pre_build"
#define POST_BUILD_FUNC_NAME				"on_post_build"
#define SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME	"set_visual_studio_options"

enum buildContextFlagBits_t {
	BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS	= bit( 0 ),
	BUILD_CONTEXT_FLAG_SHOW_STDOUT			= bit( 1 ),
};
typedef u32 buildContextFlags_t;

struct buildContext_t {
	BuilderOptions			options;

	// TODO(DM): 10/08/2024: does this want to be inside BuilderOptions?
	// it would give users more control over their build
	buildContextFlags_t		flags;

	const char*				fullBinaryName;
};

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
		For ( u64, i, 0, args->count ) {
			printf( "%s ", ( *args )[i] );
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
		"    Builder.exe <source files>\n"
		"\n"
		"Arguments:\n"
		"    " ARG_HELP_SHORT "|" ARG_HELP_LONG " (optional):\n"
		"        Shows this help and then exits.\n"
		"\n"
		"    <source files> (required):\n"
		"        The source file(s) you want to build with.  Need at least one.\n"
		"        File name MUST end with either \".c\" or \".cpp\".\n"
		"\n"
		"    " ARG_NUKE " <folder> (optional):\n"
		"        Deletes every file in <folder> and all subfolders.  Used for when you need to clean and rebuild.\n"
		"\n"
		"    " ARG_CONFIG "<config> (optional):\n"
		"        Sets the config to whatever you specify.  The config value can be whatever you want.\n"
		"        If you set this then you must also override " SET_BUILDER_OPTIONS_FUNC_NAME " in order to make use of it in your build.\n"
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
	array_reserve( &args,
		1 +	// clang
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->options.source_files.size() +
		context->options.defines.size() +
		context->options.additional_includes.size() +
		context->options.additional_lib_paths.size() +
		context->options.additional_libs.size() +
		5 +	// warning levels
		context->options.ignore_warnings.size()
	);

	array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );

	For ( u64, i, 0, context->options.source_files.size() ) {
		if ( string_ends_with( context->options.source_files[i], ".cpp" ) ) {
			array_add( &args, "-std=c++20" );
			break;
		}
	}

	if ( !context->options.remove_symbols ) {
		array_add( &args, "-g" );
	}

	array_add( &args, OptimizationLevelToString( context->options.optimization_level ) );

	array_add( &args, "-o" );
	array_add( &args, context->fullBinaryName );

	array_add_range( &args, context->options.source_files.data(), context->options.source_files.size() );

	For ( u32, i, 0, context->options.defines.size() ) {
		array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
	}

	For ( u32, i, 0, context->options.additional_includes.size() ) {
		array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
	}

	For ( u32, i, 0, context->options.additional_lib_paths.size() ) {
		array_add( &args, tprintf( "-L%s", context->options.additional_lib_paths[i] ) );
	}

	For ( u32, i, 0, context->options.additional_libs.size() ) {
		array_add( &args, tprintf( "-l%s", context->options.additional_libs[i] ) );
	}

	// must come before ignored warnings
	array_add( &args, "-Werror" );
	array_add( &args, "-Weverything" );
	array_add( &args, "-Wall" );
	array_add( &args, "-Wextra" );
	array_add( &args, "-Wpedantic" );

	if ( context->options.ignore_warnings.size() > 0 ) {
		array_add_range( &args, context->options.ignore_warnings.data(), context->options.ignore_warnings.size() );
	}

	array_add( &args, NULL );

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
	array_reserve( &args,
		1 +	// clang
		1 +	// -shared
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->options.source_files.size() +
		context->options.defines.size() +
		context->options.additional_includes.size() +
		context->options.additional_lib_paths.size() +
		context->options.additional_libs.size() +
		5 +	// warning levels
		context->options.ignore_warnings.size()
	);

	array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
	array_add( &args, "-shared" );

	For ( u64, i, 0, context->options.source_files.size() ) {
		if ( string_ends_with( context->options.source_files[i], ".cpp" ) ) {
			array_add( &args, "-std=c++20" );
			break;
		}
	}

	if ( !context->options.remove_symbols ) {
		array_add( &args, "-g" );
	}

	array_add( &args, OptimizationLevelToString( context->options.optimization_level ) );

	array_add( &args, "-o" );
	array_add( &args, context->fullBinaryName );

	array_add_range( &args, context->options.source_files.data(), context->options.source_files.size() );

	For ( u32, i, 0, context->options.defines.size() ) {
		array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
	}

	For ( u32, i, 0, context->options.additional_includes.size() ) {
		array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
	}

	For ( u32, i, 0, context->options.additional_lib_paths.size() ) {
		array_add( &args, tprintf( "-L%s", context->options.additional_lib_paths[i] ) );
	}

	For ( u32, i, 0, context->options.additional_libs.size() ) {
		array_add( &args, tprintf( "-l%s", context->options.additional_libs[i] ) );
	}

	// must come before ignored warnings
	array_add( &args, "-Werror" );
	array_add( &args, "-Weverything" );
	array_add( &args, "-Wall" );
	array_add( &args, "-Wextra" );
	array_add( &args, "-Wpedantic" );

	if ( context->options.ignore_warnings.size() > 0 ) {
		array_add_range( &args, context->options.ignore_warnings.data(), context->options.ignore_warnings.size() );
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
	array_reserve( &args,
		1 +	// clang
		1 +	// -c
		1 +	// std
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// binary name
		context->options.source_files.size() +
		context->options.defines.size() +
		context->options.additional_includes.size() +
		context->options.additional_lib_paths.size() +
		context->options.additional_libs.size() +
		5 +	// warning levels
		context->options.ignore_warnings.size()
	);

	// build .o files of all compilation units
	For ( u64, sourceFileIndex, 0, context->options.source_files.size() ) {
		const char* sourceFile = context->options.source_files[sourceFileIndex];

		array_reset( &args );

		array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
		array_add( &args, "-c" );

		if ( string_ends_with( context->options.source_files[sourceFileIndex], ".cpp" ) ) {
			array_add( &args, "-std=c++20" );
		} else if ( string_ends_with( context->options.source_files[sourceFileIndex], ".c" ) ) {
			array_add( &args, "-std=c99" );
		} else {
			assertf( false, "Something went really wrong.\n" );
			return 1;
		}

		if ( !context->options.remove_symbols ) {
			array_add( &args, "-g" );
		}

		array_add( &args, OptimizationLevelToString( context->options.optimization_level ) );

		{
			const char* sourceFileTrimmed = context->options.source_files[sourceFileIndex];
			sourceFileTrimmed = strrchr( sourceFileTrimmed, '\\' ) + 1;

			const char* outArg = tprintf( "%s\\%s.o", context->options.binary_folder.c_str(), sourceFileTrimmed );

			array_add( &args, "-o" );
			array_add( &args, outArg );
			array_add( &intermediateFiles, outArg );
		}

		array_add( &args, sourceFile );

		For ( u32, i, 0, context->options.defines.size() ) {
			array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
		}

		For ( u32, i, 0, context->options.additional_includes.size() ) {
			array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
		}

		// must come before ignored warnings
		array_add( &args, "-Werror" );
		array_add( &args, "-Weverything" );
		array_add( &args, "-Wall" );
		array_add( &args, "-Wextra" );
		array_add( &args, "-Wpedantic" );

		if ( context->options.ignore_warnings.size() > 0 ) {
			array_add_range( &args, context->options.ignore_warnings.data(), context->options.ignore_warnings.size() );
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
		array_reset( &args );

		array_add( &args, "lld-link" );
		array_add( &args, "/lib" );

		array_add( &args, tprintf( "/OUT:%s", context->fullBinaryName ) );

		array_add_range( &args, intermediateFiles.data, intermediateFiles.count );

		exitCode = RunProc( &args, NULL, showArgs, showStdout );

		if ( exitCode != 0 ) {
			error( "Link failed.\n" );
		}
	}

	return exitCode;
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

			folder_delete( fileFullPath );
		} else {
			if ( verbose ) printf( "Deleting file %s\n", fileFullPath );

			file_delete( fileFullPath );
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

static bool8 PathHasSlash( const char* path ) {
	bool8 hasSlash = false;

	hasSlash |= strchr( path, '/' ) != NULL;
	hasSlash |= strchr( path, '\\' ) != NULL;

	return hasSlash;
}

static const char* TryFindFile( const char* filename, const char* folder ) {
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
			result = TryFindFile( filename, fullFilename );
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

static bool8 GenerateVisualStudioSolution( VisualStudioSolution* solution, const char* inputFilePath );

static buildContext_t CreateBuildContext() {
	buildContext_t context = {};

	context.options.binary_type = BINARY_TYPE_EXE;
	context.options.optimization_level = OPTIMIZATION_LEVEL_O0;
	context.options.remove_symbols = false;
	context.options.remove_file_extension = false;
	//context.options.config = NULL;
	//context.options.binary_folder = NULL;
	//context.options.binary_name = NULL;
	context.flags = 0;

	return context;
}

int main( int argc, char** argv ) {
	float64 buildStart = time_ms();
	defer(
		float64 buildEnd = time_ms();
		printf( "Build finished: %f ms\n\n", buildEnd - buildStart );
	);

	core_init( MEM_KILOBYTES( 1 ), MEM_MEGABYTES( 64 ) );
	defer( core_shutdown() );

	set_command_line_args( argc, argv );

	printf( "Builder v%d.%d.%d\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = CreateBuildContext();
	context.flags |= BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS | BUILD_CONTEXT_FLAG_SHOW_STDOUT;

	// check if we need to perform first time setup
	{
		bool8 doFirstTimeSetup = false;

		// on exit set the CWD back to what we had before
		char oldCWD[MAX_PATH] = {};
		GetCurrentDirectory( MAX_PATH, oldCWD );
		defer( SetCurrentDirectory( oldCWD ) );

		// set CWD to whereever builder lives for first time setup
		SetCurrentDirectory( paths_get_app_path() );

		{
			const char* clangAbsolutePath = tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() );

			FileInfo fileInfo;
			File file = file_find_first( clangAbsolutePath, &fileInfo );

			// if we cant find our copy of clang then we definitely need to run first time setup
			if ( file.ptr == INVALID_HANDLE_VALUE ) {
				doFirstTimeSetup = true;
			} else {
				// otherwise if we have clang but the version doesnt match, then we still need to re-download and re-install it
				bool8 correctClangVersion = false;

				Array<const char*> args;
				array_add( &args, clangAbsolutePath );
				array_add( &args, "-v" );

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

			folder_create_if_it_doesnt_exist( ".\\temp" );

			// download clang
			{
				printf( "Downloading Clang (please wait) ...\n" );

				Array<const char*> args;
				array_reserve( &args, 4 );
				array_add( &args, "curl" );
				array_add( &args, "-o" );
				array_add( &args, "temp\\clang_installer.exe" );
				array_add( &args, "-L" );
				array_add( &args, tprintf( "https://github.com/llvm/llvm-project/releases/download/llvmorg-%s/%s", CLANG_VERSION, clangInstallerFilename ) );

				s32 exitCode = RunProc( &args, NULL, false, true );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to download Clang.  The CURL HTTP request failed.\n" );
					return 1;
				}
			}

			// install clang
			{
				printf( "Installing Clang (please wait) ... " );

				const char* clangInstallFolder = "clang";

				folder_create_if_it_doesnt_exist( clangInstallFolder );

				// set clang installer command line arguments
				// taken from: https://discourse.llvm.org/t/using-clang-windows-installer-from-command-line/49698/2 which references https://nsis.sourceforge.io/Docs/Chapter3.html#installerusagecommon
				Array<const char*> args;
				array_reserve( &args, 4 );
				array_add( &args, ".\\temp\\clang_installer.exe" );
				array_add( &args, "/S" );		// install in silent mode
				array_add( &args, tprintf( "/D=%s\\%s", paths_get_app_path(), clangInstallFolder ) );	// set the install directory, absolute paths only

				Array<const char*> envVars;
				array_add( &envVars, "__compat_layer=RunAsInvoker" );	// this tricks the subprocess into thinking we are running with elevation

				s32 exitCode = RunProc( &args, &envVars );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to install Clang.\n" );
					return 1;
				}
			}

			// clean up temp files
			{
				printf( "Cleaning up ... " );

				NukeFolder( "temp", true );
				folder_delete( "temp" );
			}

			doFirstTimeSetup = false;
		}
	}

	const char* inputFile = NULL;
	const char* visualStudioConfigSourceFile = NULL;

	for ( s32 argIndex = 1; argIndex < argc; argIndex++ ) {
		const char* arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( string_equals( arg, ARG_NUKE ) ) {
			if ( argIndex == argc - 1 ) {
				error( "You passed in " ARG_NUKE " but you never told me what folder you want me to nuke.  I need to know!" );
				return 1;
			}

			const char* folderToNuke = argv[argIndex + 1];

			printf( "Nuking \"%s\"\n", folderToNuke );

			float64 startTime = time_ms();

			NukeFolder( folderToNuke, true );

			float64 endTime = time_ms();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		if ( string_equals( arg, ARG_HELP_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
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

			context.options.config = configName;

			continue;
		}

		if ( string_ends_with( arg, ".c" ) || string_ends_with( arg, ".cpp" ) ) {
			if ( inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				return 1;
			}

			inputFile = arg;

			continue;
		}

		// unrecognised arg, show error
		error( "Unrecognised argument \"%s\".\n", arg );
	}

	// validate cmd line args
	if ( inputFile == NULL ) {
		error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Use " ARG_HELP_SHORT " if you need help.\n"
		);

		return 1;
	}

	// set all the additional compiler options that we know we need
	{
		context.options.defines.push_back( "_CRT_SECURE_NO_WARNINGS" );

		// add the folder that builder lives in as an additional include path
		// so that people can just include builder.h without having to add the include path manually every time
		context.options.additional_includes.push_back( paths_get_app_path() );

#if defined( _WIN64 )
		context.options.additional_libs.push_back( "user32.lib" );

		// MSVCRT is needed for ABI compatibility between builder and the user config DLL
#if defined( _DEBUG )
		context.options.additional_libs.push_back( "msvcrtd.lib" );
#elif defined( NDEBUG )
		context.options.additional_libs.push_back( "msvcrt.lib" );
#endif

		// TODO(DM): 12/10/2024: are these debug only? are they even needed?
		//context.options.additional_libs.push_back( "Shlwapi.lib" );
		//context.options.additional_libs.push_back( "DbgHelp.lib" );
#endif // defined( _WIN64 )

		context.options.ignore_warnings.push_back( "-Wno-newline-eof" );
		context.options.ignore_warnings.push_back( "-Wno-pointer-integer-compare" );
		context.options.ignore_warnings.push_back( "-Wno-declaration-after-statement" );
		context.options.ignore_warnings.push_back( "-Wno-gnu-zero-variadic-macro-arguments" );
		context.options.ignore_warnings.push_back( "-Wno-cast-align" );
		context.options.ignore_warnings.push_back( "-Wno-bad-function-cast" );
		context.options.ignore_warnings.push_back( "-Wno-format-nonliteral" );
		context.options.ignore_warnings.push_back( "-Wno-missing-braces" );
		context.options.ignore_warnings.push_back( "-Wno-switch-enum" );
		context.options.ignore_warnings.push_back( "-Wno-covered-switch-default" );
		context.options.ignore_warnings.push_back( "-Wno-double-promotion" );
		context.options.ignore_warnings.push_back( "-Wno-cast-qual" );
		context.options.ignore_warnings.push_back( "-Wno-unused-variable" );
		context.options.ignore_warnings.push_back( "-Wno-unused-function" );
		context.options.ignore_warnings.push_back( "-Wno-empty-translation-unit" );
		context.options.ignore_warnings.push_back( "-Wno-zero-as-null-pointer-constant" );
		context.options.ignore_warnings.push_back( "-Wno-c++98-compat-pedantic" );
		context.options.ignore_warnings.push_back( "-Wno-unused-macros" );
		context.options.ignore_warnings.push_back( "-Wno-unsafe-buffer-usage" );		// LLVM 17.0.1
		context.options.ignore_warnings.push_back( "-Wno-reorder-init-list" );			// C++: "designated initializers must be in order"
		context.options.ignore_warnings.push_back( "-Wno-old-style-cast" );				// C++: "C-style casts are banned"
		context.options.ignore_warnings.push_back( "-Wno-global-constructors" );		// C++: "declaration requires a global destructor"
		context.options.ignore_warnings.push_back( "-Wno-exit-time-destructors" );		// C++: "declaration requires an exit-time destructor" (same as the above, basically)
		context.options.ignore_warnings.push_back( "-Wno-missing-field-initializers" );	// LLVM 18.1.8
	}

	s32 exitCode = 0;

	const char* firstSourceFile = inputFile;
	u64 firstSourceFileLength = strlen( firstSourceFile );

	// the default binary folder is the same folder as the source file
	const char* buildFilePathAbsolute = paths_remove_file_from_path( paths_get_absolute_path( firstSourceFile ) );
	const char* firstSourceFileNoPath = paths_remove_path_from_file( firstSourceFile );

	const char* dotBuilderFolder = tprintf( "%s\\.builder", buildFilePathAbsolute );

	// if none of the source files have changed since we last checked then do not even try to rebuild
	{
		bool8 rebuild = false;

		const char* buildInfoFilename = tprintf( "%s\\%s.%s", dotBuilderFolder, firstSourceFileNoPath, BUILD_INFO_FILE_EXTENSION );

		File buildInfoFile = file_open( buildInfoFilename );

		// if we cant find a .build_info file then assume we never built this binary before
		if ( buildInfoFile.ptr == NULL ) {
			printf( "Can't open %s.  Rebuilding binary...\n", buildInfoFilename );
			rebuild = true;
		} else {
			// otherwise we have one, so get the build times out of it and check them against what we had before
			defer( file_close( &buildInfoFile ) );

			char* fileBuffer = NULL;
			u64 fileBufferLength = 0;
			bool8 read = file_read_entire( buildInfoFilename, &fileBuffer, &fileBufferLength );

			if ( read ) {
				defer( file_free_buffer( &fileBuffer ) );

				u32 sourceFileIndex = 0;

				// parse the .build_info file
				// for each source file, get the filename and the last write time
				u64 offset = 0;
				while ( offset < fileBufferLength ) {
					const char* lineStart = &fileBuffer[offset];
					const char* colon = strstr( lineStart, ": " );
					const char* lineEnd = strstr( lineStart, "\r\n" );
					if ( !lineEnd ) lineEnd = strstr( lineStart, "\n" );

					u64 filenameLength = cast( u64 ) colon - cast( u64 ) lineStart;
					filenameLength++;

					colon += 2;

					u64 timestampLength = cast( u64 ) lineEnd - cast( u64 ) colon;
					timestampLength++;

					char* filename = cast( char* ) mem_temp_alloc( filenameLength * sizeof( char ) );
					char* timestampString = cast( char* ) mem_temp_alloc( timestampLength * sizeof( char ) );

					strncpy( filename, lineStart, filenameLength );
					filename[filenameLength - 1] = 0;

					strncpy( timestampString, colon, timestampLength );
					timestampString[timestampLength - 1] = 0;

					u64 lastWriteTime = cast( u64 ) atoll( timestampString );

					FileInfo sourceFileInfo;
					File sourceFile = file_find_first( filename, &sourceFileInfo );

					assert( sourceFile.ptr != INVALID_HANDLE_VALUE );

					if ( lastWriteTime != sourceFileInfo.last_write_time ) {
						rebuild = true;
						break;
					}

					sourceFileIndex++;

					u64 lineLength = cast( u64 ) ( lineEnd + 1 ) - cast( u64 ) lineStart;
					offset += lineLength;
				}
			} else {
				error( "Found %s but failed to read it.  Rebuilding binary...\n", buildInfoFilename );
				rebuild = true;
			}
		}

		if ( !rebuild ) {
			printf( "Skipping build for %s.\n", buildInfoFilename );
			return 0;
		}
	}

	Library library;
	defer( if ( library.ptr ) library_unload( &library ) );

	typedef void ( *preBuildFunc_t )( BuilderOptions* options );
	typedef void ( *postBuildFunc_t )( BuilderOptions* options );

	preBuildFunc_t preBuildFunc = NULL;
	postBuildFunc_t postBuildFunc = NULL;

	// build config step
	// see if they have set_builder_options() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	{
		typedef void ( *setBuilderOptionsFunc_t )( BuilderOptions* options );
		typedef void ( *setVisualStudioOptionsFunc_t )( VisualStudioSolution* solution );

		buildContext_t userBuildConfigContext = CreateBuildContext();
		userBuildConfigContext.options = context.options;
		userBuildConfigContext.flags = BUILD_CONTEXT_FLAG_SHOW_STDOUT;

		userBuildConfigContext.options.binary_name = tprintf( "%s.dll", paths_remove_path_from_file( firstSourceFile ) );
		userBuildConfigContext.options.binary_folder = dotBuilderFolder;
		userBuildConfigContext.options.source_files.push_back( inputFile );
		userBuildConfigContext.options.defines.push_back( "BUILDER_DOING_USER_CONFIG_BUILD" );

		// this is needed because this tells the compiler what to set _ITERATOR_DEBUG_LEVEL to
		// ABI compatibility will be broken if this is not the same between all binaries
#if defined( _DEBUG )
		userBuildConfigContext.options.defines.push_back( "_DEBUG" );
		userBuildConfigContext.options.optimization_level = OPTIMIZATION_LEVEL_O0;
#elif defined( NDEBUG )
		userBuildConfigContext.options.defines.push_back( "NDEBUG" );
		userBuildConfigContext.options.optimization_level = OPTIMIZATION_LEVEL_O3;
#endif

		userBuildConfigContext.options.ignore_warnings.push_back( "-Wno-missing-prototypes" );	// otherwise the user has to forward declare functions like set_builder_options and thats annoying
		userBuildConfigContext.options.ignore_warnings.push_back( "-Wno-unused-parameter" );	// user can call set_pre_build (for example) and not actually touch the BuilderOptions parm

		userBuildConfigContext.fullBinaryName = tprintf( "%s\\%s", userBuildConfigContext.options.binary_folder.c_str(), userBuildConfigContext.options.binary_name.c_str() );

		folder_create_if_it_doesnt_exist( userBuildConfigContext.options.binary_folder.c_str() );

		exitCode = BuildDynamicLibrary( &userBuildConfigContext );

		if ( exitCode != 0 ) {
			error( "Pre-build failed!\n" );
			return 1;
		}

		library = library_load( tprintf( "%s\\%s", userBuildConfigContext.options.binary_folder.c_str(), userBuildConfigContext.options.binary_name.c_str() ) );

		// generate visual studio solution now if requested
		setVisualStudioOptionsFunc_t setVisualStudioOptionsFunc = cast( setVisualStudioOptionsFunc_t ) library_get_proc_address( library, SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME );
		if ( setVisualStudioOptionsFunc ) {
			VisualStudioSolution solution;
			setVisualStudioOptionsFunc( &solution );

			printf( "Generating Visual Studio Solution\n" );

			bool8 generated = GenerateVisualStudioSolution( &solution, buildFilePathAbsolute );

			if ( !generated ) {
				error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
				return 1;
			}

			printf( "Done\n" );

			return 0;
		}

		// now get the user-specified options
		setBuilderOptionsFunc_t callback = cast( setBuilderOptionsFunc_t ) library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME );
		if ( callback ) {
			callback( &context.options );

			For ( u64, includeIndex, 0, context.options.additional_includes.size() ) {
				const char* additionalInclude = context.options.additional_includes[includeIndex];

				if ( paths_is_path_absolute( additionalInclude ) ) {
					context.options.additional_includes[includeIndex] = additionalInclude;
				} else {
					context.options.additional_includes[includeIndex] = tprintf( "%s\\%s", buildFilePathAbsolute, additionalInclude );
				}
			}

			For ( u64, libPathIndex, 0, context.options.additional_lib_paths.size() ) {
				context.options.additional_lib_paths[libPathIndex] = tprintf( "%s\\%s", buildFilePathAbsolute, context.options.additional_lib_paths[libPathIndex] );
			}
		}

		preBuildFunc = cast( preBuildFunc_t ) library_get_proc_address( library, PRE_BUILD_FUNC_NAME );
		postBuildFunc = cast( postBuildFunc_t ) library_get_proc_address( library, POST_BUILD_FUNC_NAME );
	}

	// get all the "compilation units" that we are actually going to give to the compiler
	// if no source files were added in set_builder_options() then assume we want to build the same file as the one specified via the command line
	if ( context.options.source_files.size() == 0 ) {
		context.options.source_files.push_back( inputFile );
	} else {
		// otherwise the user told us to build other source files, so go find and build those instead
		// keep this as a std::vector because this gets fed back into BuilderOptions::source_files
		std::vector<const char*> finalSourceFilesToBuild;

		For ( u64, sourceFileIndex, 0, context.options.source_files.size() ) {
			const char* sourceFile = context.options.source_files[sourceFileIndex];

			FileInfo fileInfo;
			File firstFile = file_find_first( tprintf( "%s\\%s", buildFilePathAbsolute, sourceFile ), &fileInfo );

			if ( firstFile.ptr == INVALID_HANDLE_VALUE ) {
				error( "Source file \"%s\" can't be found.  Is it correct?\n", sourceFile );
				return 1;
			}

			do {
				if ( string_equals( sourceFile, "." ) || string_equals( sourceFile, ".." ) ) {
					continue;
				}

				const char* foundSourceFile = NULL;

				if ( PathHasSlash( sourceFile ) ) {
					const char* localPath = paths_remove_file_from_path( sourceFile );

					foundSourceFile = tprintf( "%s\\%s\\%s", buildFilePathAbsolute, localPath, fileInfo.filename );
				} else {
					foundSourceFile = tprintf( "%s\\%s", buildFilePathAbsolute, fileInfo.filename );
				}

				finalSourceFilesToBuild.push_back( foundSourceFile );
			} while ( file_find_next( &firstFile, &fileInfo ) );
		}

		context.options.source_files = finalSourceFilesToBuild;
	}

	// recursively resolve all includes found in each source file
	Array<const char*> buildInfoFiles;
	{
		array_add_range( &buildInfoFiles, context.options.source_files.data(), context.options.source_files.size() );

		// for each file, open it and get every include inside it
		// then go through _those_ included files
		For ( u64, sourceFileIndex, 0, buildInfoFiles.count ) {
			const char* sourceFile = buildInfoFiles[sourceFileIndex];

			const char* sourceFilePath = paths_remove_file_from_path( sourceFile );
			const char* sourceFileNoPath = paths_remove_path_from_file( sourceFile );

			char* fileBuffer = NULL;
			u64 fileLength = 0;
			bool8 read = file_read_entire( sourceFile, &fileBuffer, &fileLength );

			if ( !read ) {
				//error( "Failed to read %s.  Can't resolve includes for this file.\n", buildInfoFiles[sourceFileIndex] );
				//return EXIT_FAILURE;
				continue;
			}

			defer( file_free_buffer( &fileBuffer ) );

			For ( u64, fileOffset, 0, fileLength ) {
				if ( string_starts_with( fileBuffer + fileOffset, "#include" ) ) {
					fileOffset += strlen( "#include" );

					while ( fileBuffer[fileOffset] == ' ' ) {
						fileOffset++;
					}

					if ( fileBuffer[fileOffset] == '"' ) {
						// "local" include, so scan from where we are
						const char* includeStart = fileBuffer + fileOffset;
						includeStart++;

						const char* includeEnd = strchr( includeStart, '"' );

						u64 filenameLength = cast( u64 ) includeEnd - cast( u64 ) includeStart;
						filenameLength++;

						char* filename = cast( char* ) mem_temp_alloc( filenameLength * sizeof( char ) );
						strncpy( filename, includeStart, filenameLength * sizeof( char ) );
						filename[filenameLength - 1] = 0;

						filename = tprintf( "%s\\%s", sourceFilePath, filename );

						bool8 found = false;
						For ( u64, fileIndex, 0, buildInfoFiles.count ) {
							if ( string_equals( buildInfoFiles[fileIndex], filename ) ) {
								found = true;
								break;
							}
						}

						if ( !found ) {
							array_add( &buildInfoFiles, filename );
						}
					} else if ( fileBuffer[fileOffset] == '<' ) {
						// "external" include, so scan all the external include folders that we know about
						const char* includeStart = fileBuffer + fileOffset;
						includeStart++;

						const char* includeEnd = strchr( includeStart, '>' );

						u64 filenameLength = cast( u64 ) includeEnd - cast( u64 ) includeStart;
						filenameLength++;

						char* filename = cast( char* ) mem_temp_alloc( filenameLength * sizeof( char ) );
						strncpy( filename, includeStart, filenameLength * sizeof( char ) );
						filename[filenameLength - 1] = 0;

						const char* fullFilename = NULL;

						For ( u64, includePathIndex, 0, context.options.additional_includes.size() ) {
							const char* includePath = context.options.additional_includes[includePathIndex];

							fullFilename = TryFindFile( filename, includePath );

							if ( fullFilename != NULL ) {
								break;
							}
						}

						if ( fullFilename ) {
							bool8 found = false;
							For ( u64, fileIndex, 0, buildInfoFiles.count ) {
								if ( string_equals( buildInfoFiles[fileIndex], fullFilename ) ) {
									found = true;
									break;
								}
							}

							if ( !found ) {
								array_add( &buildInfoFiles, fullFilename );
							}
						}
						/*else {
							error( "Failed to find external include \"%s\" from any of the additional include directories.\n" );
						}*/
					}
				}

				const char* lineEnd = strchr( fileBuffer + fileOffset, '\n' );
				if ( !lineEnd ) lineEnd = strstr( fileBuffer + fileOffset, "\r\n" );
				if ( !lineEnd ) lineEnd = fileBuffer + fileOffset;

				u64 fileLineLength = cast( u64 ) lineEnd - cast( u64 ) ( fileBuffer + fileOffset );
				fileLineLength = maxull( fileLineLength, 1ULL );	// TODO(DM): this line is hiding a bug in the parser - find the bug

				fileOffset += fileLineLength;
			}
		}
	}

	if ( !context.options.binary_folder.empty() ) {
		context.options.binary_folder = tprintf( "%s\\%s", buildFilePathAbsolute, context.options.binary_folder.c_str() );
	} else {
		context.options.binary_folder = buildFilePathAbsolute;
	}

	folder_create_if_it_doesnt_exist( context.options.binary_folder.c_str() );

	// user didnt override the binary name via the callback
	// so give them a binary name based off the first source file
	if ( context.options.binary_name.empty() ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		char* firstSourceFileWithoutExtension = cast( char* ) mem_temp_alloc( ( firstSourceFileLength + 1 ) * sizeof( char ) );
		strncpy( firstSourceFileWithoutExtension, firstSourceFile, firstSourceFileLength * sizeof( char ) );
		firstSourceFileWithoutExtension[firstSourceFileLength] = 0;

		firstSourceFileWithoutExtension = cast( char* ) paths_remove_file_extension( paths_remove_path_from_file( firstSourceFileWithoutExtension ) );

		context.options.binary_name = firstSourceFileWithoutExtension;
#pragma clang diagnostic pop
	}

	context.fullBinaryName = tprintf( "%s\\%s", context.options.binary_folder.c_str(), context.options.binary_name.c_str() );

	if ( !context.options.remove_file_extension ) {
		context.fullBinaryName = tprintf( "%s.%s", context.fullBinaryName, GetFileExtensionFromBinaryType( context.options.binary_type ) );
	}

	if ( preBuildFunc ) {
		preBuildFunc( &context.options );
	}

	// now do the actual build
	switch ( context.options.binary_type ) {
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

	if ( postBuildFunc ) {
		postBuildFunc( &context.options );
	}

	// if the build was successful, write the .build_info file now
	// get the timestamp of when each source file was last written to
	if ( exitCode == 0 ) {
		const char* buildInfoFilename = tprintf( "%s\\%s.%s", dotBuilderFolder, firstSourceFileNoPath, BUILD_INFO_FILE_EXTENSION );

		File buildInfoFile = file_open_or_create( buildInfoFilename );
		defer( file_close( &buildInfoFile ) );

		For ( u32, i, 0, buildInfoFiles.count ) {
			const char* sourceFilename = buildInfoFiles[i];

			FileInfo sourceFileInfo;
			File sourceFile = file_find_first( sourceFilename, &sourceFileInfo );

			const char* sourceFileWriteTime = tprintf( "%s: %llu\n", sourceFilename, sourceFileInfo.last_write_time );
			bool8 written = file_write( &buildInfoFile, sourceFileWriteTime, strlen( sourceFileWriteTime ) );

			if ( !written ) {
				error( "Failed to write last write time for source file \"%s\".  Builder will trigger a rebuild of \"%s\" next time you want to build it even if a source file has not changed.\n", sourceFilename, context.fullBinaryName );
				exitCode = 1;
			}
		}
	} else {
		error( "Build failed.\n" );
		exitCode = 1;
	}

	return exitCode;
}

// project type guids are pre-determined by visual studio
// C++ is the only language that builder knows/cares about
#define VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"

#define CHECK_WRITE( func ) \
	do { \
		bool8 written = (func); \
		assertf( written, "Failed to write a visual studio project/solution file..\n" ); \
	} while ( 0 )

// data layout comes from: https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
static const char* CreateVisualStudioGuid() {
	GUID guid;
	HRESULT hr = CoCreateGuid( &guid );
	assert( hr == S_OK );

	return tprintf( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}

static void FindAllFiles( const char* basePath, const char* searchFilter, Array<const char*>* outFiles ) {
	const char* fullSearchPath = tprintf( "%s\\%s", basePath, searchFilter );

	FileInfo fileInfo;
	File firstFile = file_find_first( fullSearchPath, &fileInfo );

	const char* searchFilterPath = paths_remove_file_from_path( searchFilter );

	do {
		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
			continue;
		}

		const char* fullFilename = tprintf( "%s\\%s", searchFilterPath, fileInfo.filename );

		if ( fileInfo.is_directory ) {
			FindAllFiles( basePath, fullFilename, outFiles );
		} else {
			array_add( outFiles, fullFilename );
		}
	} while ( file_find_next( &firstFile, &fileInfo ) );
}

static bool8 GenerateVisualStudioSolution( VisualStudioSolution* solution, const char* inputFilePath ) {
	assert( solution );

	// validate the solution
	{
		if ( solution->name == NULL ) {
			error( "You never set the name of the solution.  I need that.\n" );
			return false;
		}

		/*if ( solution->configs.size() < 1 ) {
			error( "You must set at least one config when generating a Visual Studio Solution.\n" );
			return false;
		}*/

		if ( solution->platforms.size() < 1 ) {
			error( "You must set at least one platform when generating a Visual Studio Solution.\n" );
			return false;
		}

		if ( solution->projects.size() < 1 ) {
			error( "As well as a Solution, you must also generate at least one Visual Studio Project to go with it.\n" );
			return false;
		}
	}

	const char* visualStudioProjectFilesPathAbsolute = NULL;
	if ( solution->path ) {
		visualStudioProjectFilesPathAbsolute = tprintf( "%s\\%s", inputFilePath, solution->path );
	} else {
		visualStudioProjectFilesPathAbsolute = inputFilePath;
	}

	Array<const char*> projectGuids;
	array_resize( &projectGuids, solution->projects.size() );

	// give each project a guid
	For ( u64, i, 0, projectGuids.count ) {
		projectGuids[i] = CreateVisualStudioGuid();
	}

	printf( "Generating Projects:\n" );

	// generate each .vcxproj
	For ( u64, projectIndex, 0, solution->projects.size() ) {
		VisualStudioProject* project = &solution->projects[projectIndex];

		// validate the project
		{
			if ( project->name == NULL ) {
				error( "There is a Visual Studio Project that doesn't have a name here.  You need to fill that in.\n" );
				return false;
			}

			if ( project->source_files.size() == 0 ) {
				error( "No source files were set for project \"%s\".  You need at least one source file.\n", project->name );
				return false;
			}

			// validate each config
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				if ( config->name == NULL ) {
					error( "There is a config for project \"%s\" that doesn't have a name here.  You need to fill that in.\n", project->name );
					return false;
				}

				/*if ( config->build_source_file == NULL ) {
					error( "Build source file for project \"%s\" config \"%s\" was never set.  You need to fill that in.\n", project->name, config->name );
					return false;
				}

				if ( config->binary_name == NULL ) {
					error( "Binary name for project \"%s\" config \"%s\" was never set.  You need to fill that in.\n", project->name, config->name );
					return false;
				}

				if ( config->output_path == NULL ) {
					error( "Output path for project \"%s\" config \"%s\" was never set.  You need to fill that in.\n", project->name, config->name );
					return false;
				}

				if ( config->intermediate_path == NULL ) {
					config->intermediate_path = config->output_path;
				}*/
			}
		}

		// .vcxproj
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj", visualStudioProjectFilesPathAbsolute, project->name );

			printf( "Generating %s.vcxproj ... ", project->name );

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
						CHECK_WRITE( file_write_line( &vcxproj,			"\t\t</ProjectConfiguration>" ) );
					}
				}
				CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
			}

			// project globals
			{
				CHECK_WRITE( file_write_line( &vcxproj,			"\t<PropertyGroup Label=\"Globals\">" ) );
				CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<VCProjectVersion>17.0</VCProjectVersion>" ) );
				CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ProjectGuid>{%s}</ProjectGuid>", projectGuids[projectIndex] ) ) );
				CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>" ) );
				CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<Keyword>MakeFileProj</Keyword>" ) );
				CHECK_WRITE( file_write_line( &vcxproj,			"\t</PropertyGroup>" ) );
			}

			CHECK_WRITE( file_write_line( &vcxproj, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />" ) );

			// for each config and platform, define config type, toolset, out dir, and intermediate dir
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, solution->platforms.size() ) {
					const char* platform = solution->platforms[platformIndex];

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\" Label=\"Configuration\">", config->name, platform ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<ConfigurationType>Makefile</ConfigurationType>" ) );
					CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<UseDebugLibraries>false</UseDebugLibraries>" ) );
					CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<PlatformToolset>v143</PlatformToolset>" ) );

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<OutDir>%s</OutDir>", config->output_directory ) ) );
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<IntDir>%s</IntDir>", tprintf( "%s\\intermediate", config->options.binary_folder.c_str() ) ) ) );
					CHECK_WRITE( file_write_line( &vcxproj,			"\t</PropertyGroup>" ) );
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
					CHECK_WRITE( file_write_line( &vcxproj,			"\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />" ) );
					CHECK_WRITE( file_write_line( &vcxproj,			"\t</ImportGroup>" ) );
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
						CHECK_WRITE( file_write( &vcxproj, tprintf( "%s;", config->options.additional_includes[includePathIndex] ) ) );
					}
					CHECK_WRITE( file_write( &vcxproj, "$(ExternalIncludePath)" ) );
					CHECK_WRITE( file_write( &vcxproj, "</ExternalIncludePath>\n" ) );

					// external library paths
					CHECK_WRITE( file_write( &vcxproj, "\t\t<LibraryPath>" ) );
					For ( u64, libPathIndex, 0, config->options.additional_lib_paths.size() ) {
						CHECK_WRITE( file_write( &vcxproj, tprintf( "%s;", config->options.additional_lib_paths[libPathIndex] ) ) );
					}
					CHECK_WRITE( file_write( &vcxproj, "$(LibraryPath)" ) );
					CHECK_WRITE( file_write( &vcxproj, "</LibraryPath>\n" ) );

					// output path
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeOutput>%s</NMakeOutput>", config->options.binary_folder.c_str() ) ) );

#if VS_GENERATE_BUILD_SOURCE_FILES
					const char* buildSourceFile = tprintf( "%s\\build_%s.%s.cpp", inputFilePath, project->name, platform );
#else
					const char* buildSourceFile = config->build_source_file;
#endif

					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeBuildCommandLine>%s\\builder.exe %s %s%s</NMakeBuildCommandLine>", paths_get_app_path(), buildSourceFile, ARG_CONFIG, config->name ) ) );
					CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<NMakeReBuildCommandLine>%s\\builder.exe %s %s%s</NMakeReBuildCommandLine>", paths_get_app_path(), buildSourceFile, ARG_CONFIG, config->name ) ) );
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
				CHECK_WRITE( file_write_line( &vcxproj, "\t<ItemGroup>" ) );

				For ( u64, fileTypeIndex, 0, project->source_files.size() ) {
					const char* searchPath = project->source_files[fileTypeIndex];

					Array<const char*> files;
					FindAllFiles( visualStudioProjectFilesPathAbsolute, searchPath, &files );

					For ( u64, fileIndex, 0, files.count ) {
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<ClCompile Include=\"%s\" />", files[fileIndex] ) ) );
					}
				}

				CHECK_WRITE( file_write_line( &vcxproj, "\t</ItemGroup>" ) );
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
			const char* projectPath = tprintf( "%s\\%s.vcxproj.user", visualStudioProjectFilesPathAbsolute, project->name );

			printf( "Generating %s.vcxproj.user ... ", project->name );

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

					For ( u64, platformIndex, 0, solution->platforms.size() ) {
						const char* platform = solution->platforms[platformIndex];

						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">", config->name, platform ) ) );
						CHECK_WRITE( file_write_line( &vcxproj,			 "\t\t<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>" ) );	// TODO(DM): do want to include the other debugger types?
						CHECK_WRITE( file_write_line( &vcxproj,			 "\t\t<LocalDebuggerDebuggerType>Auto</LocalDebuggerDebuggerType>" ) );
						CHECK_WRITE( file_write_line( &vcxproj,			 "\t\t<LocalDebuggerAttach>false</LocalDebuggerAttach>" ) );
						CHECK_WRITE( file_write_line( &vcxproj, tprintf( "\t\t<LocalDebuggerCommand>%s\\%s</LocalDebuggerCommand>", config->output_directory, config->options.binary_name.c_str() ) ) );
						CHECK_WRITE( file_write_line( &vcxproj,			 "\t\t<LocalDebuggerWorkingDirectory>$(SolutionDir)</LocalDebuggerWorkingDirectory>" ) );

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
	}

	// now generate .sln file
	{
		const char* solutionPath = tprintf( "%s\\%s.sln", visualStudioProjectFilesPathAbsolute, solution->name );

		printf( "Generating %s.sln ... ", solution->name );

		File sln = file_open_or_create( solutionPath );
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
			//CHECK_WRITE( file_write_line( &sln,			"\tGlobalSection(ExtensibilityGlobals) = postSolution" ) );
			//CHECK_WRITE( file_write_line( &sln, tprintf( "\t\tSolutionGuid = {%s}", solutionGUID ) ) );
			//CHECK_WRITE( file_write_line( &sln,			"\tEndGlobalSection" ) );
		}
		CHECK_WRITE( file_write_line( &sln, "EndGlobal" ) );

		printf( "Done\n" );
	}

#if VS_GENERATE_BUILD_SOURCE_FILES
	// generate build source files for each config/platform combo
	{
		For ( u64, projectIndex, 0, solution->projects.size() ) {
			VisualStudioProject* project = &solution->projects[projectIndex];

			For ( u64, platformIndex, 0, solution->platforms.size() ) {
				const char* platform = solution->platforms[platformIndex];

				File buildSourceFile = file_open_or_create( tprintf( "%s\\build_%s.%s.cpp", inputFilePath, project->name, platform ) );
				defer( file_close( &buildSourceFile ) );

				CHECK_WRITE( file_write( &buildSourceFile, "// This file was generated by Builder as part of your Visual Studio Solution.\n" ) );
				CHECK_WRITE( file_write( &buildSourceFile, "// If you want to edit this file, you probably want to change your BuilderOptions wherever your hook is to \"" SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME "( VisualStudioSolution* )\" and re-generate the solution.\n" ) );
				CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
				CHECK_WRITE( file_write( &buildSourceFile, "#include \"../../builder.h\"\n" ) );
				CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );

				if ( project->configs.size() > 1 ) {
					CHECK_WRITE( file_write( &buildSourceFile, "#include <string.h> // strcmp\n\n" ) );
				}

				CHECK_WRITE( file_write( &buildSourceFile, "BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {\n" ) );

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\tif ( options->config == \"%s\" ) {\n", config->name ) ) );

					if ( config->options.source_files.size() > 0 ) {
						For ( u64, sourceFileIndex, 0, config->options.source_files.size() ) {
							CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->source_files.push_back( \"%s\" );\n", config->options.source_files[sourceFileIndex] ) ) );
						}

						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}

					if ( config->options.defines.size() > 0 ) {
						For ( u64, defineIndex, 0, config->options.defines.size() ) {
							CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->defines.push_back( \"%s\" );\n", config->options.defines[defineIndex] ) ) );
						}

						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}

					if ( config->options.additional_includes.size() > 0 ) {
						For ( u64, includeIndex, 0, config->options.additional_includes.size() ) {
							CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->additional_includes.push_back( \"%s\" );\n", config->options.additional_includes[includeIndex] ) ) );
						}

						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}

					if ( config->options.additional_lib_paths.size() > 0 ) {
						For ( u64, libPathIndex, 0, config->options.additional_lib_paths.size() ) {
							CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->additional_lib_paths.push_back( \"%s\" );\n", config->options.additional_lib_paths[libPathIndex] ) ) );
						}

						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}

					if ( config->options.additional_libs.size() > 0 ) {
						For ( u64, libIndex, 0, config->options.additional_libs.size() ) {
							CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->additional_libs.push_back( \"%s\" );\n", config->options.additional_libs[libIndex] ) ) );
						}

						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}

					CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->binary_type = %s;\n", BinaryTypeToString( config->options.binary_type ) ) ) );

					//CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\toptions->optimization_level = %s;\n", OptimizationLevelToString( config->options.optimization_level ) ) ) );
					switch ( config->options.optimization_level ) {
						case OPTIMIZATION_LEVEL_O0: CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->optimization_level = OPTIMIZATION_LEVEL_O0;\n" ) ) ); break;
						case OPTIMIZATION_LEVEL_O1: CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->optimization_level = OPTIMIZATION_LEVEL_O1;\n" ) ) ); break;
						case OPTIMIZATION_LEVEL_O2: CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->optimization_level = OPTIMIZATION_LEVEL_O2;\n" ) ) ); break;
						case OPTIMIZATION_LEVEL_O3: CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->optimization_level = OPTIMIZATION_LEVEL_O3;\n" ) ) ); break;
					}

					if ( config->options.remove_symbols ) {
						CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->remove_symbols = %s;\n", config->options.remove_symbols ? "true" : "false" ) ) );
					}

					if ( config->options.remove_file_extension ) {
						CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->remove_file_extension = %s;\n", config->options.remove_file_extension ? "true" : "false" ) ) );
					}

					if ( config->options.binary_folder.empty() == false ) {
						CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->binary_folder = \"%s\";\n", config->options.binary_folder.c_str() ) ) );
					}

					if ( config->options.binary_name.empty() == false ) {
						CHECK_WRITE( file_write( &buildSourceFile, tprintf( "\t\toptions->binary_name = \"%s\";\n", config->options.binary_name.c_str() ) ) );
					}

					CHECK_WRITE( file_write( &buildSourceFile, "\t}\n" ) );

					if ( configIndex != project->configs.size() - 1 ) {
						CHECK_WRITE( file_write( &buildSourceFile, "\n" ) );
					}
				}

				CHECK_WRITE( file_write( &buildSourceFile, "}\n" ) );
			}
		}
	}
#endif // VS_GENERATE_BUILD_SOURCE_FILES

	return true;
}

//static bool8 GenerateVisualStudioProject( VisualStudioProject* project )
//{
//	assert( project );
//
//	File vcxproj = file_open_or_create( tprintf( "%s.vcxproj",project->name ) );
//	defer( file_close( &vcxproj ) );
//
//	if ( vcxproj.ptr == nullptr )
//	{
//		return false;
//	}
//
//	file_write_line( &vcxproj,					"<?xml version=\"1.0\" encoding=\"utf-8\"?>" );
//
//	file_write_line( &vcxproj,					"<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" );
//
//	file_write_line( &vcxproj,					"  <ItemGroup Label=\"ProjectConfigurations\">" );
//	For ( u64, i, 0, project->configs.count ) {
//		// TODO: Alternative targets
//		file_write_line( &vcxproj, tprintf(		"    <ProjectConfiguration Include=\"%s\"\">"				, project->configs[i] ) );
//		file_write_line( &vcxproj, tprintf(		"      <Configuration>%s</Configuration>"					, project->configs[i] ) );
//		file_write_line( &vcxproj,				"      <Platform>x64</Platform>"																  );
//		file_write_line( &vcxproj,				"    </ProjectConfiguration>"																	  );
//	}
//	file_write_line( &vcxproj,					"  </ItemGroup>" );
//
//	file_write_line( &vcxproj,					"  <PropertyGroup Label=\"Globals\">" 						);
//	file_write_line( &vcxproj, tprintf(			"    <ProjectGuid> %u </ProjectGuid>" 						, hash_string(project->project_name , 59049 ) ) );
//	file_write_line( &vcxproj,					"    <Keyword>Win32Proj</Keyword>" 							);
//	file_write_line( &vcxproj, tprintf(			"    <RootNamespace> %s </RootNamespace>" 					, project->project_name ) );
//	file_write_line( &vcxproj,					"  </PropertyGroup>"						 				);
//
//	file_write_line( &vcxproj,					"  <ItemGroup>"												);
//	For ( u64, i, 0, project->source_files.count ) {
//		// TODO: deep recursive file search for ccp, h, c, hpp, inl files
//		file_write_line( &vcxproj, tprintf(		"    <ClCompile Include=\"%s\" />"							, project->source_files[i]				) );
//	}
//	file_write_line( &vcxproj,					"  </ItemGroup>"											);
//
//	file_write_line( &vcxproj,					"  <PropertyGroup Condition=\"'$(VisualStudioVersion)' == '16.0'\">" 				);
//	file_write_line( &vcxproj,					"      <PlatformToolset>v142</PlatformToolset> <!-- VS 2019 -->" 					);
//	file_write_line( &vcxproj,					"  </PropertyGroup>" 																);
//	file_write_line( &vcxproj,					"  <PropertyGroup Condition=\"'$(VisualStudioVersion)' == '17.0'\">" 				);
//	file_write_line( &vcxproj,					"      <PlatformToolset>v143</PlatformToolset> <!-- VS 2022 -->" 					);
//	file_write_line( &vcxproj,					"  </PropertyGroup>" 																);
//
//	file_write_line( &vcxproj, 					"</Project>"																		);
//
//	//This bit is a bit of guff- I got GPT to write it for me and it only really works for debug and release, Win32 && x64. Need a first pass though.
//	For ( u64, platform_idx, 0, project->platforms.count )
//	{
//		const char* platform = project->platforms[platform_idx]; // Example: "x64", "Win32"
//
//		For ( u64, config_idx, 0, project->configs.count )
//		{
//			const char* config = project->configs[config_idx]; // Example: "Debug", "Release"
//
//			file_write_line( &vcxproj, tprintf( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">", config, platform ) );
//			
//			// Note(TOM): this assumes two things: 
//			//			A) you use vis studio generation ONLY to point to a build script; NOT the actual source files. This is because otherwise you need to get source per config 
//			//					which would be tricky since they are conditionally defined in build options. NOTE: the source files in this options is NOT appropiate, 
//			//					since it's the superset of all the files needed for SLN visibility, not the ones that should actually get compiled (eg linux files visible despite building windows)
//			//			B) there's only one cpp involved in your build script. TODO(TOM): make an array. This assumption doesn't need to stick
//			file_write_line( &vcxproj, tprintf( "    <NMakeBuildCommandLine>builder.exe %s %s</NMakeBuildCommandLine>", project->build_script_path, config ) );
//
//			//NOTE(TOM): OUTPUT: This is tricky. I think it's needed for the debugger. The output location is defined in the build script and could be different per config.
//			//					That means duplicating logic between the two. That's gnarly. 
//			//					Perhaps there could be an intermediate set build location at config/platform/build.exe we could use for VS debugging, and then copy that file to user's custom location
//			file_write_line( &vcxproj, tprintf( "    <NMakeOutput>??? This is difficult cos it's so dependent on the build logic/NMakeOutput>", config ) );
//			file_write_line( &vcxproj, tprintf( "    <NMakeBuildCommandLine>builder.exe %s %s</NMakeBuildCommandLine>", project->build_script_path, config ) );
//			
//			// NOTE(TOM): "Standard" preprocessor defns. Ternaries aren't appropiate for configs that aren't debug or release but I wanted SOMETHING working
//			const char* win32_definition = string_equals( platform, "Win32" ) ? "WIN32;"  : "";
//			const char* debug_definition = string_equals( config,   "Debug" ) ? "_DEBUG;" : "NDEBUG;";
//
//			// Write the preprocessor definitions
//			file_write_line(
//				&vcxproj,
//				tprintf(
//					"    <NMakePreprocessorDefinitions>%s%s$(NMakePreprocessorDefinitions)</NMakePreprocessorDefinitions>",
//					win32_definition,    // WIN32 definition if the platform is Win32
//					debug_definition     // _DEBUG for Debug, NDEBUG for Release
//				)
//			);
//
//			file_write_line( &vcxproj, "  </PropertyGroup>" );
//		}
//	}
//
//	file_write_line( &vcxproj,					"</Project>\n" );
//
//	return true;
//}