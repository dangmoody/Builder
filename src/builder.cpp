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

#ifdef _WIN64
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
	BUILDER_VERSION_MINOR	= 7,
	BUILDER_VERSION_PATCH	= 0,
};

enum doingBuildFrom_t {
	DOING_BUILD_FROM_SOURCE_FILE	= 0,
	DOING_BUILD_FROM_BUILD_INFO_FILE
};

#define SET_BUILDER_OPTIONS_FUNC_NAME		"set_builder_options"
#define PRE_BUILD_FUNC_NAME					"on_pre_build"
#define POST_BUILD_FUNC_NAME				"on_post_build"

#define QUIT_ERROR() \
	debug_break(); \
	return 1

struct trackedSourceFile_t {
	u64						lastWriteTime;
	std::string				filename;
};

struct builderVersion_t {
	s32								major;
	s32								minor;
	s32								patch;
};

struct buildInfoData_t {
	std::vector<BuildConfig>						configs;
	std::vector<std::vector<trackedSourceFile_t>>	trackedSourceFiles;
	std::vector<u64>								binaryLastWriteTimes;

	std::string										userConfigSourceFilename;
	std::string										userConfigDLLFilename;
	builderVersion_t								builderVersion;
};

errorCode_t GetLastErrorCode() {
#ifdef _WIN64
	return GetLastError();
#else
#error Unrecognised platform!
#endif
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

bool8 FileIsSourceFile( const char* filename ) {
	static const char* fileExtensions[] = {
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

bool8 FileIsHeaderFile( const char* filename ) {
	static const char* fileExtensions[] = {
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

static const char* BuildConfig_ToString( const BuildConfig* config ) {
	StringBuilder builder = {};
	string_builder_reset( &builder );

	string_builder_appendf( &builder, "%s: {\n", config->name.c_str() );

	auto PrintCStringArray = [&builder]( const char* name, const std::vector<const char*>& array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i] );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	};

	auto PrintSTDStringArray = [&builder]( const char* name, const std::vector<std::string>& array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For ( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i].c_str() );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
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

	PrintSTDStringArray( "source_files", config->source_files );
	PrintSTDStringArray( "defines", config->defines );
	PrintSTDStringArray( "additional_includes", config->additional_includes );
	PrintSTDStringArray( "additional_lib_paths", config->additional_lib_paths );
	PrintSTDStringArray( "additional_libs", config->additional_libs );
	PrintSTDStringArray( "ignore_warnings", config->ignore_warnings );

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

void BuildConfig_AddDefaults( BuildConfig* outConfig ) {
	// add the folder that builder lives in as an additional include path
	// so that people can just include builder.h without having to add the include path manually every time
	outConfig->additional_includes.push_back( path_app_path() );

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

#if BUILDER_CLANG_VERSION_MAJOR >= 17
	outConfig->ignore_warnings.push_back( "-Wno-unsafe-buffer-usage" );			// basically any time you access a ptr as an array, which is dumb
	outConfig->ignore_warnings.push_back( "-Wno-reorder-init-list" );			// C++: "designated initializers must be in order"
	outConfig->ignore_warnings.push_back( "-Wno-old-style-cast" );				// C++: "C-style casts are banned"
	outConfig->ignore_warnings.push_back( "-Wno-global-constructors" );			// C++: "declaration requires a global destructor"
	outConfig->ignore_warnings.push_back( "-Wno-exit-time-destructors" );		// C++: "declaration requires an exit-time destructor" (same as the above, basically)
#endif

#if BUILDER_CLANG_VERSION_MAJOR >= 18
	outConfig->ignore_warnings.push_back( "-Wno-missing-field-initializers" );
#endif

#if BUILDER_CLANG_VERSION_MAJOR >= 20
	outConfig->ignore_warnings.push_back( "-Wno-nontrivial-memcall" );			// basically any time you try to use ptr arithmetic
#endif
}

const char* BuildConfig_GetFullBinaryName( const BuildConfig* config ) {
	assert( !config->binary_name.empty() );

	StringBuilder builder = {};
	string_builder_reset( &builder );

	if ( !config->binary_folder.empty() ) {
		string_builder_appendf( &builder, "%s\\", config->binary_folder.c_str() );
	}

	string_builder_appendf( &builder, "%s", config->binary_name.c_str() );

	if ( !config->remove_file_extension ) {
		string_builder_appendf( &builder, ".%s", GetFileExtensionFromBinaryType( config->binary_type ) );
	}

	return string_builder_to_string( &builder );
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

	Process* process = process_create( args, environmentVariables, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	// show stdout
	if ( showStdout ) {
		char buffer[1024] = { 0 };
		u64 bytesRead = U64_MAX;

		while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
			buffer[bytesRead] = 0;

			printf( "%s", buffer );
		}
	}

	s32 exitCode = process_join( process );

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

static const char* GetDepFilename( const buildContext_t* context, const BuildConfig* config ) {
	assert( context );
	assert( config );

	const char* configNameNoPath = path_remove_path_from_file( config->name.c_str() );

	return tprintf( "%s\\%s.d", context->dotBuilderFolder.data, configNameNoPath );
}

static s32 BuildEXE( buildContext_t* context ) {
	Array<const char*> args;
	args.reserve(
		1 +	// clang
		3 + // -MD -MF <filename>
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
		context->config.warning_levels.size() +
		context->config.ignore_warnings.size()
	);

	args.add( tprintf( "%s\\clang\\bin\\clang.exe", path_app_path() ) );

	if ( !context->config.name.empty() ) {
		args.add( "-MD" );											// generate the dependency file
		args.add( "-MF" );											// set the name of the dependency file to...
		args.add( GetDepFilename( context, &context->config ) );	// ...this
	}

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		if ( string_ends_with( context->config.source_files[sourceFileIndex].c_str(), ".cpp" ) ) {
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

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		args.add( context->config.source_files[sourceFileIndex].c_str() );
	}

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		args.add( tprintf( "-D%s", context->config.defines[defineIndex].c_str() ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
		args.add( tprintf( "-L%s", context->config.additional_lib_paths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
		args.add( tprintf( "-l%s", context->config.additional_libs[libIndex].c_str() ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		args.add( "-Werror" );
	}

	// warning levels
	{
		std::vector<std::string> allowedWarningLevels = {
			"-Weverything",
			"-Wall",
			"-Wextra",
			"-Wpedantic",
		};

		For ( u64, warningLevelIndex, 0, context->config.warning_levels.size() ) {
			const std::string& warningLevel = context->config.warning_levels[warningLevelIndex];

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				if ( allowedWarningLevels[allowedWarningLevelIndex] == warningLevel ) {
					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "\"%s\" is not allowed as a warning level.\n", warningLevel.c_str() );
				return 1;
			}

			args.add( warningLevel.c_str() );
		}
	}

	For ( u64, ignoreWarningIndex, 0, context->config.ignore_warnings.size() ) {
		args.add( context->config.ignore_warnings[ignoreWarningIndex].c_str() );
	}

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
		3 + // -MD -MF <filename>
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
		context->config.warning_levels.size() +
		context->config.ignore_warnings.size()
	);

	args.add( tprintf( "%s\\clang\\bin\\clang.exe", path_app_path() ) );
	args.add( "-shared" );

	if ( !context->config.name.empty() ) {
		args.add( "-MD" );											// generate the dependency file
		args.add( "-MF" );											// set the name of the dependency file to...
		args.add( GetDepFilename( context, &context->config ) );	// ...this
	}

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		if ( string_ends_with( context->config.source_files[sourceFileIndex].c_str(), ".cpp" ) ) {
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

	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		args.add( context->config.source_files[sourceFileIndex].c_str() );
	}

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		args.add( tprintf( "-D%s", context->config.defines[defineIndex].c_str() ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
		args.add( tprintf( "-L%s", context->config.additional_lib_paths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
		args.add( tprintf( "-l%s", context->config.additional_libs[libIndex].c_str() ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		args.add( "-Werror" );
	}

	// warning levels
	{
		std::vector<std::string> allowedWarningLevels = {
			"-Weverything",
			"-Wall",
			"-Wextra",
			"-Wpedantic",
		};

		For ( u64, warningLevelIndex, 0, context->config.warning_levels.size() ) {
			const std::string& warningLevel = context->config.warning_levels[warningLevelIndex];

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				if ( allowedWarningLevels[allowedWarningLevelIndex] == warningLevel ) {
					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "\"%s\" is not allowed as a warning level.\n", warningLevel.c_str() );
				return 1;
			}

			args.add( warningLevel.c_str() );
		}
	}

	For ( u64, ignoreWarningIndex, 0, context->config.ignore_warnings.size() ) {
		args.add( context->config.ignore_warnings[ignoreWarningIndex].c_str() );
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
		3 + // -MD -MF <filename>
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
		context->config.warning_levels.size() +
		context->config.ignore_warnings.size()
	);

	// build .o files of all compilation units
	For ( u64, sourceFileIndex, 0, context->config.source_files.size() ) {
		const char* sourceFile = context->config.source_files[sourceFileIndex].c_str();

		args.reset();

		args.add( tprintf( "%s\\clang\\bin\\clang.exe", path_app_path() ) );

		if ( !context->config.name.empty() ) {
			args.add( "-MD" );											// generate the dependency file
			args.add( "-MF" );											// set the name of the dependency file to...
			args.add( GetDepFilename( context, &context->config ) );	// ...this
		}

		args.add( "-c" );

		if ( string_ends_with( sourceFile, ".cpp" ) ) { // TODO(DM): 04/01/2025: IsSourceFile( sourceFile )
			args.add( "-std=c++20" );
		} else if ( string_ends_with( sourceFile, ".c" ) ) {
			args.add( "-std=c99" );
		} else {
			assertf( false, "Something went really wrong.\n" );
			QUIT_ERROR();
		}

		if ( !context->config.remove_symbols ) {
			args.add( "-g" );
		}

		args.add( OptimizationLevelToString( context->config.optimization_level ) );

		{
			const char* sourceFileTrimmed = sourceFile;
			sourceFileTrimmed = strrchr( sourceFileTrimmed, '\\' ) + 1;

			const char* outArg = tprintf( "%s\\%s.o", context->config.binary_folder.c_str(), sourceFileTrimmed );

			args.add( "-o" );
			args.add( outArg );
			intermediateFiles.add( outArg );
		}

		args.add( sourceFile );

		For ( u32, defineIndex, 0, context->config.defines.size() ) {
			args.add( tprintf( "-D%s", context->config.defines[defineIndex].c_str() ) );
		}

		For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
			args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
		}

		// must come before ignored warnings
		if ( context->config.warnings_as_errors ) {
			args.add( "-Werror" );
		}

		// warning levels
		{
			std::vector<std::string> allowedWarningLevels = {
				"-Weverything",
				"-Wall",
				"-Wextra",
				"-Wpedantic",
			};

			For ( u64, warningLevelIndex, 0, context->config.warning_levels.size() ) {
				const std::string& warningLevel = context->config.warning_levels[warningLevelIndex];

				bool8 found = false;

				For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
					if ( allowedWarningLevels[allowedWarningLevelIndex] == warningLevel ) {
						found = true;
						break;
					}
				}

				if ( !found ) {
					error( "\"%s\" is not allowed as a warning level.\n", warningLevel.c_str() );
					return 1;
				}

				args.add( warningLevel.c_str() );
			}
		}

		For ( u64, ignoreWarningIndex, 0, context->config.ignore_warnings.size() ) {
			args.add( context->config.ignore_warnings[ignoreWarningIndex].c_str() );
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

static void NukeFolderInternal_r( const char* folder, const bool8 verbose ) {
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

			NukeFolderInternal_r( fileFullPath, verbose );

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

void NukeFolder_r( const char* folder, const bool8 deleteRoot, const bool8 verbose ) {
	NukeFolderInternal_r( folder, verbose );

	if ( deleteRoot ) {
		if ( !folder_delete( folder ) ) {
			errorCode_t errorCode = GetLastErrorCode();
			error( "Nuke failed to delete root folder \"%s\".  You'll have to do this manually.  Error code " ERROR_CODE_FORMAT "\n", folder, errorCode );
		}
	}
}

const char* GetSlashInPath( const char* path ) {
	const char* slash = NULL;

	if ( !slash ) slash = strchr( path, '/' );
	if ( !slash ) slash = strchr( path, '\\' );

	return slash;
}

bool8 PathHasSlash( const char* path ) {
	return GetSlashInPath( path ) != NULL;
}

static const char* TryFindFile_r( const char* filename, const char* folder ) {
	const char* result = NULL;

	const char* searchPath = tprintf( "%s\\*", folder );

	const char* filenamePath = path_remove_file_from_path( filename );

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

static void GetAllSourceFiles_r( const char* path, const char* subfolder, String searchFilter, std::vector<std::string>& outSourceFiles ) {
	unused( outSourceFiles );

	const char* start = searchFilter.data;
	const char* end = NULL;
	if ( !end ) end = strchr( start, '/' );
	if ( !end ) end = strchr( start, '\\' );
	if ( end ) {
		u64 subPathLength = cast( u64, end ) - cast( u64, start );
		String subPath;
		string_copy_from_c_string( &subPath, start, subPathLength );

		if ( string_equals( subPath.data, "**" ) ) {
			//printf( "Doing recursive file search\n" );

			// + 1 to skip the slash as well
			searchFilter.data += subPathLength + 1;
			searchFilter.count -= subPathLength + 1;

			//printf( "'path' is now: %s\n", newPath.data );
			//printf( "'searchFilter' is now: %s\n", searchFilter.data );
			//printf( "\n" );

			FileInfo fileInfo = {};
			File file = file_find_first( tprintf( "%s%s*", path, PATH_SEPARATOR ), &fileInfo );

			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( !fileInfo.is_directory ) {
					continue;
				}

				const char* newSubfolder = NULL;
				if ( subfolder ) {
					newSubfolder = tprintf( "%s\\%s", subfolder, fileInfo.filename );
				} else {
					newSubfolder = tprintf( "%s", fileInfo.filename );
				}

				GetAllSourceFiles_r( path, newSubfolder, searchFilter.data, outSourceFiles );
			} while ( file_find_next( &file, &fileInfo ) );
		} else if ( string_equals( subPath.data, "*" ) ) {
			//printf( "Doing non-recursive file search\n" );

			const char* fullSearchPath = NULL;
			if ( subfolder ) {
				fullSearchPath = tprintf( "%s%s%s%s%s", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR, subPath.data );
			} else {
				fullSearchPath = subPath.data;
			}

			FileInfo fileInfo = {};
			File file = file_find_first( fullSearchPath, &fileInfo );

			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( fileInfo.is_directory ) {
					continue;
				}

				const char* foundFilename = NULL;
				if ( subfolder ) {
					foundFilename = tprintf( "%s%s%s", subfolder, PATH_SEPARATOR, fileInfo.filename );
				} else {
					foundFilename = tprintf( "%s", fileInfo.filename );
				}

				//printf( "FOUND FILE \"%s\"\n", foundFilename );

				outSourceFiles.push_back( foundFilename );
			} while ( file_find_next( &file, &fileInfo ) );
		} else {
			// + 1 to skip the slash as well
			searchFilter.data += subPathLength + 1;
			searchFilter.count -= subPathLength + 1;

			//printf( "'path' is now: %s\n", newPath.data );
			//printf( "'searchFilter' is now: %s\n", searchFilter.data );
			//printf( "\n" );

			const char* newSubfolder = NULL;
			if ( subfolder ) {
				newSubfolder = tprintf( "%s%s%s%s", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR, searchFilter );
			} else {
				newSubfolder = tprintf( "%s%s", path, PATH_SEPARATOR, searchFilter );
			}

			GetAllSourceFiles_r( path, newSubfolder, searchFilter, outSourceFiles );
		}
	} else {
		const char* fullSearchPath = NULL;
		if ( subfolder ) {
			fullSearchPath = tprintf( "%s%s%s%s%s", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR, searchFilter.data );
		} else {
			fullSearchPath = tprintf( "%s%s%s", path, PATH_SEPARATOR, searchFilter.data );
		}

		FileInfo fileInfo = {};
		File file = file_find_first( fullSearchPath, &fileInfo );

		if ( file.ptr != INVALID_HANDLE_VALUE ) {
			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( fileInfo.is_directory ) {
					continue;
				}

				const char* foundFilename = NULL;
				if ( subfolder ) {
					foundFilename = tprintf( "%s%s%s", subfolder, PATH_SEPARATOR, fileInfo.filename );
				} else {
					foundFilename = tprintf( "%s", fileInfo.filename );
				}

				//printf( "FOUND FILE \"%s\"\n", foundFilename );

				outSourceFiles.push_back( foundFilename );
			} while ( file_find_next( &file, &fileInfo ) );
		}
	}
}

void GetAllSubfolders_r( const char* basePath, const char* folder, Array<const char*>* outSubfolders ) {
	const char* fullSearchPath = NULL;

	if ( string_ends_with( basePath, "\\" ) || string_ends_with( basePath, "/" ) ) {
		if ( folder ) {
			fullSearchPath = tprintf( "%s%s\\*", basePath, folder );
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
		if ( string_equals( fileInfo.filename, "." ) ) {
			continue;
		}

		if ( string_equals( fileInfo.filename, ".." ) ) {
			continue;
		}

		if ( fileInfo.is_directory ) {
			const char* folderName = tprintf( "%s", fileInfo.filename );

			//outSubfolders->add( folderName );

			const char* newPath = NULL;
			if ( folder ) {
				newPath = tprintf( "%s\\%s", folder, folderName );
			} else {
				newPath = folderName;
			}

			outSubfolders->add( newPath );

			GetAllSubfolders_r( basePath, newPath, outSubfolders );
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

static std::vector<std::string> BuildConfig_GetAllSourceFiles( const buildContext_t* context, const BuildConfig* config ) {
	std::vector<std::string> sourceFiles;

	For ( u64, sourceFileIndex, 0, config->source_files.size() ) {
		const char* sourceFile = config->source_files[sourceFileIndex].c_str();

		bool8 inputFileIsSameAsSourceFile = string_equals( sourceFile, context->inputFile );

		const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );

		if ( inputFileIsSameAsSourceFile ) {
			GetAllSourceFiles_r( context->inputFilePath.data, NULL, sourceFileNoPath, sourceFiles );
		} else {
			const char* sourceFilePath = path_remove_file_from_path( sourceFile );

			if ( !sourceFilePath ) {
				sourceFilePath = ".";
			}

			GetAllSourceFiles_r( context->inputFilePath.data, sourceFilePath, sourceFileNoPath, sourceFiles );
		}
	}

	return sourceFiles;
}

// only call this after compilation has finished successfully
// parse the dependency file that we generated for every dependency thats in there
// add those to a list - we need to put those in the .build_info file
static void ReadDependencyFile( const buildContext_t* context, const BuildConfig* config, std::vector<trackedSourceFile_t>& outBuildInfoFiles ) {
	// .d files only get created if a config name was specified
	// if a config name was not specified then no .d file was generated, so its not an error in that case
	if ( config->name.empty() ) {
		return;
	}

	const char* depFilename = GetDepFilename( context, config );

	char* depFileBuffer = NULL;
	bool8 read = file_read_entire( depFilename, &depFileBuffer );

	if ( !read ) {
		fatal_error( "Failed to read \"%s\".  This should never happen!\n", depFilename );
		return;
	}

	defer( file_free_buffer( &depFileBuffer ) );

	outBuildInfoFiles.clear();

	char* current = depFileBuffer;

	// .d files start with the name of the binary followed by a colon
	// so skip past that first
	current = strchr( depFileBuffer, ':' );
	assert( current );
	current += 1;	// skip past the colon
	current += 1;	// skip past the following whitespace

	// skip past the newline after
	current = strchr( current, '\n' );
	assert( current );
	current++;

	while ( *current ) {
		// get start of the filename
		char* dependencyStart = current;

		while ( *dependencyStart == ' ' ) {
			dependencyStart += 1;
		}

		// get end of the filename
		char* dependencyEnd = NULL;
		// filenames are separated by either new line or space
		if ( !dependencyEnd ) dependencyEnd = strchr( dependencyStart, ' ' );
		if ( !dependencyEnd ) dependencyEnd = strchr( dependencyStart, '\n' );
		assert( dependencyEnd );
		// paths can have spaces in them, but they are preceded by a single backslash (\)
		// so if we find a space but it has a single backslash just before it then keep searching for a space
		while ( dependencyEnd && ( *( dependencyEnd - 1 ) == '\\' ) ) {
			dependencyEnd = strchr( dependencyEnd + 1, ' ' );
		}

		if ( !dependencyEnd ) {
			break;
		}

		if ( *( dependencyEnd - 1 ) == '\r' ) {
			dependencyEnd -= 1;
		}

		u64 dependencyFilenameLength = cast( u64, dependencyEnd ) - cast( u64, dependencyStart );

		// get the substring we actually need
		std::string dependencyFilename( dependencyStart, dependencyFilenameLength );
		For ( u64, i, 0, dependencyFilename.size() ) {
			if ( dependencyFilename[i] == '\\' && dependencyFilename[i + 1] == ' ' ) {
				dependencyFilename.erase( i, 1 );
			}
		}

		// get the file timestamp
		FileInfo fileInfo;
		File foundFile = file_find_first( dependencyFilename.c_str(), &fileInfo );
		assert( foundFile.ptr != INVALID_HANDLE_VALUE );
		u64 lastWriteTime = fileInfo.last_write_time;

		if ( context->verbose ) {
			printf( "Parsing dependency %s, last write time = %llu\n", dependencyFilename.c_str(), lastWriteTime );
		}

		outBuildInfoFiles.push_back( { lastWriteTime, dependencyFilename.c_str() } );

		current = dependencyEnd + 1;

		while ( *current == '\\' ) {
			current++;
		}

		if ( *current == '\r' ) {
			current++;
		}

		if ( *current == '\n' ) {
			current++;
		}
	}
}

struct byteBuffer_t {
	Array<u8>	data;
	u64			readOffset;
};

static void ByteBuffer_SkipReadTo( byteBuffer_t* buffer, const char c ) {
	while ( buffer->data[buffer->readOffset] != c ) {
		buffer->readOffset++;
	}
}

static void ByteBuffer_SkipReadPast( byteBuffer_t* buffer, const char c ) {
	ByteBuffer_SkipReadTo( buffer, c );
	buffer->readOffset++;
}

static bool8 ByteBuffer_Read_Bool8( byteBuffer_t* buffer ) {
	bool8* result = cast( bool8*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( bool8 );

	return *result;
}

static void ByteBuffer_Write_Bool8( byteBuffer_t* buffer, const bool8 x ) {
	buffer->data.add( x );
}

static s32 ByteBuffer_Read_S32( byteBuffer_t* buffer ) {
	s32* result = cast( s32*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( s32 );

	return *result;
}

static void ByteBuffer_Write_S32( byteBuffer_t* buffer, const s32 x ) {
	buffer->data.reserve( buffer->data.alloced + sizeof( s32 ) );

	buffer->data.add( ( x ) & 0xFF );
	buffer->data.add( ( x >> 8 ) & 0xFF );
	buffer->data.add( ( x >> 16 ) & 0xFF );
	buffer->data.add( ( x >> 24 ) & 0xFF );
}

static u64 ByteBuffer_Read_U64( byteBuffer_t* buffer ) {
	u64* result = cast( u64*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( u64 );

	return *result;
}

static void ByteBuffer_Write_U64( byteBuffer_t* buffer, const u64 x ) {
	buffer->data.reserve( buffer->data.alloced + sizeof( u64 ) );

	buffer->data.add( ( x ) & 0xFF );
	buffer->data.add( ( x >> 8 ) & 0xFF );
	buffer->data.add( ( x >> 16 ) & 0xFF );
	buffer->data.add( ( x >> 24 ) & 0xFF );
	buffer->data.add( ( x >> 32 ) & 0xFF );
	buffer->data.add( ( x >> 40 ) & 0xFF );
	buffer->data.add( ( x >> 48 ) & 0xFF );
	buffer->data.add( ( x >> 56 ) & 0xFF );
}

static char* ByteBuffer_Read_CString( byteBuffer_t* buffer ) {
	u8* bufferPos = &buffer->data[buffer->readOffset];

	const char* lineEnd = cast( const char*, memchr( bufferPos, '\n', buffer->data.count - buffer->readOffset ) );
	u64 stringLength = cast( u64, lineEnd ) - cast( u64, bufferPos );

	char* string = cast( char*, mem_alloc( ( stringLength + 1 ) * sizeof( char ) ) );	// + 1 for null terminator
	memcpy( string, bufferPos, stringLength * sizeof( char ) );
	string[stringLength] = 0;

	ByteBuffer_SkipReadPast( buffer, '\n' );

	return string;
}

static void ByteBuffer_Write_CString( byteBuffer_t* buffer, const char* str ) {
	assert( str );

	u64 length = strlen( str );

	buffer->data.add_range( cast( const u8*, str ), length );
}

static void ByteBuffer_Read_StringField( byteBuffer_t* buffer, char** outKey, std::string* outValue ) {
	assertf( outKey || outValue, "When reading a string field, you must take either the key or the value otherwise you will take no data and what's the point of that?" );

	const char* colon = cast( const char*, memchr( &buffer->data[buffer->readOffset], ':', buffer->data.count - buffer->readOffset ) );
	u64 keyLength = cast( u64, colon ) - cast( u64, &buffer->data[buffer->readOffset] );

	if ( outKey ) {
		*outKey = cast( char*, mem_alloc( ( keyLength + 1 ) * sizeof( char ) ) );	// + 1 for null terminator
		memcpy( *outKey, &buffer->data[buffer->readOffset], keyLength * sizeof( char ) );
		*outKey[keyLength] = 0;
	}

	buffer->readOffset += keyLength;	// skip to the colon
	buffer->readOffset += 1;			// skip past the colon
	buffer->readOffset += 1;			// skip past following whitespace

	char* value = ByteBuffer_Read_CString( buffer );

	if ( outValue ) {
		*outValue = std::string( value );
	}
}

static void ByteBuffer_Write_StringField( byteBuffer_t* buffer, const char* key, const char* value ) {
	assert( key );
	assert( value );

	ByteBuffer_Write_CString( buffer, key );
	ByteBuffer_Write_CString( buffer, ": " );
	ByteBuffer_Write_CString( buffer, value );
	ByteBuffer_Write_CString( buffer, "\n" );
}

static std::vector<const char*> ByteBuffer_Read_CStringArray( byteBuffer_t* buffer ) {
	ByteBuffer_SkipReadPast( buffer, '\n' );	// skip the name of the array

	u64 arrayCount = ByteBuffer_Read_U64( buffer );
	std::vector<const char*> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = ByteBuffer_Read_CString( buffer );
	}

	return result;
}

static void ByteBuffer_Write_CStringArray( byteBuffer_t* buffer, const std::vector<const char*>& array, const char* name ) {
	ByteBuffer_Write_CString( buffer, name );
	ByteBuffer_Write_CString( buffer, "\n" );
	ByteBuffer_Write_U64( buffer, array.size() );

	For ( u64, i, 0, array.size() ) {
		ByteBuffer_Write_CString( buffer, array[i] );
		ByteBuffer_Write_CString( buffer, "\n" );
	}
}

static std::vector<std::string> ByteBuffer_Read_STDStringArray( byteBuffer_t* buffer ) {
	ByteBuffer_SkipReadPast( buffer, '\n' );	// skip the name of the array

	u64 arrayCount = ByteBuffer_Read_U64( buffer );
	std::vector<std::string> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = ByteBuffer_Read_CString( buffer );
	}

	return result;
}

static void ByteBuffer_Write_STDStringArray( byteBuffer_t* buffer, const std::vector<std::string>& array, const char* name ) {
	ByteBuffer_Write_CString( buffer, name );
	ByteBuffer_Write_CString( buffer, "\n" );
	ByteBuffer_Write_U64( buffer, array.size() );

	For ( u64, i, 0, array.size() ) {
		ByteBuffer_Write_CString( buffer, array[i].c_str() );
		ByteBuffer_Write_CString( buffer, "\n" );
	}
}

static bool8 BuildInfo_Read( const char* buildInfoFilename, buildInfoData_t* outBuildInfoData ) {
	byteBuffer_t buffer = {};

	// do this to avoid crash in ~Array() because it expects the allocator to not be NULL
	// we are not using the members of buffer::data (Array<T>) in a "standard" way here
	buffer.data.allocator = mem_get_current_allocator();

	bool8 read = file_read_entire( buildInfoFilename, cast( char**, &buffer.data.data ), &buffer.data.count );

	// the first time you build a source file with builder there wont be a .build_info file
	if ( !read ) {
		return false;
	}

	ByteBuffer_Read_CString( &buffer );	// "builder_version" tag, skip
	outBuildInfoData->builderVersion.major = ByteBuffer_Read_S32( &buffer );
	outBuildInfoData->builderVersion.minor = ByteBuffer_Read_S32( &buffer );
	outBuildInfoData->builderVersion.patch = ByteBuffer_Read_S32( &buffer );

	ByteBuffer_Read_StringField( &buffer, NULL, &outBuildInfoData->userConfigSourceFilename );
	ByteBuffer_Read_StringField( &buffer, NULL, &outBuildInfoData->userConfigDLLFilename );

	// parse all BuilderOptions
	{
		u64 totalNumConfigs = ByteBuffer_Read_U64( &buffer );

		outBuildInfoData->configs.resize( totalNumConfigs );
		outBuildInfoData->trackedSourceFiles.resize( totalNumConfigs );
		outBuildInfoData->binaryLastWriteTimes.resize( totalNumConfigs );

		For ( u64, configIndex, 0, outBuildInfoData->configs.size() ) {
			// parse the config itself
			{
				BuildConfig* config = &outBuildInfoData->configs[configIndex];

				ByteBuffer_Read_StringField( &buffer, NULL, &config->name );
				ByteBuffer_Read_U64( &buffer );	// name hash, skip

				{
					std::vector<const char*> dependencyNames = ByteBuffer_Read_CStringArray( &buffer );

					config->depends_on.resize( dependencyNames.size() );

					For ( u64, dependencyIndex, 0, dependencyNames.size() ) {
						config->depends_on[dependencyIndex].name = dependencyNames[dependencyIndex];
					}
				}

				config->source_files = ByteBuffer_Read_STDStringArray( &buffer );
				config->defines = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_includes = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_lib_paths = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_libs = ByteBuffer_Read_STDStringArray( &buffer );
				config->ignore_warnings = ByteBuffer_Read_STDStringArray( &buffer );

				ByteBuffer_Read_StringField( &buffer, NULL, &config->binary_name );
				ByteBuffer_Read_StringField( &buffer, NULL, &config->binary_folder );

				ByteBuffer_SkipReadPast( &buffer, '\n' );	// binary_last_write_time, skip
				outBuildInfoData->binaryLastWriteTimes[configIndex] = ByteBuffer_Read_U64( &buffer );

				config->binary_type = cast( BinaryType, ByteBuffer_Read_S32( &buffer ) );
				config->optimization_level = cast( OptimizationLevel, ByteBuffer_Read_S32( &buffer ) );
				config->remove_symbols = ByteBuffer_Read_Bool8( &buffer );
				config->remove_file_extension = ByteBuffer_Read_Bool8( &buffer );
				config->warnings_as_errors = ByteBuffer_Read_Bool8( &buffer );
			}

			// parse tracked source files
			{
				ByteBuffer_SkipReadPast( &buffer, '\n' );

				u64 count = ByteBuffer_Read_U64( &buffer );
				std::vector<trackedSourceFile_t>& trackedSourceFiles = outBuildInfoData->trackedSourceFiles[configIndex];
				trackedSourceFiles.reserve( count );

				For ( u64, fileIndex, 0, count ) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder-init-list"
					trackedSourceFile_t buildInfoFile = {};
					buildInfoFile.filename = ByteBuffer_Read_CString( &buffer );
					buildInfoFile.lastWriteTime = ByteBuffer_Read_U64( &buffer );
#pragma clang diagnostic pop

					trackedSourceFiles.push_back( buildInfoFile );
				}
			}

			ByteBuffer_SkipReadPast( &buffer, '\n' );
		}
	}

	// reconstruct all build dependencies after we have deserialized them all from the file
	For ( u64, configIndex, 0, outBuildInfoData->configs.size() ) {
		BuildConfig* config = &outBuildInfoData->configs[configIndex];

		For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
			BuildConfig* dependency = &config->depends_on[dependencyIndex];

			bool8 found = false;

			For ( u64, otherConfigIndex, 0, outBuildInfoData->configs.size() ) {
				BuildConfig* otherConfig = &outBuildInfoData->configs[otherConfigIndex];

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

static bool8 BuildInfo_Write( const buildContext_t* context, const buildInfoData_t* buildInfoData ) {
	assert( context );
	assert( buildInfoData );
	assert( !buildInfoData->configs.empty() );
	assert( !buildInfoData->userConfigSourceFilename.empty() );
	assert( !buildInfoData->userConfigDLLFilename.empty() );

	printf( "Writing \"%s\"...\n", context->buildInfoFilename.data );

	byteBuffer_t buffer = {};

	ByteBuffer_Write_CString( &buffer, "builder_version:\n" );
	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_MAJOR );
	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_MINOR );
	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_PATCH );

	ByteBuffer_Write_StringField( &buffer, "build_source_file", buildInfoData->userConfigSourceFilename.c_str() );
	ByteBuffer_Write_StringField( &buffer, "DLL", buildInfoData->userConfigDLLFilename.c_str() );

	ByteBuffer_Write_U64( &buffer, buildInfoData->configs.size() );

	For ( u64, configIndex, 0, buildInfoData->configs.size() ) {
		const BuildConfig* config = &buildInfoData->configs[configIndex];

		ByteBuffer_Write_StringField( &buffer, "config", config->name.c_str() );

		u64 configNameHash = hash_string( config->name.c_str(), 0 );
		ByteBuffer_Write_U64( &buffer, configNameHash );

		// serialize names of all build dependencies
		{
			ByteBuffer_Write_CString( &buffer, "depends_on\n" );
			ByteBuffer_Write_U64( &buffer, config->depends_on.size() );

			For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
				ByteBuffer_Write_CString( &buffer, config->depends_on[dependencyIndex].name.c_str() );
				ByteBuffer_Write_CString( &buffer, "\n" );
			}
		}

		ByteBuffer_Write_STDStringArray( &buffer, config->source_files, "source_files" );
		ByteBuffer_Write_STDStringArray( &buffer, config->defines, "defines" );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_includes, "additional_includes" );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_lib_paths, "additional_lib_paths" );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_libs, "additional_libs" );
		ByteBuffer_Write_STDStringArray( &buffer, config->ignore_warnings, "ignore_warnings" );

		ByteBuffer_Write_StringField( &buffer, "binary_name", config->binary_name.c_str() );
		ByteBuffer_Write_StringField( &buffer, "binary_folder", config->binary_folder.c_str() );

		// write the last write time of the binary
		// if the binary doesnt exist (we havent built it yet) then just write 0
		{
			ByteBuffer_Write_CString( &buffer, "last_binary_write_time\n" );

			const char* binaryFullName = tprintf( "%s\\%s", context->inputFilePath.data, BuildConfig_GetFullBinaryName( config ) );

			FileInfo fileInfo;
			File binaryFile = file_find_first( binaryFullName, &fileInfo );

			if ( binaryFile.ptr != INVALID_HANDLE_VALUE ) {
				ByteBuffer_Write_U64( &buffer, fileInfo.last_write_time );
			} else {
				ByteBuffer_Write_U64( &buffer, 0 );
			}
		}

		ByteBuffer_Write_S32( &buffer, config->binary_type );
		ByteBuffer_Write_S32( &buffer, config->optimization_level );
		ByteBuffer_Write_Bool8( &buffer, config->remove_symbols );
		ByteBuffer_Write_Bool8( &buffer, config->remove_file_extension );
		ByteBuffer_Write_Bool8( &buffer, config->warnings_as_errors );

		// serialize all included files and their last write time
		{
			const std::vector<trackedSourceFile_t>& trackedSourceFiles = buildInfoData->trackedSourceFiles[configIndex];

			ByteBuffer_Write_CString( &buffer, "tracked_source_files\n" );
			ByteBuffer_Write_U64( &buffer, trackedSourceFiles.size() );

			For ( u64, buildInfoFileIndex, 0, trackedSourceFiles.size() ) {
				const trackedSourceFile_t* trackedSourceFile = &trackedSourceFiles[buildInfoFileIndex];

				// dont write full path of the filename
				// the path to the source file can change depending on whether we are building from visual studio or the command line
				// so just write the filename relative to the source folder and let Builder reconstruct the appropriate path as and when it needs to
				ByteBuffer_Write_CString( &buffer, trackedSourceFile->filename.c_str() );
				ByteBuffer_Write_CString( &buffer, "\n" );
				ByteBuffer_Write_U64( &buffer, trackedSourceFile->lastWriteTime );
			}
		}

		ByteBuffer_Write_CString( &buffer, "\n" );

		mem_reset_temp_storage();
	}

	// write to the file
	bool8 written = file_write_entire( cast( char*, context->buildInfoFilename.data ), buffer.data.data, buffer.data.count );

	if ( !written ) {
		error( "Failed to write %s!\n", context->buildInfoFilename.data );
		return false;
	}

	printf( "\n" );

	return true;
}

static void AddBuildConfigAndDependenciesUnique( buildContext_t* context, BuildConfig* config, std::vector<BuildConfig>& outConfigs ) {
	u64 configNameHash = hash_string( config->name.c_str(), 0 );

	if ( hashmap_get_value( context->configIndices, configNameHash ) == HASHMAP_INVALID_VALUE ) {
		// add other configs that this config depends on first
		For ( size_t, dependencyIndex, 0, config->depends_on.size() ) {
			AddBuildConfigAndDependenciesUnique( context, &config->depends_on[dependencyIndex], outConfigs );
		}

		outConfigs.push_back( *config );

		hashmap_set_value( context->configIndices, configNameHash, trunc_cast( u32, outConfigs.size() - 1 ) );
	}
}

int main( int argc, char** argv ) {
	float64 totalTimeStart = time_ms();

	float64 userConfigBuildTimeMS = -1.0f;
	float64 visualStudioGenerationTimeMS = -1.0f;
	float64 buildInfoReadTimeMS = -1.0f;
	float64 buildInfoWriteTimeMS = -1.0f;

	core_init( MEM_MEGABYTES( 128 ) );	// TODO(DM): 26/03/2025: can we just use defaults for this now?
	defer( core_shutdown() );

	printf( "Builder v%d.%d.%d\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = {};
	context.configIndices = hashmap_create( 1 );	// TODO(DM): 30/03/2025: whats a reasonable default here?
	context.flags |= BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS | BUILD_CONTEXT_FLAG_SHOW_STDOUT;
#ifdef _DEBUG
	context.verbose = true;
#else
	context.verbose = false;
#endif

	// check if we need to perform first time setup
	{
		bool8 doFirstTimeSetup = false;

		// on exit set the CWD back to what we had before
		const char* oldCWD = path_current_working_directory();
		defer( SetCurrentDirectory( oldCWD ) );

		// set CWD to whereever builder lives for first time setup
		SetCurrentDirectory( path_app_path() );

		{
			const char* clangAbsolutePath = tprintf( "%s\\clang\\bin\\clang.exe", path_app_path() );

			// if we cant find our copy of clang then we definitely need to run first time setup
			if ( !FileExists( clangAbsolutePath ) ) {
				doFirstTimeSetup = true;
			} else {
				// otherwise if we have clang but the version doesnt match, then we still need to re-download and re-install it
				bool8 correctClangVersion = false;

				Array<const char*> args;
				args.add( clangAbsolutePath );
				args.add( "-v" );

				Process* clangVersionCheck = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );
				defer( process_destroy( clangVersionCheck ) );

				// this string is at the very start of "clang -v"
				const char* clangVersionString = tprintf( "clang version %d.%d.%d", BUILDER_CLANG_VERSION_MAJOR, BUILDER_CLANG_VERSION_MINOR, BUILDER_CLANG_VERSION_PATCH );

				char buffer[1024];
				while ( process_read_stdout( clangVersionCheck, buffer, 1024 ) ) {
					if ( string_contains( buffer, clangVersionString ) ) {
						correctClangVersion = true;
						break;
					}
				}

				s32 exitCode = process_join( clangVersionCheck );
				assertf( exitCode == 0, "Something went terribly wrong..\n" );

				if ( !correctClangVersion ) {
					warning( "Required Clang version not found.  I will need to re-download and re-install Clang.\n" );
					doFirstTimeSetup = true;
				}
			}
		}

		if ( doFirstTimeSetup ) {
			printf( "Performing first time setup...\n" );

			const char* clangInstallerFilename = tprintf( "LLVM-%d.%d.%d-win64.exe", BUILDER_CLANG_VERSION_MAJOR, BUILDER_CLANG_VERSION_MINOR, BUILDER_CLANG_VERSION_PATCH );

			if ( !folder_create_if_it_doesnt_exist( ".\\temp" ) ) {
				errorCode_t errorCode = GetLastErrorCode();
				error( "Failed to create the temp folder that the Clang install uses.  Is it possible you have whacky user permissions? Error code: " ERROR_CODE_FORMAT "\n", errorCode );
				QUIT_ERROR();
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
				args.add( tprintf( "https://github.com/llvm/llvm-project/releases/download/llvmorg-%d.%d.%d/%s", BUILDER_CLANG_VERSION_MAJOR, BUILDER_CLANG_VERSION_MINOR, BUILDER_CLANG_VERSION_PATCH, clangInstallerFilename ) );

				s32 exitCode = RunProc( &args, NULL, false, true );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to download Clang.  The CURL HTTP request failed.\n" );
					QUIT_ERROR();
				}
			}

			// install clang
			{
				printf( "Installing Clang (please wait) ... " );

				const char* clangInstallFolder = "clang";

				if ( !folder_create_if_it_doesnt_exist( clangInstallFolder ) ) {
					errorCode_t errorCode = GetLastErrorCode();
					error( "Failed to create the clang install folder \"%s\".  Is it possible you have some whacky user permissions? Error code: " ERROR_CODE_FORMAT "\n", clangInstallFolder, errorCode );
					QUIT_ERROR();
				}

				// set clang installer command line arguments
				// taken from: https://discourse.llvm.org/t/using-clang-windows-installer-from-command-line/49698/2 which references https://nsis.sourceforge.io/Docs/Chapter3.html#installerusagecommon
				Array<const char*> args;
				args.reserve( 4 );
				args.add( ".\\temp\\clang_installer.exe" );
				args.add( "/S" );		// install in silent mode
				args.add( tprintf( "/D=%s\\%s", path_app_path(), clangInstallFolder ) );	// set the install directory, absolute paths only

				Array<const char*> envVars;
				envVars.add( "__compat_layer=RunAsInvoker" );	// this tricks the subprocess into thinking we are running with elevation

				s32 exitCode = RunProc( &args, &envVars );

				if ( exitCode == 0 ) {
					printf( "Done.\n" );
				} else {
					error( "Failed to install Clang.\n" );
					QUIT_ERROR();
				}
			}

			// clean up temp files
			{
				printf( "Cleaning up ... " );

				NukeFolder_r( "temp", true, true );

				if ( folder_exists( "temp" ) ) {
					warning( "Failed to fully delete the temp folder after installing Clang.  You are safe to delete this yourself.\n" );
				}

				printf( "\n" );
			}

			doFirstTimeSetup = false;
		}

		mem_reset_temp_storage();
	}

	//const char* inputFile = NULL;

	const char* inputConfigName = NULL;
	u64 inputConfigNameHash = 0;

	doingBuildFrom_t doingBuildFrom = DOING_BUILD_FROM_SOURCE_FILE;

	for ( s32 argIndex = 1; argIndex < argc; argIndex++ ) {
		const char* arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( string_equals( arg, ARG_HELP_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( string_equals( arg, ARG_VERBOSE_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			context.verbose = true;
			continue;
		}

		if ( string_ends_with( arg, ".c" ) || string_ends_with( arg, ".cpp" ) ) {
			if ( context.inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}

			context.inputFile = arg;
			doingBuildFrom = DOING_BUILD_FROM_SOURCE_FILE;

			continue;
		}

		if ( string_ends_with( arg, BUILD_INFO_FILE_EXTENSION ) ) {
			if ( context.inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}

			context.inputFile = arg;
			doingBuildFrom = DOING_BUILD_FROM_BUILD_INFO_FILE;

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
				QUIT_ERROR();
			}

			const char* folderToNuke = argv[argIndex + 1];

			printf( "Nuking \"%s\"\n", folderToNuke );

			float64 startTime = time_ms();

			NukeFolder_r( folderToNuke, false, true );

			float64 endTime = time_ms();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		// unrecognised arg, show error
		error( "Unrecognised argument \"%s\".\n", arg );
		QUIT_ERROR();
	}

	// validate input file
	if ( context.inputFile == NULL ) {
		error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Run builder " ARG_HELP_LONG " if you need help.\n"
		);

		QUIT_ERROR();
	}

	u64 inputFileLength = strlen( context.inputFile );

	// the default binary folder is the same folder as the source file
	// if the file doesnt have a path then assume its in the same path as the current working directory (where we are calling builder from)
	{
		const char* inputFilePath = path_remove_file_from_path( context.inputFile );

		if ( !inputFilePath ) {
			inputFilePath = path_current_working_directory();
		}

		if ( doingBuildFrom == DOING_BUILD_FROM_BUILD_INFO_FILE ) {
			string_printf( &context.inputFilePath, "%s\\..", inputFilePath );

			context.dotBuilderFolder = inputFilePath;
			context.buildInfoFilename = context.inputFile;
		} else {
			const char* inputFileNoPath = path_remove_path_from_file( context.inputFile );
			const char* inputFileNoPathOrExtension = path_remove_file_extension( inputFileNoPath );

			context.inputFilePath = inputFilePath;

			string_printf( &context.dotBuilderFolder, "%s\\.builder", context.inputFilePath.data );
			string_printf( &context.buildInfoFilename, "%s\\%s%s", context.dotBuilderFolder.data, inputFileNoPathOrExtension, BUILD_INFO_FILE_EXTENSION );
		}
	}

	const char* defaultBinaryName = path_remove_file_extension( path_remove_path_from_file( context.inputFile ) );

	// read .build_info file
	buildInfoData_t buildInfoData = {};
	bool8 readBuildInfo = false;
	{
		float64 start = time_ms();

		readBuildInfo = BuildInfo_Read( cast( char*, context.buildInfoFilename.data ), &buildInfoData );

		float64 end = time_ms();

		buildInfoReadTimeMS = end - start;
	}

	if ( !readBuildInfo ) {
		if ( doingBuildFrom == DOING_BUILD_FROM_BUILD_INFO_FILE ) {
			error( "Can't find \"%s\".  Does this file exist?\n", context.buildInfoFilename.data );
			QUIT_ERROR();
		} else {
			buildInfoData.userConfigSourceFilename = context.inputFile;
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

	bool8 doUserConfigBuild = doingBuildFrom == DOING_BUILD_FROM_SOURCE_FILE;

	if ( doingBuildFrom == DOING_BUILD_FROM_BUILD_INFO_FILE ) {
		library = library_load( buildInfoData.userConfigDLLFilename.c_str() );

		if ( library.ptr == INVALID_HANDLE_VALUE ) {
			doUserConfigBuild = true;
		}
	}

	// build config step
	// see if they have set_builder_options() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	if ( doUserConfigBuild ) {
		float64 userConfigBuildTimeStart = time_ms();

		buildContext_t userConfigBuildContext = {};
		BuildConfig_AddDefaults( &userConfigBuildContext.config );
		userConfigBuildContext.flags = BUILD_CONTEXT_FLAG_SHOW_STDOUT;

		if ( context.verbose ) {
			userConfigBuildContext.flags |= BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS;
		}

		userConfigBuildContext.config.source_files.push_back( buildInfoData.userConfigSourceFilename );

		userConfigBuildContext.config.binary_name = tprintf( "%s.dll", path_remove_path_from_file( path_remove_file_extension( buildInfoData.userConfigSourceFilename.c_str() ) ) );
		userConfigBuildContext.config.binary_folder = cast( char*, context.dotBuilderFolder.data );
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

		buildInfoData.userConfigDLLFilename = userConfigBuildContext.fullBinaryName;

		if ( !folder_create_if_it_doesnt_exist( userConfigBuildContext.config.binary_folder.c_str() ) ) {
			errorCode_t errorCode = GetLastErrorCode();
			error( "Failed to create the .builder folder.  Error code " ERROR_CODE_FORMAT "\n", userConfigBuildContext.config.binary_folder.c_str(), errorCode );
			QUIT_ERROR();
		}

		exitCode = BuildDynamicLibrary( &userConfigBuildContext );

		if ( exitCode != 0 ) {
			error( "Pre-build failed!\n" );
			QUIT_ERROR();
		}

		float64 userConfigBuildTimeEnd = time_ms();

		userConfigBuildTimeMS = userConfigBuildTimeEnd - userConfigBuildTimeStart;
	}

	printf( "\n" );

	BuilderOptions options = {};

	{
		if ( library.ptr == INVALID_HANDLE_VALUE || library.ptr == NULL ) {
			library = library_load( buildInfoData.userConfigDLLFilename.c_str() );
			assert( library.ptr != INVALID_HANDLE_VALUE );
		}

		preBuildFunc = cast( preBuildFunc_t, library_get_proc_address( library, PRE_BUILD_FUNC_NAME ) );
		postBuildFunc = cast( postBuildFunc_t, library_get_proc_address( library, POST_BUILD_FUNC_NAME ) );

		if ( doingBuildFrom == DOING_BUILD_FROM_SOURCE_FILE ) {
			// now get the user-specified options
			setBuilderOptionsFunc_t setBuilderOptionsFunc = cast( setBuilderOptionsFunc_t, library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME ) );
			if ( setBuilderOptionsFunc ) {
				setBuilderOptionsFunc( &options );

				buildInfoData.configs = options.configs;

				// if the user wants to generate a visual studio solution then do that now
				if ( options.generate_solution ) {
					// you either want to generate a visual studio solution or build this config, but not both
					if ( inputConfigName ) {
						error(
							"I see you want to generate a Visual Studio Solution, but you've also specified a config that you want to build.\n"
							"You must do one or the other, you can't do both.\n\n"
						);
						QUIT_ERROR();
					}

					context.flags |= BUILD_CONTEXT_FLAG_GENERATING_VISUAL_STUDIO_SOLUTION;

					// make sure BuilderOptions::configs and configs from visual studio match
					// we will need this list later for validation
					options.configs.clear();
					For ( u64, projectIndex, 0, options.solution.projects.size() ) {
						VisualStudioProject* project = &options.solution.projects[projectIndex];

						For ( u64, configIndex, 0, project->configs.size() ) {
							VisualStudioConfig* config = &project->configs[configIndex];

							BuildConfig_AddDefaults( &config->options );

							AddBuildConfigAndDependenciesUnique( &context, &config->options, options.configs );
						}
					}

					buildInfoData.binaryLastWriteTimes.resize( options.configs.size() );
					buildInfoData.trackedSourceFiles.resize( options.configs.size() );

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
				}
			}
		}
	}

	bool8 shouldWriteBuildInfo = true;

	if ( ( context.flags & BUILD_CONTEXT_FLAG_GENERATING_VISUAL_STUDIO_SOLUTION ) == 0 ) {
		std::vector<BuildConfig> configsToBuild;

		// if no configs were manually added then assume we are just doing a default build with no user-specified options
		if ( buildInfoData.configs.size() == 0 ) {
			BuildConfig config = {
				.source_files = { context.inputFile },
				.binary_name = defaultBinaryName
			};

			buildInfoData.configs.push_back( config );
		}

		buildInfoData.binaryLastWriteTimes.resize( buildInfoData.configs.size() );
		buildInfoData.trackedSourceFiles.resize( buildInfoData.configs.size() );

		// if only one config was added (either by user or as a default build) then we know we just want that one, no config command line arg is needed
		if ( buildInfoData.configs.size() == 1 ) {
			AddBuildConfigAndDependenciesUnique( &context, &buildInfoData.configs[0], configsToBuild );
		} else {
			if ( !inputConfigName ) {
				error(
					"This build has multiple configs, but you never specified a config name.\n"
					"You must pass in a config name via " ARG_CONFIG "\n"
					"Run builder " ARG_HELP_LONG " if you need help.\n"
				);
				QUIT_ERROR();
			}

			For ( size_t, configIndex, 0, buildInfoData.configs.size() ) {
				if ( buildInfoData.configs[configIndex].name.empty() ) {
					error(
						"You have multiple BuildConfigs in your build source file, but some of them have empty names.\n"
						"When you have multiple BuildConfigs, ALL of them MUST have non-empty names.\n"
						"You need to set 'BuildConfig::name' in every BuildConfig that you add via add_build_config() (including dependencies!).\n"
					);

					QUIT_ERROR();
				}
			}
		}

		// none of the configs can have the same name
		// TODO(DM): 14/11/2024: can we do better than o(n^2) here?
		For ( size_t, configIndexA, 0, buildInfoData.configs.size() ) {
			const char* configNameA = buildInfoData.configs[configIndexA].name.c_str();
			u64 configNameHashA = hash_string( configNameA, 0 );

			For ( size_t, configIndexB, 0, buildInfoData.configs.size() ) {
				if ( configIndexA == configIndexB ) {
					continue;
				}

				const char* configNameB = buildInfoData.configs[configIndexB].name.c_str();
				u64 configNameHashB = hash_string( configNameB, 0 );

				if ( configNameHashA == configNameHashB ) {
					error( "I found multiple configs with the name \"%s\".  All config names MUST be unique, otherwise I don't know which specific config you want me to build.\n", configNameA );
					QUIT_ERROR();
				}
			}
		}

		// of all the configs that the user filled out inside set_builder_options
		// find the one the user asked for in the command line
		if ( inputConfigName ) {
			bool8 foundConfig = false;
			For ( u64, configIndex, 0, buildInfoData.configs.size() ) {
				BuildConfig* config = &buildInfoData.configs[configIndex];

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

			const char* oldCWD = path_current_working_directory();
			SetCurrentDirectory( cast( char*, context.inputFilePath.data ) );
			defer( SetCurrentDirectory( oldCWD ) );

			preBuildFunc();
		}

		u32 numSuccessfulBuilds = 0;
		u32 numFailedBuilds = 0;
		u32 numSkippedBuilds = 0;

		For ( u64, configToBuildIndex, 0, configsToBuild.size() ) {
			BuildConfig* config = &configsToBuild[configToBuildIndex];

			u64 configNameHash = hash_string( config->name.c_str(), 0 );
			u32 actualConfigIndex = hashmap_get_value( context.configIndices, configNameHash );

			assert( !config->binary_name.empty() );

			BuildConfig_AddDefaults( config );

			// make sure that the binary folder and binary name are at least set to defaults
			if ( !config->binary_folder.empty() ) {
				config->binary_folder = tprintf( "%s\\%s", context.inputFilePath.data, config->binary_folder.c_str() );
			} else {
				config->binary_folder = cast( char*, context.inputFilePath.data );
			}

			context.config = *config;
			context.fullBinaryName = BuildConfig_GetFullBinaryName( &context.config );

			bool8 shouldSkipBuild = true;

			// figure out if we need to even rebuild
			auto ShouldRebuild = [actualConfigIndex, &buildInfoData, readBuildInfo]( const buildContext_t* buildContext ) -> bool8 {
				// if the .build_info isnt there, or we expect a different name now, or something else
				// then we wont have any tracked source files to check through later on in this subroutine
				// and the build will be skipped
				// so just force a rebuild if we cant find/read the .build_info for whatever reason
				if ( !readBuildInfo ) {
					printf( "Failed to read %s.  Rebuilding...\n", buildContext->buildInfoFilename.data );
					return true;
				}

				// if this was last built on a different version of builder then rebuild
				if ( buildInfoData.builderVersion.major != BUILDER_VERSION_MAJOR ||
					 buildInfoData.builderVersion.minor != BUILDER_VERSION_MINOR ||
					 buildInfoData.builderVersion.patch != BUILDER_VERSION_PATCH )
				{
					printf( "Different Builder version detected since last build.  Rebuilding...\n" );
					return true;
				}

				{
					// if the binary doesnt exist, we definitely need to rebuild
					FileInfo binaryFileInfo;
					File binaryFile = file_find_first( buildContext->fullBinaryName, &binaryFileInfo );

					if ( binaryFile.ptr == INVALID_HANDLE_VALUE ) {
						return true;
					}

					// if the binary exists but the last time it was written to doesnt match whats in the .build_info then we know we need to rebuild
					if ( buildInfoData.binaryLastWriteTimes[actualConfigIndex] != binaryFileInfo.last_write_time ) {
						return true;
					}
				}

				// for every code file that the .build_info knows about, find that file on disk and get the time it was written to
				// if that write time doesnt match what we have in the .build_info for that file then its changed and we need to rebuild
				const std::vector<trackedSourceFile_t>& trackedSourceFiles = buildInfoData.trackedSourceFiles[actualConfigIndex];

				For ( u64, sourceFileIndex, 0, trackedSourceFiles.size() ) {
					const trackedSourceFile_t* trackedSourceFile = &trackedSourceFiles[sourceFileIndex];

					const char* trackedSourceFileAndPath = trackedSourceFile->filename.c_str();
					if ( !path_is_absolute( trackedSourceFile->filename.c_str() ) ) {
						trackedSourceFileAndPath = tprintf( "%s\\%s", buildContext->inputFilePath.data, trackedSourceFile->filename.c_str() );
					}

					FileInfo fileInfo = {};
					File file = file_find_first( trackedSourceFileAndPath, &fileInfo );

					bool8 cantFindFile = file.ptr != INVALID_HANDLE_VALUE;
					bool8 fileWasOverwritten = fileInfo.last_write_time != trackedSourceFile->lastWriteTime;

					if ( cantFindFile || fileWasOverwritten ) {
						return true;
					}
				}

				return false;
			};

			if ( !ShouldRebuild( &context ) ) {
				numSkippedBuilds++;

				printf( "Skipped \"%s\".\n", context.config.binary_name.c_str() );
				continue;
			} else {
				printf( "Building \"%s\"", context.config.binary_name.c_str() );

				if ( !context.config.name.empty() ) {
					printf( ", config \"%s\"", context.config.name.c_str() );
				}

				printf( "\n" );
			}

			if ( !folder_create_if_it_doesnt_exist( context.config.binary_folder.c_str() ) ) {
				errorCode_t errorCode = GetLastErrorCode();
				fatal_error( "Failed to create the binary folder you specified inside %s: \"%s\".  Error code: " ERROR_CODE_FORMAT "\n", SET_BUILDER_OPTIONS_FUNC_NAME, context.config.binary_folder.c_str(), errorCode );
				return 1;
			}

			// make all non-absolute additional include paths relative to the build source file
			For ( u64, includeIndex, 0, context.config.additional_includes.size() ) {
				const char* additionalInclude = context.config.additional_includes[includeIndex].c_str();

				if ( path_is_absolute( additionalInclude ) ) {
					context.config.additional_includes[includeIndex] = additionalInclude;
				} else {
					context.config.additional_includes[includeIndex] = tprintf( "%s\\%s", context.inputFilePath.data, additionalInclude );
				}
			}

			// make all non-absolute additional library paths relative to the build source file
			For ( u64, libPathIndex, 0, context.config.additional_lib_paths.size() ) {
				const char* additionalLibPath = context.config.additional_lib_paths[libPathIndex].c_str();

				if ( path_is_absolute( additionalLibPath ) ) {
					context.config.additional_lib_paths[libPathIndex] = additionalLibPath;
				} else {
					context.config.additional_lib_paths[libPathIndex] = tprintf( "%s\\%s", context.inputFilePath.data, additionalLibPath );
				}
			}

			// get all the "compilation units" that we are actually going to give to the compiler
			// if no source files were added in set_builder_options() then assume they only want to build the same file as the one specified via the command line
			if ( context.config.source_files.size() == 0 ) {
				context.config.source_files.push_back( context.inputFile );
			} else {
				// otherwise the user told us to build other source files, so go find and build those instead
				// keep this as a std::vector because this gets fed back into BuilderOptions::source_files
				std::vector<std::string> finalSourceFilesToBuild = BuildConfig_GetAllSourceFiles( &context, &context.config );

				// at this point its totally acceptable for finalSourceFilesToBuild to be empty
				// this is because the compiler should be the one that tells the user they specified no valid source files to build with
				// the compiler can and will throw an error for that, so let it

				// the .build_info file wont store the full paths to the source files because the input path can change depending on whether we're building via visual studio or from the command line
				// so we need to reconstruct the full paths to the source files ourselves
				For ( u64, sourceFileIndex, 0, finalSourceFilesToBuild.size() ) {
					finalSourceFilesToBuild[sourceFileIndex] = tprintf( "%s\\%s", context.inputFilePath.data, finalSourceFilesToBuild[sourceFileIndex].c_str() );
				}

				context.config.source_files = finalSourceFilesToBuild;
			}

			float64 buildTimeStart = 0.0f;
			float64 buildTimeEnd = 0.0f;

			// now do the actual build
			switch ( config->binary_type ) {
				case BINARY_TYPE_EXE:
					buildTimeStart = time_ms();

					exitCode = BuildEXE( &context );

					buildTimeEnd = time_ms();
					break;

				case BINARY_TYPE_DYNAMIC_LIBRARY:
					buildTimeStart = time_ms();

					exitCode = BuildDynamicLibrary( &context );

					buildTimeEnd = time_ms();
					break;

				case BINARY_TYPE_STATIC_LIBRARY:
					buildTimeStart = time_ms();

					exitCode = BuildStaticLibrary( &context );

					buildTimeEnd = time_ms();
					break;
			}

			if ( exitCode == 0 ) {
				numSuccessfulBuilds++;

				float64 depFileReadStart = time_ms();

				// get the updated list of dependency files from the .d file and put those into the .build_info file data
				ReadDependencyFile( &context, config, buildInfoData.trackedSourceFiles[actualConfigIndex] );

				float64 depFileReadEnd = time_ms();

				printf( "Finished building \"%s\":\n", context.fullBinaryName );
				printf( "    Read .d file: %f ms\n", depFileReadEnd - depFileReadStart );
				printf( "    Build time:   %f ms\n", buildTimeEnd - buildTimeStart );
				printf( "\n" );
			} else {
				numFailedBuilds++;

				error( "Build failed.\n" );
				QUIT_ERROR();
			}

			mem_reset_temp_storage();
		}

		if ( postBuildFunc ) {
			printf( "Running post-build code...\n" );

			const char* oldCWD = path_current_working_directory();
			SetCurrentDirectory( cast( char*, context.inputFilePath.data ) );
			defer( SetCurrentDirectory( oldCWD ) );

			postBuildFunc();
		}

		// only dont want to write the .build_info if all the builds skipped or if a build failed
		shouldWriteBuildInfo = ( numSuccessfulBuilds > 0 ) && ( numFailedBuilds == 0 );
	}

	if ( shouldWriteBuildInfo ) {
		float64 start = time_ms();

		if ( !BuildInfo_Write( &context, &buildInfoData ) ) {
			QUIT_ERROR();
		}

		float64 end = time_ms();
		buildInfoWriteTimeMS = end - start;
	}

	float64 totalTimeEnd = time_ms();
	printf( "Build finished:\n" );
	if ( doUserConfigBuild ) {
		printf( "    User config build: %f ms\n", userConfigBuildTimeMS );
	}
	if ( options.generate_solution ) {
		printf( "    Generate solution: %f ms\n", visualStudioGenerationTimeMS );
	}
	if ( readBuildInfo ) {
		printf( "    Read .build_info:  %f ms\n", buildInfoReadTimeMS );
	}
	if ( shouldWriteBuildInfo ) {
		printf( "    Write .build_info: %f ms\n", buildInfoWriteTimeMS );
	}
	printf( "    Total time:        %f ms\n", totalTimeEnd - totalTimeStart );
	printf( "\n" );

	return 0;
}