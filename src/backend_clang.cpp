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

#include "core/include/debug.h"
#include "core/include/string_helpers.h"
#include "core/include/paths.h"
#include "core/include/array.inl"
#include "core/include/file.h"
#include "core/include/core_process.h"
#include "core/include/string_builder.h"

struct clangState_t {
	Array<const char*>			args;

	std::vector<std::string>	includeDependencies;

	// TODO(DM): 11/02/2026: remove these when eds command archetype changes get merged in
	String						compilerPath;
	String						compilerVersion;
	String						linkerPath;
	String						arPath;	// static library linker for gcc (on windows and linux) and clang (linux)
};

// TODO(DM): 20/07/2025: do we want to ignore this warning via the build script?
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char* LanguageVersionToCompilerArg( const LanguageVersion languageVersion ) {
	assert( languageVersion != LANGUAGE_VERSION_UNSET );

	switch ( languageVersion ) {
		case LANGUAGE_VERSION_C89:		return "-std=c89";
		case LANGUAGE_VERSION_C99:		return "-std=c99";
		case LANGUAGE_VERSION_C11:		return "-std=c11";
		case LANGUAGE_VERSION_C17:		return "-std=c17";
		case LANGUAGE_VERSION_C23:		return "-std=c23";
		case LANGUAGE_VERSION_CPP11:	return "-std=c++11";
		case LANGUAGE_VERSION_CPP14:	return "-std=c++14";
		case LANGUAGE_VERSION_CPP17:	return "-std=c++17";
		case LANGUAGE_VERSION_CPP20:	return "-std=c++20";
		case LANGUAGE_VERSION_CPP23:	return "-std=c++23";
	}

	return NULL;
}

#pragma clang diagnostic pop

static const char* OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "-O0";
		case OPTIMIZATION_LEVEL_O1:	return "-O1";
		case OPTIMIZATION_LEVEL_O2:	return "-O2";
		case OPTIMIZATION_LEVEL_O3:	return "-O3";
	}
}

static void ReadDependencyFile( const char* depFilename, std::vector<std::string>& outIncludeDependencies ) {
	char* depFileBuffer = NULL;

	if ( !file_read_entire( depFilename, &depFileBuffer ) ) {
		errorCode_t errorCode = get_last_error_code();
		fatal_error( "Failed to read \"%s\".  This should never happen! Error code: " ERROR_CODE_FORMAT "\n", depFilename, errorCode );
		return;
	}

	defer( file_free_buffer( &depFileBuffer ) );

	outIncludeDependencies.clear();

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
	current += 1;

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
		//u64 lastWriteTime = GetLastFileWriteTime( dependencyFilename.c_str() );
		//printf( "Parsing dependency %s, last write time = %llu\n", dependencyFilename.c_str(), lastWriteTime );

		outIncludeDependencies.push_back( dependencyFilename.c_str() );

		current = dependencyEnd + 1;

		//while ( *current == PATH_SEPARATOR ) {
		while ( *current == '\\' ) {
			current += 1;
		}

		if ( *current == '\r' ) {
			current += 1;
		}

		if ( *current == '\n' ) {
			current += 1;
		}
	}
}

static void ResolveCompilerAndLinkerPaths( clangState_t *clangState, const char *compilerPath, const char *compilerName, const char *linkerName ) {
	const char *pathToCompiler = path_remove_file_from_path( compilerPath );

	if ( pathToCompiler == NULL ) {
		clangState->compilerPath = compilerPath;
		clangState->linkerPath = linkerName;
	} else {
		clangState->compilerPath = tprintf( "%s%c%s", pathToCompiler, PATH_SEPARATOR, compilerName );
		clangState->linkerPath = tprintf( "%s%c%s", pathToCompiler, PATH_SEPARATOR, linkerName );
	}
}

//================================================================

static bool8 Clang_Init( compilerBackend_t* backend, const std::string &compilerPath, const std::string &compilerVersion ) {
	backend->data = cast( clangState_t*, mem_alloc( sizeof( clangState_t ) ) );
	new( backend->data ) clangState_t;

	clangState_t* clangState = cast( clangState_t*, backend->data );

	clangState->compilerVersion = compilerVersion.c_str();

	const char *clangExe = "clang";
#if defined( _WIN32 )
	const char *linkerExe = "lld-link";
#elif defined( __linux__ )
	const char *linkerExe = "llvm-ar";
#else
	#error Unrecognised platform.
#endif

	ResolveCompilerAndLinkerPaths( clangState, compilerPath.c_str(), clangExe, linkerExe );

	const char *pathToCompiler = path_remove_file_from_path( compilerPath.c_str() );
	string_printf( &clangState->arPath, "%s%car", pathToCompiler, PATH_SEPARATOR );

	return true;
}

static bool8 GCC_Init( compilerBackend_t *backend, const std::string &compilerPath, const std::string &compilerVersion ) {
	backend->data = cast( clangState_t *, mem_alloc( sizeof( clangState_t ) ) );
	new( backend->data ) clangState_t;

	clangState_t *clangState = cast( clangState_t *, backend->data );

	clangState->compilerVersion = compilerVersion.c_str();

	ResolveCompilerAndLinkerPaths( clangState, compilerPath.c_str(), "gcc", "ld" );

	const char *pathToCompiler = path_remove_file_from_path( compilerPath.c_str() );
	string_printf( &clangState->arPath, "%s%car", pathToCompiler, PATH_SEPARATOR );

	return true;
}

static void Clang_Shutdown( compilerBackend_t* backend ) {
	mem_free( backend->data );
	backend->data = NULL;
}

static bool8 Clang_CompileSourceFile( compilerBackend_t* backend, const char* sourceFile, BuildConfig* config ) {
	assert( backend );
	assert( sourceFile );

	clangState_t* clangState = cast( clangState_t*, backend->data );

	bool8 isClang = string_ends_with( clangState->compilerPath.data, "clang" ) || string_ends_with( clangState->compilerPath.data, "clang++" );
	bool8 isGCC = string_ends_with( clangState->compilerPath.data, "gcc" ) || string_ends_with( clangState->compilerPath.data, "g++" );

	const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );

	const char* intermediatePath = tprintf( "%s%c%s", config->binary_folder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );
	const char* intermediateFilename = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

	const char* depFilename = tprintf( "%s%c%s.d", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

	Array<const char*>& args = clangState->args;
	args.reserve(
		1 +	// clang/gcc
		1 +	// -shared/-lib
		1 +	// -c
		1 +	// std=
		1 +	// symbols
		1 +	// optimisation
		1 +	// -o
		1 +	// intermediate filename
		3 +	// -MD -MF <filename>
		1 +	// source file
		config->defines.size() +
		config->additional_includes.size() +
		config->additional_lib_paths.size() +
		config->additional_libs.size() +
		config->warning_levels.size() +
		config->ignore_warnings.size()
	);

	args.reset();

	args.add( clangState->compilerPath.data );

	args.add( "-c" );

	if ( config->language_version != LANGUAGE_VERSION_UNSET ) {
		args.add( LanguageVersionToCompilerArg( config->language_version ) );
	}

	if ( !config->remove_symbols ) {
		args.add( "-g" );
	}

	args.add( OptimizationLevelToCompilerArg( config->optimization_level ) );

	args.add( "-o" );
	args.add( intermediateFilename );

	args.add( "-MMD" );			// generate the dependency file
	args.add( "-MF" );			// set the name of the dependency file to...
	args.add( depFilename );	// ...this

	args.add( sourceFile );

	For ( u32, defineIndex, 0, config->defines.size() ) {
		args.add( tprintf( "-D%s", config->defines[defineIndex].c_str() ) );
	}

	For ( u32, includeIndex, 0, config->additional_includes.size() ) {
		args.add( tprintf( "-I%s", config->additional_includes[includeIndex].c_str() ) );
	}

	// must come before ignored warnings
	if ( config->warnings_as_errors ) {
		args.add( "-Werror" );
	}

	// warning levels
	{
		std::vector<std::string> allowedWarningLevels = {
			"-Wall",
			"-Wextra",
			"-Wpedantic",
		};

		// gcc doesnt have this as a warning level but clang does
		if ( isClang ) {
			allowedWarningLevels.push_back( "-Weverything" );
		}

		For ( u64, warningLevelIndex, 0, config->warning_levels.size() ) {
			const std::string& warningLevel = config->warning_levels[warningLevelIndex];

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				if ( allowedWarningLevels[allowedWarningLevelIndex] == warningLevel ) {	// TODO(DM): 14/06/2025: better to compare hashes here instead?
					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "\"%s\" is not allowed as a warning level.\n", warningLevel.c_str() );
				return false;
			}

			args.add( warningLevel.c_str() );
		}
	}

	For ( u64, ignoreWarningIndex, 0, config->ignore_warnings.size() ) {
		args.add( config->ignore_warnings[ignoreWarningIndex].c_str() );
	}

	For ( u64, additionalArgumentIndex, 0, config->additional_compiler_arguments.size() ) {
		args.add( config->additional_compiler_arguments[additionalArgumentIndex].c_str() );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	if ( exitCode == 0 ) {
		ReadDependencyFile( depFilename, clangState->includeDependencies );
	}

	return exitCode == 0;
}

static bool8 Clang_LinkIntermediateFiles( compilerBackend_t* backend, const Array<const char*>& intermediateFiles, BuildConfig* config ) {
	assert( backend );
	assert( config );

	clangState_t *clangState = cast( clangState_t *, backend->data );

	// TODO(DM): 11/02/2026: remove this when eds command archetype changes get merged in
	bool8 isClang = string_ends_with( clangState->compilerPath.data, "clang" ) || string_ends_with( clangState->compilerPath.data, "clang++" );
	bool8 isGCC = string_ends_with( clangState->compilerPath.data, "gcc" ) || string_ends_with( clangState->compilerPath.data, "g++" );

	const char* fullBinaryName = BuildConfig_GetFullBinaryName( config );

	Array<const char*>& args = clangState->args;
	args.reserve(
		1 + // lld-link
		1 + // /lib or -shared
		1 + // -g
		1 + // -o
		1 + // binary name
		intermediateFiles.count +
		config->additional_lib_paths.size() +
		config->additional_libs.size()
	);

	args.reset();

	// clang and gcc treat static libraries as just an archive of .o files
	// so there is no real "link" step in this case, the .o files are just "archived" together
	// for dynamic libraries and executables clang and gcc recommend you call the compiler again and just pass in all the intermediate files
	if ( config->binary_type == BINARY_TYPE_STATIC_LIBRARY ) {
#if defined( _WIN32 )
		// TODO(DM): 11/02/2026: remove this when eds command archetype changes get merged in
		if ( isGCC ) {
			args.add( clangState->arPath.data );
			args.add( "rc" );
			args.add( fullBinaryName );
		} else {
			args.add( clangState->linkerPath.data );
			args.add( "/lib" );
			args.add( tprintf( "/OUT:%s", fullBinaryName ) );
		}
#elif defined( __linux__ )
		args.add( clangState->arPath );
		args.add( "rc" );
		args.add( fullBinaryName );
#endif

		args.add_range( &intermediateFiles );
	} else {
		args.add( clangState->compilerPath.data );

		if ( config->binary_type == BINARY_TYPE_DYNAMIC_LIBRARY ) {
			args.add( "-shared" );
		}

		if ( !config->remove_symbols ) {
			args.add( "-g" );
		}

		args.add_range( &intermediateFiles );

		For ( u32, libPathIndex, 0, config->additional_lib_paths.size() ) {
			args.add( tprintf( "-L%s", config->additional_lib_paths[libPathIndex].c_str() ) );
		}

		// TODO(DM): 11/02/2026: does just mean we dont care about file extensions when linking to static libraries?
		// this is turning into a mess
		if ( isGCC ) {
			For ( u32, libIndex, 0, config->additional_libs.size() ) {
				args.add( tprintf( "-l%s", path_remove_file_extension( config->additional_libs[libIndex].c_str() ) ) );
			}
		} else {
			For ( u32, libIndex, 0, config->additional_libs.size() ) {
				std::string& staticLib = config->additional_libs[libIndex];

	#if defined( _WIN32 )
				args.add( tprintf( "-l%s", staticLib.c_str() ) );
	#elif defined( __linux__ )
				if ( string_starts_with( staticLib.c_str(), "lib" ) ) {
					staticLib.erase( 0, strlen( "lib" ) );
					args.add( tprintf( "-l%s", staticLib.c_str() ) );
				} else {
					args.add( tprintf( "-l:%s", staticLib.c_str() ) );
				}
	#endif
			}
		}

		// TODO(DM): 09/10/2025: this works fine but do we want to expose this to the user?
		// or do we want to just do this by default on linux because its a really common thing that people do?
#ifdef __linux__
		if ( config->binary_type == BINARY_TYPE_EXE ) {
			const char* fullBinaryPath = path_remove_file_from_path( fullBinaryName );
			args.add( tprintf( "-Wl,-rpath=%s", fullBinaryPath ) );
		}
#endif

		args.add( "-o" );
		args.add( fullBinaryName );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

// only call this after compilation has finished successfully
// parse the dependency file that we generated for every dependency thats in there
// add those to a list - we need to put those in the .build_info file
static void Clang_GetIncludeDependenciesFromSourceFileBuild( compilerBackend_t* backend, std::vector<std::string>& outIncludeDependencies ) {
	clangState_t* clangState = cast( clangState_t*, backend->data );

	outIncludeDependencies = clangState->includeDependencies;
}

static String Clang_GetCompilerPath( compilerBackend_t* backend ) {
	clangState_t *clangState = cast( clangState_t *, backend->data );

	return clangState->compilerPath;
}

static String Clang_GetCompilerVersion( compilerBackend_t* backend ) {
	clangState_t* clangState = cast( clangState_t*, backend->data );

	String compilerVersion;

	const char* clangVersionPrefix = "clang version ";

	Array<const char*> args;
	args.add( clangState->compilerPath.data );
	args.add( "-v" );

	Process* process = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	if ( !process ) {
		error( "Failed to find process \"%s\".  Did you type it correctly?\n", args[0] );
		return String();
	}

	StringBuilder clangOutput = {};
	string_builder_reset( &clangOutput );
	defer( string_builder_destroy( &clangOutput ) );

	char buffer[1024] = {};
	u64 bytesRead = U64_MAX;
	while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
		buffer[bytesRead] = 0;

		string_builder_appendf( &clangOutput, "%s", buffer );
	}

	const char* clangOutputString = string_builder_to_string( &clangOutput );

	const char* versionStart = strstr( clangOutputString, clangVersionPrefix );

	if ( versionStart ) {
		versionStart += strlen( clangVersionPrefix );

		const char* versionEnd = NULL;
		if ( !versionEnd ) versionEnd = strchr( versionStart, '\r' );
		if ( !versionEnd ) versionEnd = strchr( versionStart, '\n' );
		assert( versionEnd );

		u64 versionLength = cast( u64, versionEnd ) - cast( u64, versionStart );

		string_copy_from_c_string( &compilerVersion, versionStart, versionLength );
	}

	process_join( process );

	process_destroy( process );
	process = NULL;

	return compilerVersion;
}

static String GCC_GetCompilerVersion( compilerBackend_t* backend ) {
	clangState_t* clangState = cast( clangState_t*, backend->data );

	String compilerVersion;

	const char* gccVersionPrefix = "gcc version ";

	Array<const char*> args;
	args.add( clangState->compilerPath.data );
	args.add( "-v" );

	Process* process = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	if ( !process ) {
		error( "Failed to find process \"%s\".  Did you type it correctly?\n", args[0] );
		return String();
	}

	StringBuilder gccOutput = {};
	string_builder_reset( &gccOutput );
	defer( string_builder_destroy( &gccOutput ) );

	char buffer[1024] = {};
	u64 bytesRead = U64_MAX;
	while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
		buffer[bytesRead] = 0;

		string_builder_appendf( &gccOutput, "%s", buffer );
	}

	const char* gccOutputString = string_builder_to_string( &gccOutput );

	const char* versionStart = strstr( gccOutputString, gccVersionPrefix );

	if ( versionStart ) {
		versionStart += strlen( gccVersionPrefix );

		const char* versionEnd = strchr( versionStart, ' ' );
		assert( versionEnd );

		u64 versionLength = cast( u64, versionEnd ) - cast( u64, versionStart );

		string_copy_from_c_string( &compilerVersion, versionStart, versionLength );
	}

	process_join( process );

	process_destroy( process );
	process = NULL;

	return compilerVersion.data;
}

void CreateCompilerBackend_Clang( compilerBackend_t* outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= Clang_Init,
		.Shutdown									= Clang_Shutdown,
		.CompileSourceFile							= Clang_CompileSourceFile,
		.LinkIntermediateFiles						= Clang_LinkIntermediateFiles,
		.GetIncludeDependenciesFromSourceFileBuild	= Clang_GetIncludeDependenciesFromSourceFileBuild,
		.GetCompilerPath							= Clang_GetCompilerPath,
		.GetCompilerVersion							= Clang_GetCompilerVersion,
	};
}

void CreateCompilerBackend_GCC( compilerBackend_t* outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= GCC_Init,
		.Shutdown									= Clang_Shutdown,
		.CompileSourceFile							= Clang_CompileSourceFile,
		.LinkIntermediateFiles						= Clang_LinkIntermediateFiles,
		.GetIncludeDependenciesFromSourceFileBuild	= Clang_GetIncludeDependenciesFromSourceFileBuild,
		.GetCompilerPath							= Clang_GetCompilerPath,
		.GetCompilerVersion							= GCC_GetCompilerVersion,
	};
}
