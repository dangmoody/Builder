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
================================================================================================

	Builder

	IF YOU MAKE ANY CHANGES HERE YOU WILL NEED TO DO THE FOLLOWING:
		1. Update the version number accordingly.
		2. Make sure you build both debug and release.
		3. Submit the newly built binaries.

	This is our custom build tool that is our way of not having to rely on batch/shell scripts.

	We do this as an EXE for speed (we want fast build times and python is too slow).

	Builder deliberately does not link against ANYTHING.  It only has one include dependency,
	which is to Core (which doesn't link against anything anyway) and that's it.

	TODO(DM):
		* if --out= is not specified, and if only one source file was given then just build a
		  .EXE where the name of the .EXE is the name of the source file (this is what Jai does)
		* ALL the optimisation

================================================================================================
*/

enum {
	BUILDER_VERSION_MAJOR	= 0,
	BUILDER_VERSION_MINOR	= 1,
	BUILDER_VERSION_PATCH	= 0,
};

#define ARG_HELP_SHORT	"-h"
#define ARG_HELP_LONG	"--help"
#define ARG_NUKE		"--nuke"
#define ARG_CONFIG		"--config="
#define ARG_SOLUTION	"--solution"

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
		case BINARY_TYPE_EXE: return "exe";
		case BINARY_TYPE_DLL: return "dll";
		case BINARY_TYPE_LIB: return "lib";
	}

	assertf( false, "Something went really wrong here.  Get Dan.\n" );

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
		"    Builder.exe <source files> --<config>\n"
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
		"    " ARG_SOLUTION " <file> (optional):\n"
		"        Use this argument to generate a Visual Studio solution and projects.\n"
		"        <file> refers to the source file where you will configure your solution.\n"
		"\n"
	);

	return exitCode;
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
		context->options.source_files.count +
		context->options.defines.count +
		context->options.additional_includes.count +
		context->options.additional_lib_paths.count +
		context->options.additional_libs.count +
		5 +	// warning levels
		context->options.ignore_warnings.count
	);

	array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );

	For ( u64, i, 0, context->options.source_files.count ) {
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

	array_add_range( &args, context->options.source_files.data, context->options.source_files.count );

	For ( u32, i, 0, context->options.defines.count ) {
		array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
	}

	For ( u32, i, 0, context->options.additional_includes.count ) {
		array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
	}

	For ( u32, i, 0, context->options.additional_lib_paths.count ) {
		array_add( &args, tprintf( "-L%s", context->options.additional_lib_paths[i] ) );
	}

	For ( u32, i, 0, context->options.additional_libs.count ) {
		array_add( &args, tprintf( "-l%s", context->options.additional_libs[i] ) );
	}

	// must come before ignored warnings
	array_add( &args, "-Werror" );
	array_add( &args, "-Weverything" );
	array_add( &args, "-Wall" );
	array_add( &args, "-Wextra" );
	array_add( &args, "-Wpedantic" );

	if ( context->options.ignore_warnings.count > 0 ) {
		array_add_range( &args, context->options.ignore_warnings.data, context->options.ignore_warnings.count );
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
		context->options.source_files.count +
		context->options.defines.count +
		context->options.additional_includes.count +
		context->options.additional_lib_paths.count +
		context->options.additional_libs.count +
		5 +	// warning levels
		context->options.ignore_warnings.count
	);

	array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
	array_add( &args, "-shared" );

	For ( u64, i, 0, context->options.source_files.count ) {
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

	array_add_range( &args, context->options.source_files.data, context->options.source_files.count );

	For ( u32, i, 0, context->options.defines.count ) {
		array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
	}

	For ( u32, i, 0, context->options.additional_includes.count ) {
		array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
	}

	For ( u32, i, 0, context->options.additional_lib_paths.count ) {
		array_add( &args, tprintf( "-L%s", context->options.additional_lib_paths[i] ) );
	}

	For ( u32, i, 0, context->options.additional_libs.count ) {
		array_add( &args, tprintf( "-l%s", context->options.additional_libs[i] ) );
	}

	// must come before ignored warnings
	array_add( &args, "-Werror" );
	array_add( &args, "-Weverything" );
	array_add( &args, "-Wall" );
	array_add( &args, "-Wextra" );
	array_add( &args, "-Wpedantic" );

	if ( context->options.ignore_warnings.count > 0 ) {
		array_add_range( &args, context->options.ignore_warnings.data, context->options.ignore_warnings.count );
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
		context->options.source_files.count +
		context->options.defines.count +
		context->options.additional_includes.count +
		context->options.additional_lib_paths.count +
		context->options.additional_libs.count +
		5 +	// warning levels
		context->options.ignore_warnings.count
	);

	// build .o files of all compilation units
	For ( u64, sourceFileIndex, 0, context->options.source_files.count ) {
		const char* sourceFile = context->options.source_files[sourceFileIndex];

		array_reset( &args );

		array_add( &args, tprintf( "%s\\clang\\bin\\clang.exe", paths_get_app_path() ) );
		array_add( &args, "-c" );

		if ( string_ends_with( context->options.source_files[sourceFileIndex], ".cpp" ) ) {
			array_add( &args, "-std=c++20" );
		} else if ( string_ends_with( context->options.source_files[sourceFileIndex], ".c" ) ) {
			array_add( &args, "-std=c99" );
		} else {
			assertf( false, "Something went really wrong.  Get Dan.\n" );
			return 1;
		}

		if ( !context->options.remove_symbols ) {
			array_add( &args, "-g" );
		}

		array_add( &args, OptimizationLevelToString( context->options.optimization_level ) );

		{
			const char* sourceFileTrimmed = context->options.source_files[sourceFileIndex];
			sourceFileTrimmed = strrchr( sourceFileTrimmed, '\\' ) + 1;

			const char* outArg = tprintf( "%s\\%s.o", context->options.binary_folder, sourceFileTrimmed );

			array_add( &args, "-o" );
			array_add( &args, outArg );
			array_add( &intermediateFiles, outArg );
		}

		array_add( &args, sourceFile );

		For ( u32, i, 0, context->options.defines.count ) {
			array_add( &args, tprintf( "-D%s", context->options.defines[i] ) );
		}

		For ( u32, i, 0, context->options.additional_includes.count ) {
			array_add( &args, tprintf( "-I%s", context->options.additional_includes[i] ) );
		}

		// must come before ignored warnings
		array_add( &args, "-Werror" );
		array_add( &args, "-Weverything" );
		array_add( &args, "-Wall" );
		array_add( &args, "-Wextra" );
		array_add( &args, "-Wpedantic" );

		if ( context->options.ignore_warnings.count > 0 ) {
			array_add_range( &args, context->options.ignore_warnings.data, context->options.ignore_warnings.count );
		}

		array_add( &args, NULL );

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

		//array_add( &args, NULL );

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

static const char* TryFindFile( const char* filename, const char* folder ) {
	const char* result = NULL;

	const char* searchPath = tprintf( "%s\\*", folder );

	FileInfo fileInfo;
	File firstFile = file_find_first( searchPath, &fileInfo );

	do {
		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) || fileInfo.filename[0] == 0 ) {
			continue;
		}

		const char* fullFilename = tprintf( "%s\\%s", folder, fileInfo.filename );

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

static bool8 GenerateVisualStudioSolution( VisualStudioOptions* visualStudioOptions );

int main( int argc, char** argv ) {
	float64 buildStart = time_ms();
	defer(
		float64 buildEnd = time_ms();
		printf( "Build finished: %f ms\n\n", buildEnd - buildStart );
	);

	core_init( MEM_KILOBYTES( 1 ), MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	set_command_line_args( argc, argv );

	printf( "Builder v%d.%d.%d\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context;
	memset( &context, 0, sizeof( buildContext_t ) );
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
				assertf( exitCode == 0, "Something went terribly wrong.  Go get Dan.\n" );

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
					error( "Failed to download Clang.  The CURL HTTP request failed.  Speak to Dan.\n" );
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
					error( "Failed to install Clang.  Speak to Dan.\n" );
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

		if ( string_equals( arg, ARG_SOLUTION ) ) {
			const char* nextArg = ( argIndex + 1 < argc ) ? argv[argIndex + 1] : NULL;

			if ( nextArg ) {
				visualStudioConfigSourceFile = nextArg;
			} else {
				error(
					ARG_SOLUTION " arg was specified, but you never gave me the source file that tells me how you want your solution to be.\n"
					"Pass a source file after this argument which calls " SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME "( VisualStudioOptions* options ).\n"
				);

				return 1;
			}

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

	typedef void ( *initCoreCallback_t )( CoreContext* coreContext );

	if ( visualStudioConfigSourceFile != NULL ) {
		printf( "Generating Visual Studio Solution ... \n" );

		const char* sourceFileAbsolute = paths_get_absolute_path( visualStudioConfigSourceFile );
		const char* sourceFilePath = paths_remove_file_from_path( sourceFileAbsolute );
		const char* dotBuilderFolder = tprintf( "%s\\.builder", sourceFilePath );
		const char* outputName = tprintf( "%s\\generate_solution.dll", dotBuilderFolder );

		folder_create_if_it_doesnt_exist( dotBuilderFolder );

		buildContext_t generateSolutionContext = {};
		memset( &generateSolutionContext, 0, sizeof( buildContext_t ) );

		generateSolutionContext.flags = BUILD_CONTEXT_FLAG_SHOW_STDOUT;
		generateSolutionContext.fullBinaryName = outputName;

		array_add( &generateSolutionContext.options.source_files, visualStudioConfigSourceFile );

		// create a temp source file which will automatically call core_hook() for us so that the user doesnt have to do it themselves
		const char* tempFileName = tprintf( "%s\\core_init.cpp", dotBuilderFolder );
		const char* content = "#include <core/core.cpp>\n"
			"\n"
			"extern \"C\" __declspec( dllexport ) void init_core( CoreContext* core ) {\n"
			"\tcore_hook( core );\n"
			"}\n";

		bool8 written = file_write_entire( tempFileName, content, strlen( content ) * sizeof( char ) );
		assertf( written, "Something went really wrong.  Go get Dan." );

		// when were finished, delete this file and remove it from the list of source files to build
		defer( file_delete( tempFileName ) );

		array_add( &generateSolutionContext.options.source_files, tempFileName );

		array_add( &generateSolutionContext.options.defines, "_CRT_SECURE_NO_WARNINGS" );
		array_add( &generateSolutionContext.options.defines, "BUILDER_DOING_USER_CONFIG_BUILD" );

		// add builder as an additional include path for the user config build so that we can automatically include core because we know where it is
		array_add( &generateSolutionContext.options.additional_includes, tprintf( "%s\\src", paths_get_app_path() ) );

#ifdef _WIN64
		array_add( &generateSolutionContext.options.additional_libs, "user32.lib" );
		array_add( &generateSolutionContext.options.additional_libs, "Shlwapi.lib" );
		array_add( &generateSolutionContext.options.additional_libs, "msvcrtd.lib" );
		array_add( &generateSolutionContext.options.additional_libs, "DbgHelp.lib" );
#endif // _WIN64

		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-newline-eof" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-c++98-compat-pedantic" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-missing-prototypes" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-old-style-cast" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-unsafe-buffer-usage" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-zero-as-null-pointer-constant" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-format-nonliteral" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-missing-field-initializers" );
		array_add( &generateSolutionContext.options.ignore_warnings, "-Wno-unused-function" );

		s32 exitCode = BuildDynamicLibrary( &generateSolutionContext );

		if ( exitCode != 0 ) {
			error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
			return 1;
		}

		Library library = library_load( outputName );

		assertf( library.ptr, "Something went really wrong.  Go get Dan.\n" );

		defer( library_unload( &library ) );

		initCoreCallback_t coreInitFunc = cast( initCoreCallback_t ) library_get_proc_address( library, "init_core" );
		assertf( coreInitFunc != NULL, "Failed to find the core init callback" );
		coreInitFunc( &g_core_context );

		typedef void ( *setVisualStudioOptionsFunc_t )( VisualStudioOptions* options );

		setVisualStudioOptionsFunc_t setVisualStudioOptionsFunc = cast( setVisualStudioOptionsFunc_t ) library_get_proc_address( library, SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME );

		if ( !setVisualStudioOptionsFunc ) {
			error(
				"You asked me to generate a Visual Studio solution via " ARG_SOLUTION ", but you never set the options for it via " SET_VISUAL_STUDIO_OPTIONS_FUNC_NAME "( VisualStudioOptions* options ).\n"
				"You need to do this so that you can tell me how you want your Visual Studio Solution to be.\n"
			);

			return 1;
		}

		VisualStudioOptions visualStudioOptions = {};
		setVisualStudioOptionsFunc( &visualStudioOptions );

		bool8 generated = GenerateVisualStudioSolution( &visualStudioOptions );

		if ( !generated ) {
			error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
			return 1;
		}

		printf( "Done.\n" );

		return 0;
	}

	// validate cmd line args
	if ( inputFile == NULL ) {
		error( "You haven't told me what source files I need to build.  I need one.\n" );

		return 1;
	}

	// set all the additional compiler options that we know we need
	{
		array_add( &context.options.defines, "_CRT_SECURE_NO_WARNINGS" );

#ifdef _WIN64
		array_add( &context.options.additional_libs, "user32.lib" );
		array_add( &context.options.additional_libs, "Shlwapi.lib" );
		array_add( &context.options.additional_libs, "msvcrtd.lib" );
		array_add( &context.options.additional_libs, "DbgHelp.lib" );
#endif // _WIN64

		array_add( &context.options.ignore_warnings, "-Wno-newline-eof" );
		array_add( &context.options.ignore_warnings, "-Wno-pointer-integer-compare" );
		array_add( &context.options.ignore_warnings, "-Wno-declaration-after-statement" );
		array_add( &context.options.ignore_warnings, "-Wno-gnu-zero-variadic-macro-arguments" );
		array_add( &context.options.ignore_warnings, "-Wno-cast-align" );
		array_add( &context.options.ignore_warnings, "-Wno-bad-function-cast" );
		array_add( &context.options.ignore_warnings, "-Wno-format-nonliteral" );
		array_add( &context.options.ignore_warnings, "-Wno-missing-braces" );
		array_add( &context.options.ignore_warnings, "-Wno-switch-enum" );
		array_add( &context.options.ignore_warnings, "-Wno-covered-switch-default" );
		array_add( &context.options.ignore_warnings, "-Wno-double-promotion" );
		array_add( &context.options.ignore_warnings, "-Wno-cast-qual" );
		array_add( &context.options.ignore_warnings, "-Wno-unused-variable" );
		array_add( &context.options.ignore_warnings, "-Wno-unused-function" );
		array_add( &context.options.ignore_warnings, "-Wno-empty-translation-unit" );
		array_add( &context.options.ignore_warnings, "-Wno-zero-as-null-pointer-constant" );
		array_add( &context.options.ignore_warnings, "-Wno-c++98-compat-pedantic" );
		array_add( &context.options.ignore_warnings, "-Wno-unused-macros" );
		array_add( &context.options.ignore_warnings, "-Wno-unsafe-buffer-usage" );			// LLVM 17.0.1
		array_add( &context.options.ignore_warnings, "-Wno-reorder-init-list" );			// C++: "designated initializers must be in order"
		array_add( &context.options.ignore_warnings, "-Wno-old-style-cast" );				// C++: "C-style casts are banned"
		array_add( &context.options.ignore_warnings, "-Wno-global-constructors" );			// C++: "declaration requires a global destructor"
		array_add( &context.options.ignore_warnings, "-Wno-exit-time-destructors" );		// C++: "declaration requires an exit-time destructor" (same as the above, basically)
		array_add( &context.options.ignore_warnings, "-Wno-missing-field-initializers" );	// LLVM 18.1.8
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
		typedef void ( *setOptionsCallback_t )( BuilderOptions* options );

		setOptionsCallback_t callback = NULL;
		initCoreCallback_t init_callback = NULL;

		// TODO(DM): 09/08/2024: why does memcpy not work for buildContext_t?
		// is it because buildContext_t contains a BuilderOptions member, which contains Array<T> which is technically not POD?
		buildContext_t userBuildConfigContext = {};
		userBuildConfigContext.options = context.options;
		userBuildConfigContext.flags = BUILD_CONTEXT_FLAG_SHOW_STDOUT;
		userBuildConfigContext.fullBinaryName = context.fullBinaryName;

		userBuildConfigContext.options.binary_folder = dotBuilderFolder;

		array_add( &userBuildConfigContext.options.source_files, inputFile );
		array_add( &userBuildConfigContext.options.defines, "BUILDER_DOING_USER_CONFIG_BUILD" );
		array_add( &userBuildConfigContext.options.ignore_warnings, "-Wno-missing-prototypes" );
		array_add( &userBuildConfigContext.options.ignore_warnings, "-Wno-unused-parameter" );

		// add builder as an additional include path for the user config build so that we can automatically include core because we know where it is
		array_add( &userBuildConfigContext.options.additional_includes, tprintf( "%s\\src", paths_get_app_path() ) );

		userBuildConfigContext.options.binary_name = tprintf( "%s.dll", paths_remove_path_from_file( firstSourceFile ) );

		userBuildConfigContext.fullBinaryName = tprintf( "%s\\%s", userBuildConfigContext.options.binary_folder, userBuildConfigContext.options.binary_name );

		folder_create_if_it_doesnt_exist( userBuildConfigContext.options.binary_folder );

		// create a temp source file which will automatically call core_hook() for us so that the user doesnt have to do it themselves
		const char* tempFileName = tprintf( "%s\\core_init.cpp", dotBuilderFolder );
		const char* content =	"#include <core/core.cpp>\n"
								"\n"
								"extern \"C\" __declspec( dllexport ) void init_core( CoreContext* core ) {\n"
								"\tcore_hook( core );\n"
								"}\n";

		bool8 written = file_write_entire( tempFileName, content, strlen( content ) * sizeof( char ) );
		assertf( written, "Something went really wrong.  Go get Dan." );

		// when were finished, delete this file and remove it from the list of source files to build
		defer( file_delete( tempFileName ) );

		array_add( &userBuildConfigContext.options.source_files, tempFileName );

		exitCode = BuildDynamicLibrary( &userBuildConfigContext );

		if ( exitCode != 0 ) {
			error( "Pre-build failed!\n" );
			return 1;
		}

		library = library_load( tprintf( "%s\\%s", userBuildConfigContext.options.binary_folder, userBuildConfigContext.options.binary_name ) );

		callback = cast( setOptionsCallback_t ) library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME );
		init_callback = cast( initCoreCallback_t ) library_get_proc_address( library, "init_core" );

		assertf( init_callback != NULL, "Failed to find the core init callback" );

		init_callback( &g_core_context );

		if ( callback ) {
			callback( &context.options );
		}

		preBuildFunc = cast( preBuildFunc_t ) library_get_proc_address( library, PRE_BUILD_FUNC_NAME );
		postBuildFunc = cast( postBuildFunc_t ) library_get_proc_address( library, POST_BUILD_FUNC_NAME );
	}

	// get all the "compilation units" that we are actually going to give to the compiler
	// if no source files were added in set_builder_options() then assume we want to build the same file as the one specified via the command line
	if ( context.options.source_files.count == 0 ) {
		array_add( &context.options.source_files, inputFile );
	} else {
		// otherwise the user told us to build other source files, so go find and build those instead
		Array<const char*> finalSourceFilesToBuild;

		For ( u64, sourceFileIndex, 0, context.options.source_files.count ) {
			const char* sourceFile = context.options.source_files[sourceFileIndex];

			FileInfo fileInfo;
			File firstFile = file_find_first( tprintf( "%s\\%s", buildFilePathAbsolute, sourceFile ), &fileInfo );

			assertf( firstFile.ptr != INVALID_HANDLE_VALUE, "Something went really wrong.  Go get Dan." );

			do {
				if ( string_equals( sourceFile, "." ) || string_equals( sourceFile, ".." ) ) {
					continue;
				}

				const char* localPath = paths_remove_file_from_path( sourceFile );

				const char* foundSourceFile = tprintf( "%s\\%s\\%s", buildFilePathAbsolute, localPath, fileInfo.filename );

				array_add( &finalSourceFilesToBuild, foundSourceFile );
			} while ( file_find_next( &firstFile, &fileInfo ) );
		}

		context.options.source_files = finalSourceFilesToBuild;
	}

	// recursively resolve all includes found in each source file
	Array<const char*> buildInfoFiles;
	{
		array_add_range( &buildInfoFiles, context.options.source_files.data, context.options.source_files.count );

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
				error( "Failed to read %s.  Can't resolve includes for this file.\n", buildInfoFiles[sourceFileIndex] );
				return 1;
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

						For ( u64, includePathIndex, 0, context.options.additional_includes.count ) {
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
				fileLineLength = maxull( fileLineLength, 1ULL );

				fileOffset += fileLineLength;
			}
		}
	}

	if ( context.options.binary_folder ) {
		context.options.binary_folder = tprintf( "%s\\%s", buildFilePathAbsolute, context.options.binary_folder );
	} else {
		context.options.binary_folder = buildFilePathAbsolute;
	}

	folder_create_if_it_doesnt_exist( context.options.binary_folder );

	// user didnt override the binary name via the callback
	// so give them a binary name based off the first source file
	if ( context.options.binary_name == NULL ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		char* firstSourceFileWithoutExtension = cast( char* ) mem_temp_alloc( ( firstSourceFileLength + 1 ) * sizeof( char ) );
		strncpy( firstSourceFileWithoutExtension, firstSourceFile, firstSourceFileLength * sizeof( char ) );
		firstSourceFileWithoutExtension[firstSourceFileLength] = 0;

		firstSourceFileWithoutExtension = cast( char* ) paths_remove_file_extension( paths_remove_path_from_file( firstSourceFileWithoutExtension ) );

		context.options.binary_name = firstSourceFileWithoutExtension;
#pragma clang diagnostic pop
	}

	context.fullBinaryName = tprintf( "%s\\%s", context.options.binary_folder, context.options.binary_name );

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

		case BINARY_TYPE_DLL:
			exitCode = BuildDynamicLibrary( &context );
			break;

		case BINARY_TYPE_LIB:
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

static bool8 GenerateVisualStudioSolution( VisualStudioOptions* visualStudioOptions )
{
	assert( visualStudioOptions );

	File vcxproj = file_open_or_create( tprintf( "%s.vcxproj",visualStudioOptions->project_name ) );
	defer( file_close( &vcxproj ) );

	if ( vcxproj.ptr == nullptr )
	{
		return false;
	}

	file_write_line( &vcxproj,					"<?xml version=\"1.0\" encoding=\"utf-8\"?>" );

	file_write_line( &vcxproj,					"<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" );

	file_write_line( &vcxproj,					"  <ItemGroup Label=\"ProjectConfigurations\">" );
	For ( u64, i, 0, visualStudioOptions->configs.count ) {
		// TODO: Alternative targets
		file_write_line( &vcxproj, tprintf(		"    <ProjectConfiguration Include=\"%s\"\">"				, visualStudioOptions->configs[i] ) );
		file_write_line( &vcxproj, tprintf(		"      <Configuration>%s</Configuration>"					, visualStudioOptions->configs[i] ) );
		file_write_line( &vcxproj,				"      <Platform>x64</Platform>"																  );
		file_write_line( &vcxproj,				"    </ProjectConfiguration>"																	  );
	}
	file_write_line( &vcxproj,					"  </ItemGroup>" );

	file_write_line( &vcxproj,					"  <PropertyGroup Label=\"Globals\">" 						);
	file_write_line( &vcxproj, tprintf(			"    <ProjectGuid> %u </ProjectGuid>" 						, hash_string(visualStudioOptions->project_name , 59049 ) ) );
	file_write_line( &vcxproj,					"    <Keyword>Win32Proj</Keyword>" 							);
	file_write_line( &vcxproj, tprintf(			"    <RootNamespace> %s </RootNamespace>" 					, visualStudioOptions->project_name ) );
	file_write_line( &vcxproj,					"  </PropertyGroup>"						 				);

	file_write_line( &vcxproj,					"  <ItemGroup>"												);
	For ( u64, i, 0, visualStudioOptions->source_files.count ) {
		// TODO: deep recursive file search for ccp, h, c, hpp, inl files
		file_write_line( &vcxproj, tprintf(		"    <ClCompile Include=\"%s\" />"							, visualStudioOptions->source_files[i]				) );
	}
	file_write_line( &vcxproj,					"  </ItemGroup>"											);

	file_write_line( &vcxproj,					"  <PropertyGroup Condition=\"'$(VisualStudioVersion)' == '16.0'\">" 				);
	file_write_line( &vcxproj,					"      <PlatformToolset>v142</PlatformToolset> <!-- VS 2019 -->" 					);
	file_write_line( &vcxproj,					"  </PropertyGroup>" 																);
	file_write_line( &vcxproj,					"  <PropertyGroup Condition=\"'$(VisualStudioVersion)' == '17.0'\">" 				);
	file_write_line( &vcxproj,					"      <PlatformToolset>v143</PlatformToolset> <!-- VS 2022 -->" 					);
	file_write_line( &vcxproj,					"  </PropertyGroup>" 																);

	file_write_line( &vcxproj, 					"</Project>"																		);

	//This bit is a bit of guff- I got GPT to write it for me and it only really works for debug and release, Win32 && x64. Need a first pass though.
	For ( u64, platform_idx, 0, visualStudioOptions->platforms.count )
	{
		const char* platform = visualStudioOptions->platforms[platform_idx]; // Example: "x64", "Win32"

		For ( u64, config_idx, 0, visualStudioOptions->configs.count )
		{
			const char* config = visualStudioOptions->configs[config_idx]; // Example: "Debug", "Release"

			file_write_line( &vcxproj, tprintf( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">", config, platform ) );
			
			// Note(TOM): this assumes two things: 
			//			A) you use vis studio generation ONLY to point to a build script; NOT the actual source files. This is because otherwise you need to get source per config 
			//					which would be tricky since they are conditionally defined in build options. NOTE: the source files in this options is NOT appropiate, 
			//					since it's the superset of all the files needed for SLN visibility, not the ones that should actually get compiled (eg linux files visible despite building windows)
			//			B) there's only one cpp involved in your build script. TODO(TOM): make an array. This assumption doesn't need to stick
			file_write_line( &vcxproj, tprintf( "    <NMakeBuildCommandLine>builder.exe %s %s</NMakeBuildCommandLine>", visualStudioOptions->build_script_path, config ) );

			//NOTE(TOM): OUTPUT: This is tricky. I think it's needed for the debugger. The output location is defined in the build script and could be different per config.
			//					That means duplicating logic between the two. That's gnarly. 
			//					Perhaps there could be an intermediate set build location at config/platform/build.exe we could use for VS debugging, and then copy that file to user's custom location
			file_write_line( &vcxproj, tprintf( "    <NMakeOutput>??? This is difficult cos it's so dependent on the build logic/NMakeOutput>", config ) );
			file_write_line( &vcxproj, tprintf( "    <NMakeBuildCommandLine>builder.exe %s %s</NMakeBuildCommandLine>", visualStudioOptions->build_script_path, config ) );
			
			// NOTE(TOM): "Standard" preprocessor defns. Ternaries aren't appropiate for configs that aren't debug or release but I wanted SOMETHING working
			const char* win32_definition = string_equals( platform, "Win32" ) ? "WIN32;"  : "";
			const char* debug_definition = string_equals( config,   "Debug" ) ? "_DEBUG;" : "NDEBUG;";

			// Write the preprocessor definitions
			file_write_line(
				&vcxproj,
				tprintf(
					"    <NMakePreprocessorDefinitions>%s%s$(NMakePreprocessorDefinitions)</NMakePreprocessorDefinitions>",
					win32_definition,    // WIN32 definition if the platform is Win32
					debug_definition     // _DEBUG for Debug, NDEBUG for Release
				)
			);

			file_write_line( &vcxproj, "  </PropertyGroup>" );
		}
	}

	file_write_line( &vcxproj,					"</Project>\n" );

	return true;
}