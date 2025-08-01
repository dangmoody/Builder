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

struct clangState_t {
	Array<const char*>	args;

	const char*			depFilename;
};

static clangState_t* g_clangState = NULL;

// TODO(DM): 20/07/2025: do we want to ignore this warning via the build script?
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char* LanguageVersionToCompilerArg( const LanguageVersion languageVersion ) {
	assert( languageVersion != LANGUAGE_VERSION_UNSET );

	switch ( languageVersion ) {
		case LANGUAGE_VERSION_C89:		return "-std=c89";
		case LANGUAGE_VERSION_C99:		return "-std=c99";
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

static bool8 Clang_Init() {
	g_clangState = cast( clangState_t*, mem_alloc( sizeof( clangState_t ) ) );
	new( g_clangState ) clangState_t;

	return true;
}

static void Clang_Shutdown() {
	mem_free( g_clangState );
	g_clangState = NULL;
}

static bool8 Clang_CompileSourceFile( buildContext_t* context, const char* sourceFile ) {
	assert( context );
	assert( sourceFile );

	bool8 isClang = string_ends_with( context->compilerPath.data, "clang" ) || string_ends_with( context->compilerPath.data, "clang++" );
	bool8 isGCC = string_ends_with( context->compilerPath.data, "gcc" ) || string_ends_with( context->compilerPath.data, "g++" );

	const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );

	const char* intermediatePath = tprintf( "%s%c%s", context->config.binary_folder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );
	const char* intermediateFilename = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

	g_clangState->depFilename = tprintf( "%s%c%s.d", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

	procFlags_t procFlags = GetProcFlagsFromBuildContextFlags( context->flags );

	g_clangState->args.reserve(
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
		context->config.defines.size() +
		context->config.additional_includes.size() +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size() +
		context->config.warning_levels.size() +
		context->config.ignore_warnings.size()
	);

	g_clangState->args.reset();

	g_clangState->args.add( context->compilerPath.data );

	g_clangState->args.add( "-c" );

	if ( context->config.language_version != LANGUAGE_VERSION_UNSET ) {
		g_clangState->args.add( LanguageVersionToCompilerArg( context->config.language_version ) );
	}

	if ( !context->config.remove_symbols ) {
		g_clangState->args.add( "-g" );
	}

	g_clangState->args.add( OptimizationLevelToCompilerArg( context->config.optimization_level ) );

	g_clangState->args.add( "-o" );
	g_clangState->args.add( intermediateFilename );

	if ( context->flags & BUILD_CONTEXT_FLAG_GENERATE_INCLUDE_DEPENDENCIES ) {
		g_clangState->args.add( "-MMD" );						// generate the dependency file
		g_clangState->args.add( "-MF" );						// set the name of the dependency file to...
		g_clangState->args.add( g_clangState->depFilename );	// ...this
	}

	g_clangState->args.add( sourceFile );

	For ( u32, defineIndex, 0, context->config.defines.size() ) {
		g_clangState->args.add( tprintf( "-D%s", context->config.defines[defineIndex].c_str() ) );
	}

	For ( u32, includeIndex, 0, context->config.additional_includes.size() ) {
		g_clangState->args.add( tprintf( "-I%s", context->config.additional_includes[includeIndex].c_str() ) );
	}

	// must come before ignored warnings
	if ( context->config.warnings_as_errors ) {
		g_clangState->args.add( "-Werror" );
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

			g_clangState->args.add( warningLevel.c_str() );
		}
	}

	For ( u64, ignoreWarningIndex, 0, context->config.ignore_warnings.size() ) {
		g_clangState->args.add( context->config.ignore_warnings[ignoreWarningIndex].c_str() );
	}

	s32 exitCode = RunProc( &g_clangState->args, NULL, procFlags );

	return exitCode == 0;
}

static bool8 Clang_LinkIntermediateFiles( buildContext_t* context, const Array<const char*>& intermediateFiles ) {
	assert( context );

	const char* fullBinaryName = BuildConfig_GetFullBinaryName( &context->config );

	procFlags_t procFlags = GetProcFlagsFromBuildContextFlags( context->flags );

	g_clangState->args.reserve(
		1 + // lld-link
		1 + // /lib or -shared
		1 + // -g
		1 + // -o
		1 + // binary name
		intermediateFiles.count +
		context->config.additional_lib_paths.size() +
		context->config.additional_libs.size()
	);

	g_clangState->args.reset();

	// clang and gcc treat static libraries as just an archive of .o files
	// so there is no real "link" step in this case, the .o files are just "archived" together
	// for dynamic libraries and executables clang and gcc recommend you call the compiler again and just pass in all the intermediate files
	if ( context->config.binary_type == BINARY_TYPE_STATIC_LIBRARY ) {
		g_clangState->args.add( context->linkerPath.data );
		g_clangState->args.add( "/lib" );

		g_clangState->args.add( tprintf( "/OUT:%s", fullBinaryName ) );

		g_clangState->args.add_range( &intermediateFiles );
	} else {
		g_clangState->args.add( context->compilerPath.data );

		if ( context->config.binary_type == BINARY_TYPE_DYNAMIC_LIBRARY ) {
			g_clangState->args.add( "-shared" );
		}

		if ( !context->config.remove_symbols ) {
			g_clangState->args.add( "-g" );
		}

		g_clangState->args.add( "-o" );
		g_clangState->args.add( fullBinaryName );

		g_clangState->args.add_range( &intermediateFiles );

		For ( u32, libPathIndex, 0, context->config.additional_lib_paths.size() ) {
			g_clangState->args.add( tprintf( "-L%s", context->config.additional_lib_paths[libPathIndex].c_str() ) );
		}

		For ( u32, libIndex, 0, context->config.additional_libs.size() ) {
			g_clangState->args.add( tprintf( "-l%s", context->config.additional_libs[libIndex].c_str() ) );
		}
	}

	s32 exitCode = RunProc( &g_clangState->args, NULL, procFlags );

	return exitCode == 0;
}

// only call this after compilation has finished successfully
// parse the dependency file that we generated for every dependency thats in there
// add those to a list - we need to put those in the .build_info file
static void Clang_GetIncludeDependenciesFromSourceFileBuild( std::vector<std::string>& outIncludeDependencies ) {
	const char* depFilename = g_clangState->depFilename;

	char* depFileBuffer = NULL;
	bool8 read = file_read_entire( depFilename, &depFileBuffer );

	if ( !read ) {
		fatal_error( "Failed to read \"%s\".  This should never happen!\n", depFilename );
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
		while ( dependencyEnd && ( *( dependencyEnd - 1 ) == PATH_SEPARATOR ) ) {
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
			if ( dependencyFilename[i] == PATH_SEPARATOR && dependencyFilename[i + 1] == ' ' ) {
				dependencyFilename.erase( i, 1 );
			}
		}

		// get the file timestamp
		FileInfo fileInfo;
		File foundFile = file_find_first( dependencyFilename.c_str(), &fileInfo );
		assert( foundFile.ptr != INVALID_HANDLE_VALUE );
		u64 lastWriteTime = fileInfo.last_write_time;

		//printf( "Parsing dependency %s, last write time = %llu\n", dependencyFilename.c_str(), lastWriteTime );

		outIncludeDependencies.push_back( dependencyFilename.c_str() );

		current = dependencyEnd + 1;

		while ( *current == PATH_SEPARATOR ) {
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

compilerBackend_t g_clangBackend = {
	.linkerName									= "lld-link",
	.data										= NULL,
	.Init										= Clang_Init,
	.Shutdown									= Clang_Shutdown,
	.CompileSourceFile							= Clang_CompileSourceFile,
	.LinkIntermediateFiles						= Clang_LinkIntermediateFiles,
	.GetIncludeDependenciesFromSourceFileBuild	= Clang_GetIncludeDependenciesFromSourceFileBuild,
};

compilerBackend_t g_gccBackend = {
	.linkerName									= "ld",
	.data										= NULL,
	.Init										= Clang_Init,
	.Shutdown									= Clang_Shutdown,
	.CompileSourceFile							= Clang_CompileSourceFile,
	.LinkIntermediateFiles						= Clang_LinkIntermediateFiles,
	.GetIncludeDependenciesFromSourceFileBuild	= Clang_GetIncludeDependenciesFromSourceFileBuild,
};