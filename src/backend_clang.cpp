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

#ifdef _WIN32
#include "win_support.h"
#endif

#include "temp_storage.h"
#include "string.h"
#include "debug.h"
#include "paths.h"
#include "array.inl"
#include "file.h"
#include "subprocess.h"
#include "string_builder.h"
#include "defer.h"
#include "helpers.h"
#include "library.h"

#include <clang-c/Index.h>

struct clangState_t {
	// TODO(DM): 11/02/2026: remove these when eds command archetype changes get merged in
	String						compilerPath;
	String						compilerVersion;
	String						linkerPath;
	String						arPath;	// static library linker for gcc (on windows and linux) and clang (linux)

#ifdef _WIN32
	windowsSDK_t				winSDK;
	msvcInstall_t				msvcInstall;
#endif
};


static const char *OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "-O0";
		case OPTIMIZATION_LEVEL_O1:	return "-O1";
		case OPTIMIZATION_LEVEL_O2:	return "-O2";
		case OPTIMIZATION_LEVEL_O3:	return "-O3";
	}
}

static void ReadDependencyFile( const char *depFilename, std::vector<std::string> &outIncludeDependencies ) {
	LogVerbose( "Parsing dependency file \"%s\"...\n", depFilename );

	String depFileBuffer = {};

	if ( !file_read_entire( depFilename, &depFileBuffer ) ) {
		s32 errorCode = get_last_error_code();
		fatal_error( "Failed to read \"%s\".  This should never happen! Error code: " ERROR_CODE_FORMAT "\n", depFilename, errorCode );
		return;
	}

	defer { file_free_buffer( &depFileBuffer ); };

	outIncludeDependencies.clear();

	char *current = depFileBuffer.data;

	// .d files start with the name of the binary followed by a colon
	// so skip past that first
	current = strchr( depFileBuffer.data, ':' );
	assert( current );
	current += 1;	// skip past the colon
	current += 1;	// skip past the following whitespace

	// skip past the newline after
	current = strchr( current, '\n' );
	assert( current );
	current += 1;

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

		{
			u64 lastWriteTime = GetLastFileWriteTime( dependencyFilename.c_str() );
			LogVerbose( " - Found dependency %s, last write time = %llu\n", dependencyFilename.c_str(), lastWriteTime );
		}

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

	LogVerbose( "Finished parsing dependency file \"%s\"...\n", depFilename );
}

static void ResolveCompilerAndLinkerPaths( clangState_t *clangState, LinearAllocator *allocator, const char *compilerPath, const char *compilerName, const char *linkerName ) {
	String compilerPathStr = string_set( compilerPath );
	String pathToCompiler = path_remove_file_from_path( &compilerPathStr );

	if ( pathToCompiler.count != compilerPathStr.count ) {
		const char *pathToCompilerCStr = string_cstr( &pathToCompiler );
		clangState->compilerPath = path_join( allocator, pathToCompilerCStr, compilerName );
		clangState->linkerPath = path_join( allocator, pathToCompilerCStr, linkerName );
	} else {
		clangState->compilerPath = string_alloc( allocator, compilerPath, strlen( compilerPath ) + 1 );
		clangState->linkerPath = string_alloc( allocator, linkerName, strlen( linkerName ) + 1 );
	}
}

//================================================================

static bool8 Clang_Init( compilerBackend_t *backend, const buildContext_t *context, const char *compilerPath, const char *compilerVersion ) {
	backend->data = cast( clangState_t *, linear_allocator_alloc( context->allocator, sizeof( clangState_t ) ) );
	new( backend->data ) clangState_t;

	clangState_t *clangState = cast( clangState_t *, backend->data );

	if ( compilerVersion ) {
		clangState->compilerVersion = string_alloc( context->allocator, compilerVersion, strlen( compilerVersion ) + 1 );
	}

	const char *clangExe = "clang";
#if defined( _WIN32 )
	const char *linkerExe = "lld-link";
#elif defined( __linux__ )
	const char *linkerExe = "llvm-ar";
#else
#error Unrecognised platform.
#endif

	ResolveCompilerAndLinkerPaths( clangState, context->allocator, compilerPath, clangExe, linkerExe );

	String pathToCompiler = string_set( compilerPath );
	pathToCompiler = path_remove_file_from_path( &pathToCompiler );

#if defined( _WIN32 )
	clangState->arPath = path_join( context->allocator, string_cstr( &pathToCompiler ), "ar" );
#elif defined( __linux__ )
	clangState->arPath = path_join( context->allocator, string_cstr( &pathToCompiler ), linkerExe );
#endif

#ifdef _WIN32
	clangState->winSDK = context->winSDK;
	clangState->msvcInstall = context->msvcInstall;
#endif

	return true;
}

static bool8 GCC_Init( compilerBackend_t *backend, const buildContext_t *context, const char *compilerPath, const char *compilerVersion ) {
	backend->data = cast( clangState_t *, linear_allocator_alloc( context->allocator, sizeof( clangState_t ) ) );
	new( backend->data ) clangState_t;

	clangState_t *clangState = cast( clangState_t *, backend->data );

	if ( compilerVersion ) {
		clangState->compilerVersion = string_alloc( context->allocator, compilerVersion, strlen( compilerVersion ) + 1 );
	}

	ResolveCompilerAndLinkerPaths( clangState, context->allocator, compilerPath, "gcc", "ld" );

	String compilerPathStr = string_set( compilerPath );
	String pathToCompiler = path_remove_file_from_path( &compilerPathStr );

	if ( pathToCompiler.count != compilerPathStr.count ) {
		clangState->arPath = path_join( context->allocator, string_cstr( &pathToCompiler ), "ar" );
	} else {
		clangState->arPath = string_alloc( context->allocator, "ar", strlen( "ar" ) + 1 );
	}

	return true;
}

static void Clang_Shutdown( compilerBackend_t *backend ) {
	clangState_t *clangState = cast( clangState_t *, backend->data );

	clangState->~clangState_t();
	backend->data = NULL;
}

static bool8 Clang_CompileSourceFile(
	compilerBackend_t *backend,
	buildContext_t *buildContext,
	BuildConfig *config,
	compilationCommandArchetype_t &cmdArchetype,
	const char *sourceFile,
	bool recordCompilation,
	u64 sourceFileIndex,
	std::vector<std::string> *outIncludeDependencies )
{
	assert( backend );
	assert( sourceFile );

	clangState_t *clangState = cast( clangState_t *, backend->data );

	String sourceFileNoPath = string_set( sourceFile );
	sourceFileNoPath = path_remove_path_from_file( &sourceFileNoPath );

	const char *depFilename = temp_printf( "%s%c%s.d", config->intermediateFolder.c_str(), PATH_SEPARATOR, string_cstr( &sourceFileNoPath ) );

	Array<const char *> finalArgs;
	finalArgs.init( mem_get_temp_storage() );
	finalArgs.add_range( &cmdArchetype.baseArgs );

	String sourceFileNoPathAndExtension = path_remove_file_extension( &sourceFileNoPath );

	const char *intermediateFile = temp_printf( "%s%c%s.o", config->intermediateFolder.c_str(), PATH_SEPARATOR, string_cstr( &sourceFileNoPathAndExtension ) );

	// Fill up remaining arguments

	// Dependency Flags/File
	For ( u64, flagIndex, 0, cmdArchetype.dependencyFlags.count ) {
		finalArgs.add( cmdArchetype.dependencyFlags[flagIndex] );
	}
	finalArgs.add( temp_printf( "%s%c%s.d", config->intermediateFolder.c_str(), PATH_SEPARATOR, sourceFileNoPath.data ) );

	// Output Flag/File
	finalArgs.add( cmdArchetype.outputFlag );
	finalArgs.add( intermediateFile );

	// Source File
	finalArgs.add( sourceFile );

	procFlags_t procFlags = PROC_FLAG_SHOW_STDOUT;
	if ( buildContext->consolidateCompilerArgs ) {
		printf( "%s -> %s\n", sourceFile, intermediateFile );
	} else {
		procFlags |= PROC_FLAG_SHOW_ARGS;
	}

	s32 exitCode = RunProc( &finalArgs, NULL, procFlags );

	if ( exitCode == 0 && outIncludeDependencies ) {
		ReadDependencyFile( depFilename, *outIncludeDependencies );
	}

	if ( recordCompilation ) {
		RecordCompilationDatabaseEntry( buildContext, sourceFile, finalArgs, sourceFileIndex );
	}

	return exitCode == 0;
}

static bool8 Clang_LinkIntermediateFiles( compilerBackend_t *backend, const std::vector<std::string> &intermediateFiles, BuildConfig *config, const BuilderOptions *options ) {
	assert( backend );
	assert( config );
	// assert( options );

	clangState_t *clangState = cast( clangState_t *, backend->data );

	const char *fullBinaryName = BuildConfig_GetFullBinaryName( config, mem_get_temp_storage() );

	Array<const char *> args;
	args.init( mem_get_temp_storage() );
	args.reserve(
		1 + // lib.exe or link.exe
		1 + // verbose flag
		1 + // /DLL
		1 + // /NODEFAULTLIB
		1 + // /DEBUG
		1 + // /OUT:
		1 + // kernel32.lib
		4 + // CRT libs (e.g. msvcrt, msvcprt, vcruntime, ucrt when -D_DLL and NOT -D_DEBUG)
		3 + // /LIBPATH: ucrt, um, msvc
		intermediateFiles.size() +
		config->additionalLibPaths.size() +
		config->additionalLibs.size() +
		config->additionalLinkerArguments.size()
	);

	// TODO(DM): 30/04/2026: this is a repetition of MSVC_LinkIntermediateFiles
	// so we need to start splitting backend files down by compiler and linker
	// and then unify the linker codepaths on windows when calling either clang or msvc
#ifdef _WIN32
	//args.add( clangState->linkerPath.data );
	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( temp_printf( "%s\\bin\\Hostx64\\x64\\lib", clangState->msvcInstall.rootFolder.data ) );
	} else {
		args.add( temp_printf( "%s\\bin\\Hostx64\\x64\\link", clangState->msvcInstall.rootFolder.data ) );

		args.add( "/NODEFAULTLIB" );
	}

	if ( g_verbose ) {
		args.add( "/verbose" );
	}

	if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
		args.add( "/DLL" );
	}

	if ( config->binaryType != BINARY_TYPE_STATIC_LIBRARY && !config->removeSymbols ) {
		args.add( "/DEBUG" );
	}

	args.add( temp_printf( "/OUT:%s", fullBinaryName ) );

	For ( u64, i, 0, intermediateFiles.size() ) {
		args.add( intermediateFiles[i].c_str() );
	}

	args.add( temp_printf( "/LIBPATH:%s", clangState->winSDK.ucrtLibPath.data ) );
	args.add( temp_printf( "/LIBPATH:%s", clangState->winSDK.umLibPath.data ) );
	args.add( temp_printf( "/LIBPATH:%s", clangState->msvcInstall.libPath.data ) );

	For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
		args.add( temp_printf( "/LIBPATH:%s", config->additionalLibPaths[libPathIndex].c_str() ) );
	}

	if ( !options || !options->noDefaultLibs ) {
		args.add( "kernel32.lib" );

		// these defines are what drive the choice of lib windows std compiles against
		bool8 debugBuild = false;
		bool8 hasDllRuntime = false;
		For ( u32, i, 0, config->defines.size() ) {
			if ( config->defines[i] == "_DEBUG" ) {
				debugBuild = true;
			}
			if ( config->defines[i] == "_DLL" ) {
				hasDllRuntime = true;
			}
		}

		// static library doesn't pass /NODEFAULTLIB so we don't need to supply it ourselves
		if ( config->binaryType != BINARY_TYPE_STATIC_LIBRARY ) {
			if( debugBuild ) {
				if ( hasDllRuntime ) {
				args.add( "msvcrtd.lib" );
				args.add( "msvcprtd.lib" );
				args.add( "vcruntimed.lib" );
				args.add( "ucrtd.lib" );
				}
				else {
					args.add( "libcmtd.lib" );
					args.add( "libcpmtd.lib" );
					args.add( "libvcruntimed.lib" );
					args.add( "libucrtd.lib" );
				}
			} else {
				if ( hasDllRuntime ) {
					args.add( "msvcrt.lib" );
					args.add( "msvcprt.lib" );
					args.add( "vcruntime.lib" );
					args.add( "ucrt.lib" );
				}
				else {
					args.add( "libcmt.lib" );
					args.add( "libcpmt.lib" );
					args.add( "libvcruntime.lib" );
					args.add( "libucrt.lib" );
				}
			}
		}
	}

	For ( u32, libIndex, 0, config->additionalLibs.size() ) {
		args.add( temp_printf( "%s%s", config->additionalLibs[libIndex].c_str(), GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) ) );
	}

	For ( u32, libIndex, 0, config->additionalLinkerArguments.size() ) {
		args.add( config->additionalLinkerArguments[libIndex].c_str() );
	}
#else
	// clang and gcc treat static libraries as just an archive of .o files
	// so there is no real "link" step in this case, the .o files are just "archived" together
	// for dynamic libraries and executables clang and gcc recommend you call the compiler again and just pass in all the intermediate files
	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( clangState->arPath.data );
		args.add( "rc" );
		args.add( fullBinaryName );

		if ( g_verbose ) {
			args.add( "-v" );
		}

		For ( u64, i, 0, intermediateFiles.size() ) {
			args.add( intermediateFiles[i].c_str() );
		}
	} else {
		args.add( clangState->compilerPath.data );

		if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
			args.add( "-shared" );
		}

		if ( !config->removeSymbols ) {
			args.add( "-g" );
		}

		if ( g_verbose ) {
			args.add( "-v" );
		}

		For ( u64, i, 0, intermediateFiles.size() ) {
			args.add( intermediateFiles[i].c_str() );
		}

		For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
			args.add( temp_printf( "-L%s", config->additionalLibPaths[libPathIndex].c_str() ) );
		}

#ifdef _WIN32
		args.add( temp_printf( "-L%s", clangState->winSDK.ucrtLibPath.data ) );
		args.add( temp_printf( "-L%s", clangState->winSDK.umLibPath.data ) );
#endif

#ifdef __linux__
		if ( !options || !options->noDefaultLibs ) {
			args.add( "-lstdc++" );
		}
#endif

		// on windows we already know the extension is going to be .lib, so we can add that ourselves
		// on linux the file extension could be either .a or .so depending on whether the library we are linking to is a static or dynamic library, respectively
		// so on linux make the user specify the file extension themselves
		For ( u32, libIndex, 0, config->additionalLibs.size() ) {
#ifdef _WIN32
			args.add( temp_printf( "-l%s%s", config->additionalLibs[libIndex].c_str(), GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) ) );
#else
			args.add( temp_printf( "-l%s", config->additionalLibs[libIndex].c_str() ) );
#endif
		}

		For ( u32, libIndex, 0, config->additionalLinkerArguments.size() ) {
			args.add( config->additionalLinkerArguments[libIndex].c_str() );
		}

		// TODO(DM): 09/10/2025: this works fine but do we want to expose this to the user?
		// or do we want to just do this by default on linux because its a really common thing that people do?
#ifdef __linux__
		if ( config->binaryType == BINARY_TYPE_EXE ) {
			String fullBinaryPath = string_set( fullBinaryName );
			fullBinaryPath = path_remove_file_from_path( &fullBinaryPath );

			args.add( temp_printf( "-Wl,-rpath=%s", string_cstr( &fullBinaryPath ) ) );
		}
#endif

		args.add( "-o" );
		args.add( fullBinaryName );
	}
#endif

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

static bool8 GCC_LinkIntermediateFiles( compilerBackend_t *backend, const std::vector<std::string> &intermediateFiles, BuildConfig *config, const BuilderOptions *options ) {
	assert( backend );
	assert( config );

	clangState_t *clangState = cast( clangState_t *, backend->data );

	const char *fullBinaryName = BuildConfig_GetFullBinaryName( config, mem_get_temp_storage() );

	Array<const char *> args;
	args.init( mem_get_temp_storage() );
	args.reserve(
		1 + // lld-link
		1 + // verbose flag
		1 + // /lib or -shared
		1 + // -nodefaultlibs
		1 + // -g
		1 + // -o
		1 + // binary name
		intermediateFiles.size() +
		config->additionalLibPaths.size() +
		config->additionalLibs.size() +
		config->additionalLinkerArguments.size()
	);

	// clang and gcc treat static libraries as just an archive of .o files
	// so there is no real "link" step in this case, the .o files are just "archived" together
	// for dynamic libraries and executables clang and gcc recommend you call the compiler again and just pass in all the intermediate files
	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( clangState->arPath.data );
		args.add( "rc" );
		args.add( fullBinaryName );

		if ( g_verbose ) {
			args.add( "-v" );
		}

		For ( u64, i, 0, intermediateFiles.size() ) {
			args.add( intermediateFiles[i].c_str() );
		}
	} else {
		args.add( clangState->compilerPath.data );

		if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
			args.add( "-shared" );
		}

		if ( !config->removeSymbols ) {
			args.add( "-g" );
		}

		if ( options && options->noDefaultLibs ) {
			args.add( "-nodefaultlibs" );
		}

		if ( g_verbose ) {
			args.add( "-v" );
		}

		For ( u64, i, 0, intermediateFiles.size() ) {
			args.add( intermediateFiles[i].c_str() );
		}

		For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
			args.add( temp_printf( "-L%s", config->additionalLibPaths[libPathIndex].c_str() ) );
		}

// #ifdef _WIN32
// 		args.add( temp_printf( "-L%s", clangState->winSDK.ucrtLibPath.data ) );
// 		args.add( temp_printf( "-L%s", clangState->winSDK.umLibPath.data ) );
// #endif

		For ( u32, libIndex, 0, config->additionalLibs.size() ) {
			args.add( temp_printf( "-l%s", config->additionalLibs[libIndex].c_str() ) );
		}

		For ( u32, libIndex, 0, config->additionalLinkerArguments.size() ) {
			args.add( config->additionalLinkerArguments[libIndex].c_str() );
		}

		// TODO(DM): 09/10/2025: this works fine but do we want to expose this to the user?
		// or do we want to just do this by default on linux because its a really common thing that people do?
#ifdef __linux__
		if ( config->binaryType == BINARY_TYPE_EXE ) {
			String fullBinaryPath = string_set( fullBinaryName );
			fullBinaryPath = path_remove_file_from_path( &fullBinaryPath );

			args.add( temp_printf( "-Wl,-rpath=%s", string_cstr( &fullBinaryPath ) ) );
		}
#endif

		args.add( "-o" );
		args.add( fullBinaryName );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

static bool8 Clang_GetCompilationCommandArchetype( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype ) {
	clangState_t *clangState = cast( clangState_t *, backend->data );

	outCmdArchetype.baseArgs.init( mem_get_temp_storage() );
	outCmdArchetype.dependencyFlags.init( mem_get_temp_storage() );

	const char *compilerPath = clangState->compilerPath.data;

	bool8 isClang = string_ends_with( compilerPath, "clang" ) || string_ends_with( compilerPath, "clang++" );

	// Not used originally but leaving here for clarity
	//bool8 isGCC = string_ends_with( backend->compilerPath.data, "gcc" ) || string_ends_with( backend->compilerPath.data, "g++" ); not used

	const u64 definesCount = config->defines.size();
	const u64 additionalIncludesCount = config->additionalIncludes.size();
	const u64 ignoredWarningsCount = config->ignoreWarnings.size();
	const u64 additionalArgsCount = config->additionalCompilerArguments.size();

	// Only reserve up enough up to additionalArgsCount,
	// as we keep dependency flags, and the output flag separate
	Array<const char *> &baseArgs = outCmdArchetype.baseArgs;
	baseArgs.reserve(
		1 +	// compiler path
		1 +	// verbose flag
		1 +	// compile flag
		1 +	// lang version flag
		1 +	// symbols flag
		1 +	// opt level flag
		definesCount +
		additionalIncludesCount +
		1 +	// warning as error flag
		1 +	// warning level flag
		ignoredWarningsCount +
		additionalArgsCount +
		2	// dependency flags
	);

	// Compiler Path
	baseArgs.add( compilerPath );

	if ( g_verbose ) {
		baseArgs.add( "-v" );
	}

	// Compile Flag
	baseArgs.add( "-c" );

	// Language Version
	if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
		baseArgs.add( temp_printf( "-std=%s", LanguageVersionToString( config->languageVersion ) ) );
	}

	// Symbols Flag
	if ( !config->removeSymbols ) {
		baseArgs.add( "-g" );
	}

	// Optimization Level
	baseArgs.add( OptimizationLevelToCompilerArg( config->optimizationLevel ) );

	// Defines
	For ( u32, defineIndex, 0, definesCount ) {
		baseArgs.add( temp_printf( "-D%s", config->defines[defineIndex].c_str() ) );
	}

// #ifdef _WIN32
// 	// windows SDK includes
// 	baseArgs.add( temp_printf( "-isystem%s", clangState->winSDK.ucrtInclude.data ) );
// 	baseArgs.add( temp_printf( "-isystem%s", clangState->winSDK.umInclude.data ) );
// 	baseArgs.add( temp_printf( "-isystem%s", clangState->winSDK.sharedInclude.data ) );
// #endif

	// Additional Includes
	For ( u32, includeIndex, 0, additionalIncludesCount ) {
		baseArgs.add( temp_printf( "-I%s", config->additionalIncludes[includeIndex].c_str() ) );
	}

	// Warning As Error
	if ( config->warningsAsErrors ) {
		baseArgs.add( "-Werror" );
	}

	// Warning Level
	{
		// -Weverything is clang-only; gcc stops at -Wpedantic
		static const char *allowedWarningLevels[] = { "-Wall", "-Wextra", "-Wpedantic", "-Weverything" };
		const u64 numAllowedWarningLevels = isClang ? 4 : 3;

		For ( u64, warningLevelIndex, 0, config->warningLevels.size() ) {
			const char *warningLevel = config->warningLevels[warningLevelIndex].c_str();

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, numAllowedWarningLevels ) {
				if ( strcmp( allowedWarningLevels[allowedWarningLevelIndex], warningLevel ) == 0 ) {
					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "\"%s\" is not allowed as a warning level.\n", warningLevel );
				return false;
			}

			baseArgs.add( warningLevel );
		}
	}

	// Ignored Warnings
	For ( u64, ignoreWarningIndex, 0, ignoredWarningsCount ) {
		baseArgs.add( config->ignoreWarnings[ignoreWarningIndex].c_str() );
	}

	// Additional Arguments
	For ( u64, additionalArgumentIndex, 0, additionalArgsCount ) {
		baseArgs.add( config->additionalCompilerArguments[additionalArgumentIndex].c_str() );
	}

	// Dependency Flags
	outCmdArchetype.dependencyFlags.add( "-MMD" );
	outCmdArchetype.dependencyFlags.add( "-MF" );

	// Output Flag
	outCmdArchetype.outputFlag = "-o";

	return true;
}

static String Clang_GetCompilerPath( compilerBackend_t *backend ) {
	clangState_t *clangState = cast( clangState_t *, backend->data );

	return clangState->compilerPath;
}

static String Clang_GetCompilerVersion( compilerBackend_t *backend ) {
	unused( backend );

	// TODO: DM: 09/05/2026: pretty sure we can do better with this bit here
	CXString clangVersionString = clang_getClangVersion();
	const char *clangVersionCStr = clang_getCString( clangVersionString );
	u64 clangVersionCStrLength = strlen( clangVersionCStr );
	defer { clang_disposeString( clangVersionString ); };

	String result = string_alloc( mem_get_temp_storage(), clangVersionCStr, clangVersionCStrLength + 1 );

	if ( string_starts_with( result.data, "clang version " ) ) {
		u64 versionPrefixLength = strlen( "clang version " );
		result.data += versionPrefixLength;
		result.count -= versionPrefixLength;
	}

	u64 firstWhitespacePos = 0;
	if ( string_find_from_left( &result, ' ', &firstWhitespacePos ) ) {
		result.count = firstWhitespacePos;
		result.data[result.count] = 0;
	}

	return result;
}

// TODO: DM: 09/05/2026: revisit this function
static String GCC_GetCompilerVersion( compilerBackend_t *backend ) {
	clangState_t *clangState = cast( clangState_t *, backend->data );

	String compilerVersion = {};

	const char *gccVersionPrefix = "gcc version ";

	Array<const char *> args;
	args.init( mem_get_temp_storage() );
	args.add( clangState->compilerPath.data );
	args.add( "-v" );

	String gccOutputString = {};
	s32 exitCode = RunProc( &args, NULL, 0, &gccOutputString );

	const char *versionStart = strstr( gccOutputString.data, gccVersionPrefix );

	if ( versionStart ) {
		versionStart += strlen( gccVersionPrefix );

		const char *versionEnd = strchr( versionStart, ' ' );
		assert( versionEnd );

		u64 versionLength = cast( u64, versionEnd ) - cast( u64, versionStart );

		compilerVersion = string_alloc( mem_get_temp_storage(), versionStart, versionLength + 1 );
		compilerVersion.data[versionLength] = '\0';
		compilerVersion.count = versionLength;
	}

	return compilerVersion;
}

void CreateCompilerBackend_Clang( compilerBackend_t *outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= Clang_Init,
		.Shutdown									= Clang_Shutdown,
		.CompileSourceFile							= Clang_CompileSourceFile,
		.LinkIntermediateFiles						= Clang_LinkIntermediateFiles,
		.GetCompilationCommandArchetype				= Clang_GetCompilationCommandArchetype,
		.GetCompilerPath							= Clang_GetCompilerPath,
		.GetCompilerVersion							= Clang_GetCompilerVersion,
	};
}

void CreateCompilerBackend_GCC( compilerBackend_t *outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= GCC_Init,
		.Shutdown									= Clang_Shutdown,
		.CompileSourceFile							= Clang_CompileSourceFile,
		.LinkIntermediateFiles						= GCC_LinkIntermediateFiles,
		.GetCompilationCommandArchetype				= Clang_GetCompilationCommandArchetype,
		.GetCompilerPath							= Clang_GetCompilerPath,
		.GetCompilerVersion							= GCC_GetCompilerVersion,
	};
}
