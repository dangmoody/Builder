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
#include "debug.h"
#include "string.h"
#include "paths.h"
#include "array.inl"
#include "string_builder.h"
#include "subprocess.h"
#include "file.h"
#include "temp_storage.h"
#include "helpers.h"

struct msvcState_t {
	string_t				compilerPath;
	string_t				compilerVersion;
	string_t				linkerPath;

	// windows sdk includes, msvc includes, that kind of thing
	array_t<const char *>	microsoftCoreIncludes;
	array_t<const char *>	microsoftCoreLibPaths;
};

//================================================================


static const char *OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "/Od";
		case OPTIMIZATION_LEVEL_O1:	return "/O1";
		case OPTIMIZATION_LEVEL_O2:	return "/O2";
		case OPTIMIZATION_LEVEL_O3:	return "/O2";
	}

	Assert( false && "Bad OptimizationLevel passed.\n" );

	return NULL;
}

//================================================================

// Querying for Windows SDK and MSVC is based off code from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// DM: I'm just straight lifting stuff from the code listing linked above - idc anymore
// its ridiculous that Microsoft genuinely think this isnt a frankly retarded way of grabbing some simple information off your PC

static bool8 MSVC_Init( compilerBackend_t *backend, const buildContext_t *context, const char *compilerPath, const char *compilerVersion ) {
	msvcState_t *msvcState = Cast( msvcState_t *, Mem_AllocatorAlloc( context->allocator, sizeof( msvcState_t ) ) );
	new( msvcState ) msvcState_t;

	msvcState->compilerPath = String_Alloc( context->allocator, compilerPath, strlen( compilerPath ) + 1 );

	if ( compilerVersion ) {
		msvcState->compilerVersion = String_Alloc( context->allocator, compilerVersion, strlen( compilerVersion ) + 1 );
	}

	backend->data = msvcState;

	msvcState->microsoftCoreIncludes.Init( context->allocator );
	msvcState->microsoftCoreLibPaths.Init( context->allocator );

	msvcState->microsoftCoreIncludes.Add( context->winSDK.ucrtInclude.data );
	msvcState->microsoftCoreIncludes.Add( context->winSDK.umInclude.data );
	msvcState->microsoftCoreIncludes.Add( context->winSDK.sharedInclude.data );

	msvcState->microsoftCoreLibPaths.Add( context->winSDK.ucrtLibPath.data );
	msvcState->microsoftCoreLibPaths.Add( context->winSDK.umLibPath.data );

	if ( String_Equals( compilerPath, "cl" ) || String_Equals( compilerPath, "cl.exe" ) ) {
		msvcState->compilerPath = path_join( context->allocator, context->msvcInstall.rootFolder.data, "bin", "Hostx64", "x64", "cl" );
		msvcState->linkerPath = path_join( context->allocator, context->msvcInstall.rootFolder.data, "bin", "Hostx64", "x64", "link" );
	} else {
		string_t compilerDir = String_Set( compilerPath );
		compilerDir = Path_RemoveFileFromPath( &compilerDir );
		msvcState->linkerPath = path_join( context->allocator, String_Cstr( &compilerDir ), "link" );
	}

	msvcState->microsoftCoreIncludes.Add( context->msvcInstall.includePath.data );
	msvcState->microsoftCoreLibPaths.Add( context->msvcInstall.libPath.data );

	return true;
}

static void MSVC_Shutdown( compilerBackend_t *backend ) {
	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );

	msvcState->~msvcState_t();
	backend->data = NULL;
}

static bool8 MSVC_CompileSourceFile(
	compilerBackend_t *backend,
	buildContext_t *buildContext,
	BuildConfig *config,
	compilationCommandArchetype_t &cmdArchetype,
	const char *sourceFile,
	bool recordCompilation,
	u64 sourceFileIndex,
	std::vector<std::string> *outIncludeDependencies )
{
	Assert( backend );
	Assert( sourceFile );
	Assert( config );

	string_t sourceFileNoPathAndExtension = String_Set( sourceFile );
	sourceFileNoPathAndExtension = Path_RemovePathFromFile( &sourceFileNoPathAndExtension );
	sourceFileNoPathAndExtension = Path_RemoveFileExtension( &sourceFileNoPathAndExtension );

	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );


	array_t<const char *> finalArgs;
	finalArgs.Init( Mem_GetTempStorage() );
	finalArgs.AddRange( &cmdArchetype.baseArgs );

	const char *intermediateFile = TempPrintf( "%s%c%s.o", config->intermediateFolder.c_str(), PATH_SEPARATOR, String_Cstr( &sourceFileNoPathAndExtension ) );

	// Fill up remaining arguments

	// Output Flag/File
	finalArgs.Add( TempPrintf( "%s%s", cmdArchetype.outputFlag, intermediateFile ) );

	// Source File
	finalArgs.Add( sourceFile );

	// MSVC doesnt output include dependencies to .d files
	// it only supports printing them to stdout
	// so we have to parse the stdout of the process ourselves
	procFlags_t procFlags = 0;
	if ( buildContext->consolidateCompilerArgs ) {
		printf( "%s -> %s\n", sourceFile, intermediateFile );
	} else {
		procFlags = PROC_FLAG_SHOW_ARGS;
	}

	string_t processStdout = {};
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
		char *lineStart = Cast( char *, buffer );
#pragma clang diagnostic pop

		while ( *lineStart ) {
			char *lineEnd = strchr( lineStart, '\n' );

			if ( !lineEnd ) {
				continue;
			}

			u64 lineLength = Cast( u64, lineEnd ) - Cast( u64, lineStart );
			std::string bufferLine( lineStart, lineLength );

			if ( String_EndsWith( bufferLine.c_str(), "\r" ) ) {
				bufferLine.pop_back();
			}

			if ( String_StartsWith( bufferLine.c_str(), includeDependencyPrefix ) ) {
				bufferLine.erase( 0, includeDependencyPrefixLength );

				while ( bufferLine[0] == ' ' ) {
					bufferLine.erase( 0, 1 );
				}

				if ( outIncludeDependencies ) {
					outIncludeDependencies->push_back( bufferLine );
				}
			} else {
				printf( "%s\n", bufferLine.c_str() );
			}

			lineStart = lineEnd + 1;
		}
	}

	if ( recordCompilation ) {
		RecordCompilationDatabaseEntry( buildContext, sourceFile, finalArgs, sourceFileIndex );
	}

	return exitCode == 0;
}

static bool8 MSVC_LinkIntermediateFiles( compilerBackend_t *backend, const std::vector<std::string> &intermediateFiles, BuildConfig *config, const BuilderOptions *options ) {
	Assert( backend );
	Assert( config );

	const char *fullBinaryName = BuildConfig_GetFullBinaryName( config, Mem_GetTempStorage() );

	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );
	array_t<const char *> args;
	args.Init( Mem_GetTempStorage() );
	args.Reserve(
		1 +	// link
		1 +	// /lib or /shared
		1 +	// /DEBUG
		1 + // /NODEFAULTLIB
		1 +	// /OUT:<name>
		intermediateFiles.size() +
		msvcState->microsoftCoreLibPaths.count +
		config->additionalLibPaths.size() +
		config->additionalLibs.size()
	);

	// TODO(DM): 30/04/2026: this is a repetition of the windows path of Clang_LinkIntermediateFiles
	// so we need to start splitting backend files down by compiler and linker
	// and then unify the linker codepaths on windows when calling either clang or msvc
	args.Add( msvcState->linkerPath.data );

	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.Add( "/lib" );
	} else if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
		args.Add( "/DLL" );
	}

	if ( !config->removeSymbols && config->binaryType != BINARY_TYPE_STATIC_LIBRARY ) {
		args.Add( "/DEBUG" );
	}

	if ( options && options->noDefaultLibs ) {
		args.Add( "/NODEFAULTLIB" );
	}

	args.Add( TempPrintf( "/OUT:%s", fullBinaryName ) );

	For ( u64, i, 0, intermediateFiles.size() ) {
		args.Add( intermediateFiles[i].c_str() );
	}

	For ( u32, libPathIndex, 0, msvcState->microsoftCoreLibPaths.count ) {
		args.Add( TempPrintf( "/LIBPATH:%s", msvcState->microsoftCoreLibPaths[libPathIndex] ) );
	}

	For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
		args.Add( TempPrintf( "/LIBPATH:%s", config->additionalLibPaths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, config->additionalLibs.size() ) {
		args.Add( TempPrintf( "%s%s", config->additionalLibs[libIndex].c_str(), GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) ) );
	}

	For ( u32, libIndex, 0, config->additionalLinkerArguments.size() ) {
		args.Add( config->additionalLinkerArguments[libIndex].c_str() );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

static bool8 MSVC_GetCompilationCommandArchetype( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype ) {
	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );

	outCmdArchetype.baseArgs.Init( Mem_GetTempStorage() );
	outCmdArchetype.dependencyFlags.Init( Mem_GetTempStorage() );

	const u64 definesCount = config->defines.size();
	const u64 microsoftCoreIncludesCount = msvcState->microsoftCoreIncludes.count;
	const u64 additionalIncludesCount = config->additionalIncludes.size();
	const u64 ignoredWarningsCount = config->ignoreWarnings.size();
	const u64 additionalArgsCount = config->additionalCompilerArguments.size();

	array_t<const char *> &baseArgs = outCmdArchetype.baseArgs;
	baseArgs.Reserve(
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
	baseArgs.Add( msvcState->compilerPath.data );

	// Compile Flag
	baseArgs.Add( "/c" );

	// Language Version
	if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
		baseArgs.Add( TempPrintf( "/std:%s", LanguageVersionToString( config->languageVersion ) ) );
	}

	// Symbols Flag
	if ( !config->removeSymbols ) {
		baseArgs.Add( "/DEBUG" );
	}

	// Optimization Level
	if ( config->optimizationLevel == OPTIMIZATION_LEVEL_O3 ) {
		static bool8 warned = false;
		if ( !warned ) {
			Warning( "MSVC doesn't have optimization level /O3. /O2 is the maximum. Defaulting to that instead...\n" );
			warned = true;
		}
	}
	baseArgs.Add( OptimizationLevelToCompilerArg( config->optimizationLevel ) );

	// Diagnostics Flag
	baseArgs.Add( "/showIncludes" );

	// Defines
	For ( u32, defineIndex, 0, definesCount ) {
		baseArgs.Add( TempPrintf( "/D%s", config->defines[defineIndex].c_str() ) );
	}

	// Microsoft Core Includes
	For ( u32, includeIndex, 0, microsoftCoreIncludesCount ) {
		baseArgs.Add( TempPrintf( "/I%s", msvcState->microsoftCoreIncludes[includeIndex] ) );
	}

	// Additional Includes
	For ( u32, includeIndex, 0, additionalIncludesCount ) {
		baseArgs.Add( TempPrintf( "/I%s", config->additionalIncludes[includeIndex].c_str() ) );
	}

	// Warning As Error
	if ( config->warningsAsErrors ) {
		baseArgs.Add( "/WX" );
	}

	// Warning Level
	{
		static const char *allowedWarningLevels[] = { "/W", "/W0", "/W1", "/W2", "/W3", "/W4", "/Wall" };

		// MSVC only allows one warning level to be set
		if ( config->warningLevels.size() > 1 ) {
			stringBuilder_t builder = SB_Create( Mem_GetTempStorage() );

			SB_Appendf( &builder, "MSVC only allows ONE of the following warning levels to be set:\n" );

			For ( u64, allowedWarningLevelIndex, 0, COUNT_OF( allowedWarningLevels ) ) {
				SB_Appendf( &builder, "%s, ", allowedWarningLevels[allowedWarningLevelIndex] );
			}

			Error( "%s\n", SB_ToString( &builder ) );

			return false;
		}

		For ( u64, warningLevelIndex, 0, config->warningLevels.size() ) {
			const char *warningLevel = config->warningLevels[warningLevelIndex].c_str();

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, COUNT_OF( allowedWarningLevels ) ) {
				if ( String_Equals( allowedWarningLevels[allowedWarningLevelIndex], warningLevel ) ) {
					found = true;
					break;
				}
			}

			if ( !found ) {
				Error( "\"%s\" is not allowed as a warning level.\n", warningLevel );
				return false;
			}

			baseArgs.Add( warningLevel );
		}
	}

	// Ignored Warnings
	For ( u64, ignoreWarningIndex, 0, ignoredWarningsCount ) {
		baseArgs.Add( config->ignoreWarnings[ignoreWarningIndex].c_str() );
	}

	// Additional Arguments
	For ( u64, additionalArgumentIndex, 0, additionalArgsCount ) {
		baseArgs.Add( config->additionalCompilerArguments[additionalArgumentIndex].c_str() );
	}

	// Dependency Flags
	// MSVC doesn't have any

	// Output Flag
	outCmdArchetype.outputFlag = "/Fo";

	return true;
}

static string_t MSVC_GetCompilerPath( compilerBackend_t *backend ) {
	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );

	return msvcState->compilerPath;
}

static string_t MSVC_GetCompilerVersion( compilerBackend_t *backend ) {
	msvcState_t *msvcState = Cast( msvcState_t *, backend->data );

	return msvcState->compilerVersion;
}

void CreateCompilerBackend_MSVC( compilerBackend_t *outBackend ) {
	*outBackend = compilerBackend_t {
		.data							= NULL,
		.Init							= MSVC_Init,
		.Shutdown						= MSVC_Shutdown,
		.CompileSourceFile				= MSVC_CompileSourceFile,
		.LinkIntermediateFiles			= MSVC_LinkIntermediateFiles,
		.GetCompilationCommandArchetype	= MSVC_GetCompilationCommandArchetype,
		.GetCompilerPath				= MSVC_GetCompilerPath,
		.GetCompilerVersion				= MSVC_GetCompilerVersion,
	};
}

#endif // _WIN32
