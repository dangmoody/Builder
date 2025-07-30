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
#include "core/include/string_builder.h"
#include "core/include/core_process.h"
#include "core/include/file.h"

// TODO(DM): 20/07/2025: do we want to ignore this warning via the build script?
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char* LanguageVersionToCompilerArg( const LanguageVersion languageVersion ) {
	assert( languageVersion != LANGUAGE_VERSION_UNSET );

	switch ( languageVersion ) {
		case LANGUAGE_VERSION_C89:		return "/std=c89";
		case LANGUAGE_VERSION_C99:		return "/std=c99";
		case LANGUAGE_VERSION_CPP11:	return "/std=c++11";
		case LANGUAGE_VERSION_CPP14:	return "/std=c++14";
		case LANGUAGE_VERSION_CPP17:	return "/std=c++17";
		case LANGUAGE_VERSION_CPP20:	return "/std=c++20";
		case LANGUAGE_VERSION_CPP23:	return "/std=c++23";
	}

	return NULL;
}

#pragma clang diagnostic pop

static const char* OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "/Od";
		case OPTIMIZATION_LEVEL_O1:	return "/O1";
		case OPTIMIZATION_LEVEL_O2:	return "/O2";
		case OPTIMIZATION_LEVEL_O3:	return "/O2";	// DM!!! 22/07/2025: whats the real answer here?
	}
}

//================================================================

struct msvcState_t {
	Array<const char*>			args;

	std::vector<std::string>	windowsIncludes;
	std::vector<std::string>	windowsLibPaths;

	std::vector<std::string>	includeDependencies;
};

static msvcState_t* g_msvcState = NULL;

static bool8 MSVC_Init() {
	auto ParseTagString = []( const char* fileBuffer, const char* tag ) -> std::string {
		const char* lineStart = strstr( fileBuffer, tag );
		assert( lineStart );
		lineStart += strlen( tag );

		while ( *lineStart == ' ' ) {
			lineStart++;
		}

		const char* lineEnd = NULL;
		if ( !lineEnd ) lineEnd = strchr( lineStart, '\r' );
		if ( !lineEnd ) lineEnd = strchr( lineStart, '\n' );

		return std::string( lineStart, lineEnd );
	};

	auto ParseTagArray = []( const char* fileBuffer, const char* tag ) -> std::vector<std::string> {
		std::vector<std::string> paths;

		const char* lineStart = strstr( fileBuffer, tag );
		assert( lineStart );
		lineStart += strlen( tag );

		while ( *lineStart == ' ' ) {
			lineStart++;
		}

		const char* lineEnd = strchr( lineStart, '\n' );

		const char* semicolon = strchr( lineStart, ';' );

		while ( cast( u64, semicolon ) < cast( u64, lineEnd ) ) {
			u64 pathLength = cast( u64, semicolon ) - cast( u64, lineStart );
			std::string path( lineStart, pathLength );

			paths.push_back( path );

			lineStart = semicolon + 1;
			semicolon = strchr( lineStart, ';' );
		}

		return paths;
	};

	g_msvcState = cast( msvcState_t*, mem_alloc( sizeof( msvcState_t ) ) );
	new( g_msvcState ) msvcState_t;

	std::string msvcRootFolder;
	String clPath;

	// call vswhere.exe to get the MSVC root folder
	{
		const char* defaultVSWherePath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";

		Array<const char*> args;
		args.add( defaultVSWherePath );

		Process* process = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

		if ( !process ) {
			error( "I can't find vswhere.exe in teh default install directory on your PC (\"%s\").  I need this to be able to build with MSVC.  Sorry.\n" );
			return false;
		}

		StringBuilder processStdout = {};
		string_builder_reset( &processStdout );
		defer( string_builder_destroy( &processStdout ) );

		char buffer[1024] = {};
		u64 bytesRead = U64_MAX;
		while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
			buffer[bytesRead] = 0;

			string_builder_appendf( &processStdout, "%s", buffer );
		}

		s32 exitCode = process_join( process );

		const char* outputBuffer = string_builder_to_string( &processStdout );

		msvcRootFolder = ParseTagString( outputBuffer, "installationPath:" );

		process_destroy( process );
		process = NULL;
	}

	// get latest version of msvc
	String latestMSVCVersion;
	{
		const char* msvcVersionSearchFolder = tprintf( "%s\\VC\\Tools\\MSVC\\*", msvcRootFolder.c_str() );

		FileInfo fileInfo;
		File file = file_find_first( msvcVersionSearchFolder, &fileInfo );

		u32 highestVersion = 0;
		do {
			if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
				continue;
			}

			if ( !fileInfo.is_directory ) {
				continue;
			}

			u32 version0 = 0;
			u32 version1 = 0;
			u32 version2 = 0;
			sscanf( fileInfo.filename, "%u.%u.%u", &version0, &version1, &version2 );

			u32 mask = ( version0 << 24 ) | ( version1 << 16 ) | ( version2 );

			if ( mask > highestVersion ) {
				highestVersion = mask;

				latestMSVCVersion = fileInfo.filename;
			}
		} while ( file_find_next( &file, &fileInfo ) );
	}

	// now use MSVC root folder and the correct MSVC version to get the path to cl.exe
	string_printf( &clPath, "%s\\VC\\Tools\\MSVC\\%s\\bin\\Hostx64\\x64", msvcRootFolder.c_str(), latestMSVCVersion.data );

	// now microsoft need us to tell their own compiler that runs on their own platform (specifically FOR their own platform) where their own include and library folders are, sigh...
	// the way we do that is by manually calling a vcvars*.bat script and using the information it gives us back to know which include and lib folders to look for
	// and even then we still have to manually construct the windows SDK folders! AAARGH!
	Array<const char*> args;
	args.add( tprintf( "%s\\VC\\Auxiliary\\Build\\vcvars64.bat", msvcRootFolder.c_str() ) );
	args.add( "&&" );
	args.add( "set" );

	Process* vcvarsProcess = process_create( &args, NULL, /*PROCESS_FLAG_ASYNC*/0 );

	if ( !vcvarsProcess ) {
		error( "Failed to run vcvars64.bat.  Builder currently expects this to be installed in the default directory.  Sorry.\n" );
		return false;
	}

	StringBuilder vcvarsOutput = {};
	string_builder_reset( &vcvarsOutput );
	defer( string_builder_destroy( &vcvarsOutput ) );

	char buffer[1024] = {};
	u64 bytesRead = U64_MAX;
	while ( ( bytesRead = process_read_stdout( vcvarsProcess, buffer, 1024 ) ) ) {
		buffer[bytesRead] = 0;

		string_builder_appendf( &vcvarsOutput, "%s", buffer );
	}

	const char* outputBuffer = string_builder_to_string( &vcvarsOutput );

	{
		g_msvcState->windowsIncludes = ParseTagArray( outputBuffer, "INCLUDE=" );
		g_msvcState->windowsLibPaths = ParseTagArray( outputBuffer, "LIB=" );

		std::string windowsSDKVersion = ParseTagString( outputBuffer, "WindowsSDKLibVersion=" );
		windowsSDKVersion.pop_back();		// remove trailing slash

		std::string windowsSDKRootFolder = ParseTagString( outputBuffer, "WindowsSdkDir=" );
		windowsSDKRootFolder.pop_back();	// remove trailing slash

		// add windows sdk lib folders that we need too
		std::string windowsSDKLibFolder = windowsSDKRootFolder + PATH_SEPARATOR + "Lib" + PATH_SEPARATOR + windowsSDKVersion + PATH_SEPARATOR + "um" + PATH_SEPARATOR + "x64";
		g_msvcState->windowsLibPaths.push_back( windowsSDKLibFolder );

		// set PATH environment variable
		SetEnvironmentVariable( "PATH", clPath.data );

		// set include environment variable
		StringBuilder msvcIncludes = {};
		string_builder_reset( &msvcIncludes );
		For ( u64, includeIndex, 0, g_msvcState->windowsIncludes.size() - 1 ) {
			string_builder_appendf( &msvcIncludes, "%s;", g_msvcState->windowsIncludes[includeIndex].c_str() );
		}
		string_builder_appendf( &msvcIncludes, "%s", g_msvcState->windowsIncludes[g_msvcState->windowsIncludes.size() - 1].c_str() );
		const char* includeEnvVar = string_builder_to_string( &msvcIncludes );
		SetEnvironmentVariable( "INCLUDE", includeEnvVar );	// DM!!! 25/07/2025: make an os level wrapper for this

		// set lib path environment variable
		StringBuilder msvcLibs = {};
		string_builder_reset( &msvcLibs );
		For ( u64, libPathIndex, 0, g_msvcState->windowsLibPaths.size() - 1 ) {
			string_builder_appendf( &msvcLibs, "%s;", g_msvcState->windowsLibPaths[libPathIndex].c_str() );
		}
		string_builder_appendf( &msvcLibs, "%s", g_msvcState->windowsLibPaths[g_msvcState->windowsLibPaths.size() - 1].c_str() );
		const char* libsEnvVar = string_builder_to_string( &msvcLibs );
		SetEnvironmentVariable( "LIB", libsEnvVar );	// DM!!! 25/07/2025: make an os level wrapper for this
	}

	s32 exitCode = process_join( vcvarsProcess );

	process_destroy( vcvarsProcess );
	vcvarsProcess = NULL;

	return exitCode == 0;
}

static void MSVC_Shutdown() {
	mem_free( g_msvcState );
	g_msvcState = NULL;
}

static bool8 MSVC_CompileSourceFile( buildContext_t* context, const char* sourceFile ) {
	assert( context );
	assert( sourceFile );

	const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );

	const char* intermediatePath = tprintf( "%s%c%s", context->config.binary_folder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );
	const char* intermediateFilename = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

	procFlags_t procFlags = GetProcFlagsFromBuildContextFlags( context->flags );

	context->config.additional_includes.push_back( "." );

	g_msvcState->includeDependencies.clear();

	g_msvcState->args.reserve(
		1 +	// cl
		1 +	// /c
		1 +	// /std=
		1 +	// /DEBUG
		1 +	// /O*
		1 +	// /Fo:<intermediate filename>
		1 + // /showIncludes
		3 +	// -MD -MF <filename>
		1 +	// source file
		context->config.defines.size() +
		context->config.additional_includes.size() +	// /I <include>
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size() +
		1 +	// /WX
		context->config.warning_levels.size() +
		context->config.ignore_warnings.size()
	);

	g_msvcState->args.reset();

	g_msvcState->args.add( context->compilerPath.data );

	g_msvcState->args.add( "/c" );

	if ( context->config.language_version != LANGUAGE_VERSION_UNSET ) {
		g_msvcState->args.add( LanguageVersionToCompilerArg( context->config.language_version ) );
	}

	if ( !context->config.remove_symbols ) {
		g_msvcState->args.add( "/DEBUG" );
	}

	g_msvcState->args.add( OptimizationLevelToCompilerArg( context->config.optimization_level ) );

	g_msvcState->args.add( tprintf( "/Fo%s", intermediateFilename ) );

	if ( context->flags & BUILD_CONTEXT_FLAG_GENERATE_INCLUDE_DEPENDENCIES ) {
		g_msvcState->args.add( "/showIncludes" );
	}

	g_msvcState->args.add( sourceFile );

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		g_msvcState->args.add( tprintf( "/D%s", context->config.defines[defineIndex].c_str() ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		g_msvcState->args.add( tprintf( "/I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		g_msvcState->args.add( "/WX" );
	}

	// warning levels
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
		if ( context->config.warning_levels.size() > 1 ) {
			StringBuilder builder;
			string_builder_reset( &builder );

			string_builder_appendf( &builder, "MSVC only allows ONE of the following warning levels to be set:\n" );

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				string_builder_appendf( &builder, "%s, ", allowedWarningLevels[allowedWarningLevelIndex].c_str() );
			}

			error( "%s\n", string_builder_to_string( &builder ) );

			return false;
		}

		For ( u64, warningLevelIndex, 0, context->config.warning_levels.size() ) {
			const std::string& warningLevel = context->config.warning_levels[warningLevelIndex];

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

			g_msvcState->args.add( warningLevel.c_str() );
		}
	}

	For ( u64, ignoreWarningIndex, 0, context->config.ignore_warnings.size() ) {
		g_msvcState->args.add( context->config.ignore_warnings[ignoreWarningIndex].c_str() );
	}

	if ( procFlags & PROC_FLAG_SHOW_ARGS ) {
		For ( u64, argIndex, 0, g_msvcState->args.count ) {
			printf( "%s ", g_msvcState->args[argIndex] );
		}
		printf( "\n" );
	}

	// MSVC doesnt output include dependencies to .d files
	// it only supports printing them to stdout
	// so we have to parse the stdout of the process ourselves
	s32 exitCode = 0;
	StringBuilder processStdout = {};
	defer( string_builder_destroy( &processStdout ) );
	{
		u32 processFlags = PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR;
		Process* process = process_create( &g_msvcState->args, NULL, processFlags );

		string_builder_reset( &processStdout );

		if ( procFlags & PROC_FLAG_SHOW_STDOUT ) {
			char buffer[1024] = { 0 };
			u64 bytesRead = U64_MAX;

			while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
				buffer[bytesRead] = 0;

				string_builder_appendf( &processStdout, "%s", buffer );
			}
		}

		exitCode = process_join( process );

		process_destroy( process );
		process = NULL;
	}

	const char* buffer = string_builder_to_string( &processStdout );

	if ( context->flags & BUILD_CONTEXT_FLAG_GENERATE_INCLUDE_DEPENDENCIES ) {
		// now parse the stdout
		// all include dependencies are on their own line
		// the line always starts with a specific prefix
		const char* includeDependencyPrefix = "Note: including file: ";
		const u64 includeDependencyPrefixLength = strlen( includeDependencyPrefix );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		char* lineStart = cast( char*, buffer );
#pragma clang diagnostic pop

		while ( *lineStart ) {
			char* lineEnd = strchr( lineStart, '\n' );

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

				g_msvcState->includeDependencies.push_back( bufferLine );
			} else {
				printf( "%s\n", bufferLine.c_str() );
			}

			lineStart = lineEnd + 1;
		}
	} else {
		printf( "%s", buffer );
	}

	return exitCode == 0;
}

static bool8 MSVC_LinkIntermediateFiles( buildContext_t* context, const Array<const char*>& intermediateFiles ) {
	assert( context );

	const char* fullBinaryName = BuildConfig_GetFullBinaryName( &context->config );

	procFlags_t procFlags = GetProcFlagsFromBuildContextFlags( context->flags );

	g_msvcState->args.reserve(
		1 +	// link
		1 +	// /lib or /shared
		1 +	// /DEBUG
		1 +	// /OUT:<name>
		intermediateFiles.count +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size()
	);

	g_msvcState->args.reset();

	const char* compilerPathOnly = path_remove_file_from_path( context->compilerPath.data );
	if ( compilerPathOnly ) {
		g_msvcState->args.add( tprintf( "%s%c%s", compilerPathOnly, PATH_SEPARATOR, g_msvcBackend.linkerName ) );
	} else {
		g_msvcState->args.add( g_msvcBackend.linkerName );
	}

	if ( context->config.binary_type == BINARY_TYPE_STATIC_LIBRARY ) {
		g_msvcState->args.add( "/lib" );
	} else if ( context->config.binary_type == BINARY_TYPE_DYNAMIC_LIBRARY ) {
		g_msvcState->args.add( "/shared" );
	}

	if ( !context->config.remove_symbols ) {
		g_msvcState->args.add( "/DEBUG" );
	}

	g_msvcState->args.add( tprintf( "/OUT:%s", fullBinaryName ) );

	g_msvcState->args.add_range( &intermediateFiles );

	For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
		g_msvcState->args.add( tprintf( "/LIBPATH:\"%s\"", context->config.additional_lib_paths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
		g_msvcState->args.add( context->config.additional_libs[libIndex].c_str() );
	}

	s32 exitCode = RunProc( &g_msvcState->args, NULL, procFlags );

	return exitCode == 0;
}

static void MSVC_GetIncludeDependenciesFromSourceFileBuild( std::vector<std::string>& outIncludeDependencies ) {
	outIncludeDependencies = g_msvcState->includeDependencies;
}

compilerBackend_t g_msvcBackend = {
	.linkerName									= "link",
	.data										= &g_msvcState,
	.Init										= MSVC_Init,
	.Shutdown									= MSVC_Shutdown,
	.CompileSourceFile							= MSVC_CompileSourceFile,
	.LinkIntermediateFiles						= MSVC_LinkIntermediateFiles,
	.GetIncludeDependenciesFromSourceFileBuild	= MSVC_GetIncludeDependenciesFromSourceFileBuild,
};