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

#ifdef _WIN32

#include "builder_local.h"
#include "win_support.h"

#include "core/include/debug.h"
#include "core/include/string_helpers.h"
#include "core/include/paths.h"
#include "core/include/array.inl"
#include "core/include/string_builder.h"
#include "core/include/core_process.h"
#include "core/include/file.h"
#include "core/include/temp_storage.h"

struct msvcState_t {
	String						compilerPath;
	String						compilerVersion;
	String						linkerPath;

	Array<const char *>			args;

	// windows sdk includes, msvc includes, that kind of thing
	std::vector<std::string>	microsoftCoreIncludes;
	std::vector<std::string>	microsoftCoreLibPaths;

	std::vector<std::string>	includeDependencies;
};

//================================================================

static const char *LanguageVersionToCompilerArg( const LanguageVersion languageVersion ) {
	switch ( languageVersion ) {
		case LANGUAGE_VERSION_C89:		return "/std:c89";
		case LANGUAGE_VERSION_C99:		return "/std:c99";
		case LANGUAGE_VERSION_C11:		return "/std:c11";
		case LANGUAGE_VERSION_C17:		return "/std:c17";
		case LANGUAGE_VERSION_C23:		return "/std:c23";
		case LANGUAGE_VERSION_CPP11:	return "/std:c++11";
		case LANGUAGE_VERSION_CPP14:	return "/std:c++14";
		case LANGUAGE_VERSION_CPP17:	return "/std:c++17";
		case LANGUAGE_VERSION_CPP20:	return "/std:c++20";
		case LANGUAGE_VERSION_CPP23:	return "/std:c++23";
	}

	assert( false && "Bad LanguageVersion passed.\n" );

	return NULL;
}

static const char *OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "/Od";
		case OPTIMIZATION_LEVEL_O1:	return "/O1";
		case OPTIMIZATION_LEVEL_O2:	return "/O2";
		case OPTIMIZATION_LEVEL_O3:	return "/O2";
	}

	assert( false && "Bad OptimizationLevel passed.\n" );

	return NULL;
}

//================================================================

// Querying for Windows SDK and MSVC is based off code from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// DM: I'm just straight lifting stuff from the code listing linked above - idc anymore
// its ridiculous that Microsoft genuinely think this isnt a frankly retarded way of grabbing some simple information off your PC

static bool8 MSVC_Init( compilerBackend_t *backend, const buildContext_t *context, const std::string &compilerPath, const std::string &compilerVersion ) {
	msvcState_t *msvcState = cast( msvcState_t *, mem_alloc( sizeof( msvcState_t ) ) );
	new( msvcState ) msvcState_t;

	msvcState->compilerPath = compilerPath.c_str();
	msvcState->compilerVersion = compilerVersion.c_str();

	backend->data = msvcState;

	msvcState->microsoftCoreIncludes.push_back( context->winSDK.ucrtInclude.data );
	msvcState->microsoftCoreIncludes.push_back( context->winSDK.umInclude.data );
	msvcState->microsoftCoreIncludes.push_back( context->winSDK.sharedInclude.data );

	msvcState->microsoftCoreLibPaths.push_back( context->winSDK.ucrtLibPath.data );
	msvcState->microsoftCoreLibPaths.push_back( context->winSDK.umLibPath.data );

	if ( string_equals( compilerPath.c_str(), "cl" ) || string_equals( compilerPath.c_str(), "cl.exe" ) ) {
		string_printf( &msvcState->compilerPath, "%s\\bin\\Hostx64\\x64\\cl", context->msvcInstall.rootFolder.data );
		string_printf( &msvcState->linkerPath, "%s\\bin\\Hostx64\\x64\\link", context->msvcInstall.rootFolder.data );
	} else {
		const char *compilerDir = path_remove_file_from_path( compilerPath.c_str() );
		string_printf( &msvcState->linkerPath, "%s\\link", compilerDir );
	}

	msvcState->microsoftCoreIncludes.push_back( context->msvcInstall.includePath.data );
	msvcState->microsoftCoreLibPaths.push_back( context->msvcInstall.libPath.data );

	return true;
}

static void MSVC_Shutdown( compilerBackend_t *backend ) {
	mem_free( backend->data );
	backend->data = NULL;
}

static bool8 MSVC_CompileSourceFile(
	compilerBackend_t *backend,
	buildContext_t *buildContext,
	BuildConfig *config,
	compilationCommandArchetype_t &cmdArchetype,
	const char *sourceFile,
	bool recordCompilation )
{
	assert( backend );
	assert( sourceFile );
	assert( config );

	const char *sourceFileNoPath = path_remove_path_from_file( sourceFile );
	const char *sourceFileNoExtension = path_remove_file_extension( sourceFileNoPath );

	config->additionalIncludes.emplace_back( "." );

	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	msvcState->includeDependencies.clear();

	Array<const char *> finalArgs = cmdArchetype.baseArgs;

	const char *intermediateFile = tprintf( "%s%c%s.o", config->intermediateFolder.c_str(), PATH_SEPARATOR, sourceFileNoExtension );

	// Fill up remaining arguments

	// Output Flag/File
	finalArgs.add( tprintf( "%s%s", cmdArchetype.outputFlag, intermediateFile ) );

	// Source File
	finalArgs.add( sourceFile );

	// MSVC doesnt output include dependencies to .d files
	// it only supports printing them to stdout
	// so we have to parse the stdout of the process ourselves
	procFlags_t procFlags = 0;
	if ( buildContext->consolidateCompilerArgs ) {
		printf( "%s -> %s\n", sourceFile, intermediateFile );
	} else {
		procFlags = PROC_FLAG_SHOW_ARGS;
	}

	String processStdout;
	s32 exitCode = RunProc( &finalArgs, NULL, procFlags, &processStdout );

	// now parse the stdout
	// all include dependencies are on their own line
	// the line always starts with a specific prefix
	{
		const char *buffer = processStdout.data;

		const char *includeDependencyPrefix = "Note: including file: ";
		const u64 includeDependencyPrefixLength = strlen( includeDependencyPrefix );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		char *lineStart = cast( char *, buffer );
#pragma clang diagnostic pop

		while ( *lineStart ) {
			char *lineEnd = strchr( lineStart, '\n' );

			if ( !lineEnd ) {
				continue;
			}

			u64 lineLength = cast( u64, lineEnd ) - cast( u64, lineStart );
			std::string bufferLine( lineStart, lineLength );

			if ( string_ends_with( bufferLine.c_str(), "\r" ) ) {
				bufferLine.pop_back();
			}

			if ( string_starts_with( bufferLine.c_str(), includeDependencyPrefix ) ) {
				bufferLine.erase( 0, includeDependencyPrefixLength );

				while ( bufferLine[0] == ' ' ) {
					bufferLine.erase( 0, 1 );
				}

				msvcState->includeDependencies.push_back( bufferLine );
			} else {
				printf( "%s\n", bufferLine.c_str() );
			}

			lineStart = lineEnd + 1;
		}
	}

	if ( recordCompilation ) {
		RecordCompilationDatabaseEntry( buildContext, sourceFile, finalArgs );
	}

	return exitCode == 0;
}

static bool8 MSVC_LinkIntermediateFiles( compilerBackend_t *backend, const Array<const char *> &intermediateFiles, BuildConfig *config, const BuilderOptions *options ) {
	assert( backend );
	assert( config );

	const char *fullBinaryName = BuildConfig_GetFullBinaryName( config );

	msvcState_t *msvcState = cast( msvcState_t *, backend->data );
	Array<const char *> &args = msvcState->args;
	args.reserve(
		1 +	// link
		1 +	// /lib or /shared
		1 +	// /DEBUG
		1 + // /NODEFAULTLIB
		1 +	// /OUT:<name>
		intermediateFiles.count +
		msvcState->microsoftCoreLibPaths.size() +
		config->additionalLibPaths.size() +
		config->additionalLibs.size()
	);

	args.reset();

	args.add( msvcState->linkerPath.data );

	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( "/lib" );
	} else if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
		args.add( "/DLL" );
	}

	if ( !config->removeSymbols ) {
		args.add( "/DEBUG" );
	}

	if ( options && options->noDefaultLibs && config->binaryType != BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( "/NODEFAULTLIB" );
	}

	args.add( tprintf( "/OUT:%s", fullBinaryName ) );

	args.add_range( &intermediateFiles );

	For ( u32, libPathIndex, 0, msvcState->microsoftCoreLibPaths.size() ) {
		args.add( tprintf( "/LIBPATH:%s", msvcState->microsoftCoreLibPaths[libPathIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
		args.add( tprintf( "/LIBPATH:%s", config->additionalLibPaths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, config->additionalLibs.size() ) {
		args.add( tprintf( "%s%s", config->additionalLibs[libIndex].c_str(), GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) ) );
	}

	For ( u32, libIndex, 0, config->additionalLinkerArguments.size() ) {
		args.add( config->additionalLinkerArguments[libIndex].c_str() );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

static bool8 MSVC_GetCompilationCommandArchetype( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	const u64 definesCount = config->defines.size();
	const u64 microsoftCoreIncludesCount = msvcState->microsoftCoreIncludes.size();
	const u64 additionalIncludesCount = config->additionalIncludes.size();
	const u64 ignoredWarningsCount = config->ignoreWarnings.size();
	const u64 additionalArgsCount = config->additionalCompilerArguments.size();

	Array<const char *> &baseArgs = outCmdArchetype.baseArgs;
	baseArgs.reserve(
		1 +	// compilerPath
		1 +	// compile flag
		1 +	// lang version flag
		1 +	// symbols flag
		1 +	// opt level flag
		definesCount +
		microsoftCoreIncludesCount +
		additionalIncludesCount +
		1 +	// warning as error flag
		1 +	// warning level flag
		ignoredWarningsCount +
		additionalArgsCount
	);

	// Compiler Path
	baseArgs.add( msvcState->compilerPath.data );

	// Compile Flag
	baseArgs.add( "/c" );

	// Language Version
	if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
		baseArgs.add( LanguageVersionToCompilerArg( config->languageVersion ) );
	}

	// Symbols Flag
	if ( !config->removeSymbols ) {
		baseArgs.add( "/DEBUG" );
	}

	// Optimization Level
	if ( config->optimizationLevel == OPTIMIZATION_LEVEL_O3 ) {
		static bool8 warned = false;
		if ( !warned ) {
			warning( "MSVC doesn't have optimization level /O3. /O2 is the maximum. Defaulting to that instead...\n" );
			warned = true;
		}
	}
	baseArgs.add( OptimizationLevelToCompilerArg( config->optimizationLevel ) );

	// Diagnostics Flag
	baseArgs.add( "/showIncludes" );

	// Defines
	For ( u32, defineIndex, 0, definesCount ) {
		baseArgs.add( tprintf( "/D%s", config->defines[defineIndex].c_str() ) );
	}

	// Microsoft Core Includes
	For ( u32, includeIndex, 0, microsoftCoreIncludesCount ) {
		baseArgs.add( tprintf( "/I%s", msvcState->microsoftCoreIncludes[includeIndex].c_str() ) );
	}

	// Additional Includes
	For ( u32, includeIndex, 0, additionalIncludesCount ) {
		baseArgs.add( tprintf( "/I%s", config->additionalIncludes[includeIndex].c_str() ) );
	}

	// Warning As Error
	if ( config->warningsAsErrors ) {
		baseArgs.add( "/WX" );
	}

	// Warning Level
	{
		std::vector<std::string> allowedWarningLevels = {
			"/W",
			"/W0",
			"/W1",
			"/W2",
			"/W3",
			"/W4",
			"/Wall",
		};

		// MSVC only allows one warning level to be set
		if ( config->warningLevels.size() > 1 ) {
			StringBuilder builder;
			string_builder_reset( &builder );

			string_builder_appendf( &builder, "MSVC only allows ONE of the following warning levels to be set:\n" );

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				string_builder_appendf( &builder, "%s, ", allowedWarningLevels[allowedWarningLevelIndex].c_str() );
			}

			error( "%s\n", string_builder_to_string( &builder ) );

			return false;
		}

		For ( u64, warningLevelIndex, 0, config->warningLevels.size() ) {
			const std::string &warningLevel = config->warningLevels[warningLevelIndex];

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

			baseArgs.add( warningLevel.c_str() );
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
	// MSVC doesn't have any

	// Output Flag
	outCmdArchetype.outputFlag = "/Fo";

	return true;
}

static void MSVC_GetIncludeDependenciesFromSourceFileBuild( compilerBackend_t *backend, std::vector<std::string> &outIncludeDependencies ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	outIncludeDependencies = msvcState->includeDependencies;
}

static String MSVC_GetCompilerPath( compilerBackend_t *backend ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	return msvcState->compilerPath;
}

static String MSVC_GetCompilerVersion( compilerBackend_t *backend ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	return msvcState->compilerVersion;
}

void CreateCompilerBackend_MSVC( compilerBackend_t *outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= MSVC_Init,
		.Shutdown									= MSVC_Shutdown,
		.CompileSourceFile							= MSVC_CompileSourceFile,
		.LinkIntermediateFiles						= MSVC_LinkIntermediateFiles,
		.GetCompilationCommandArchetype				= MSVC_GetCompilationCommandArchetype,
		.GetIncludeDependenciesFromSourceFileBuild	= MSVC_GetIncludeDependenciesFromSourceFileBuild,
		.GetCompilerPath							= MSVC_GetCompilerPath,
		.GetCompilerVersion							= MSVC_GetCompilerVersion,
	};
}

#endif // _WIN32
