/*
===========================================================================

Builder

Distributed under MIT License:
Copyright (c) 2026 Dan Moody

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


CONTENTS:
	1. QUICK START GUIDE


1. Quick Start Guide
Builder can be used to generate a compile_commands.json file (a JSON Compilation Database).
Clangd, CLion, VS Code, and anything else that supports the format can use for code completion and navigation:
https://clang.llvm.org/docs/JSONCompilationDatabase.html

	#define BUILDER_IMPLEMENTATION
	#include "builder.h"

	#define BUILDER_COMPILATION_DATABASE_IMPLEMENTATION
	#include "builder_compilation_database.h"

	int main( int argc, char **argv ) {
		BuilderOptions options = { 0 };

		BuildConfig *config = CreateBuildConfig( &options );
		*config = (BuildConfig) {
			.name			= "my_awesome_program",
			.sourceFiles	= MakeStringList( "src/my_code.c" ),
		};

		options.defaultConfig = config;

		if ( HasCommandLineArg( argc, argv, "--compile-commands" ) ) {
			CompilationDatabaseOptions compilationDatabaseOptions = { 0 };

			return Builder_GenerateCompilationDatabase( &options, &compilationDatabaseOptions, argc, argv ) ? 0 : 1;
		}

		return Build( &options, argc, argv );
	}

You'd then generate the compile_commands.json file with:

	build.exe --compile-commands

By default, if you call Builder_GenerateCompilationDatabase() without filling in any other settings Builder will
generate an entry for every source file of every BuildConfig you created.

Nothing gets compiled - the commands are the same ones Builder would run, minus the output file.

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "builder.h"

typedef struct CompilationDatabaseOptions {
	// Where do you want compile_commands.json to go?
	// Leave NULL to put it in the same folder as where your build script is running from.
	const char		*path;

	// The configs whose source files go into compile_commands.json.
	// Leave empty to default to every BuilderOptions::configs entry.
	ConfigPtrList	configs;
} CompilationDatabaseOptions;

bool	Builder_GenerateCompilationDatabase( BuilderOptions *options, CompilationDatabaseOptions *compilationDatabaseOptions, int argc, char **argv );


#ifdef BUILDER_COMPILATION_DATABASE_IMPLEMENTATION

#if !defined( BUILDER_IMPLEMENTATION )
#error "BUILDER_COMPILATION_DATABASE_IMPLEMENTATION requires BUILDER_IMPLEMENTATION to also be defined, and \"builder.h\" to be included before \"builder_compilation_database.h\", in this translation unit."
#endif

#define COMPILATION_DATABASE_FILENAME	"compile_commands.json"

// escape backslashes and quotes so the string is a valid JSON string
// windows paths and quoted compiler paths are full of both
static const char *Builder_JSONEscapeString( arena_t *arena, const char *string ) {
	size_t length = strlen( string );

	// worst case every char needs escaping
	char *escaped = Builder_ArenaAlloc( arena, char, ( length * 2 ) + 1 );
	char *dst = escaped;

	for ( const char *src = string; *src; src++ ) {
		if ( *src == '\\' || *src == '"' ) {
			*dst++ = '\\';
		}

		*dst++ = *src;
	}

	*dst = '\0';

	return escaped;
}

bool Builder_GenerateCompilationDatabase( BuilderOptions *options, CompilationDatabaseOptions *compilationDatabaseOptions, int argc, char **argv ) {
	BUILDER_ASSERT( options );
	BUILDER_ASSERT( compilationDatabaseOptions );

	Builder_SetCmdArgs( options, argc, argv );

	Builder_SetCWD( options, argv );

	scratch_t scratch = Builder_GetScratch( NULL );

	const char *compilerPath = !Builder_StringIsEmpty( options->compilerPath ) ? options->compilerPath : "clang";

	bool compilerIsMSVC = Builder_StringEquals( compilerPath, "cl" ) || Builder_StringEquals( compilerPath, "cl.exe" );
	bool compilerIsClangCL = Builder_PathEndsWith( compilerPath, "clang-cl" ) || Builder_PathEndsWith( compilerPath, "clang-cl.exe" );

	bool useMSVCSyntax = compilerIsMSVC || compilerIsClangCL;

#if defined( _WIN32 )
	if ( useMSVCSyntax ) {
		if ( !Builder_GetWindowsSDKInstall( scratch.arena, &g_windowsSDKInstall ) || !Builder_GetMSVCInstall( scratch.arena, &g_msvcInstall ) ) {
			Builder_RewindScratch( &scratch );
			return false;
		}

		if ( compilerIsMSVC ) {
			compilerPath = g_msvcInstall.compilerPath;
		}
	}
#elif defined( __linux__ )
	if ( compilerIsMSVC ) {
		Builder_Error(
			"It appears you want to compile with MSVC on a non-Windows platform.\n"
			"MSVC only supports Windows.  Sorry.\n"
		);

		Builder_RewindScratch( &scratch );
		return false;
	}
#else
#error Unrecognised platform.
#endif

	const char *directory = NULL;
	{
		char cwd[BUILDER_MAX_PATH] = { 0 };

#if defined( _WIN32 )
		if ( GetCurrentDirectory( BUILDER_MAX_PATH, cwd ) == 0 ) {
			Builder_Error( "Failed to get the CWD.  GetLastError: 0x%X\n", GetLastError() );
			Builder_RewindScratch( &scratch );
			return false;
		}
#elif defined( __linux__ )
		if ( !getcwd( cwd, BUILDER_MAX_PATH ) ) {
			int err = errno;
			Builder_Error( "Failed to get the CWD.  errno: %d (\"%s\")\n", err, strerror( err ) );
			Builder_RewindScratch( &scratch );
			return false;
		}
#else
#error Unrecognised platform.
#endif

		directory = Builder_JSONEscapeString( scratch.arena, cwd );
	}

	const char *compilationDatabaseFilename = COMPILATION_DATABASE_FILENAME;

	if ( !Builder_StringIsEmpty( compilationDatabaseOptions->path ) ) {
		if ( !Builder_CreateFolderIfItDoesntExist( compilationDatabaseOptions->path ) ) {
			Builder_Error( "Failed to create \"%s\" folder.\n", compilationDatabaseOptions->path );
			Builder_RewindScratch( &scratch );
			return false;
		}

		compilationDatabaseFilename = Builder_FormatString( scratch.arena, "%s%c%s", compilationDatabaseOptions->path, BUILDER_PATH_SEPARATOR, COMPILATION_DATABASE_FILENAME );
	}

	printf( "Generating %s ... ", compilationDatabaseFilename );

	const ConfigPtrList *configs = ( compilationDatabaseOptions->configs.count > 0 ) ? &compilationDatabaseOptions->configs : &options->configs;

	stringBuilder_t content = { 0 };

	StringBuilder_Appendf( scratch.arena, &content, "[\n" );

	bool isFirstEntry = true;

	for ( buildConfigPtrChunk_t *configChunk = configs->head; configChunk; configChunk = configChunk->next ) {
		for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < configChunk->count; chunkConfigIndex++ ) {
			BuildConfig *config = configChunk->items[chunkConfigIndex];

			builderCompileContext_t compileContext = {
				.config			= config,
				.compilerPath	= compilerPath,
				.useMSVCSyntax	= useMSVCSyntax,
			};

			const char *baseCompileCommand = Builder_CreateCompilationCommand( scratch.arena, &compileContext );

			if ( !baseCompileCommand ) {
				Builder_Error( "Failed to create compilation command for config %s!\n", config->name );
				Builder_RewindScratch( &scratch );
				return false;
			}

			StringList sourceFiles = Builder_GlobFiles( scratch.arena, &config->sourceFiles, options );

			for ( builderStringChunk_t *sourceChunk = sourceFiles.head; sourceChunk; sourceChunk = sourceChunk->next ) {
				for ( uint32_t sourceFileIndex = 0; sourceFileIndex < sourceChunk->count; sourceFileIndex++ ) {
					const char *sourceFile = sourceChunk->items[sourceFileIndex];

					const char *command = Builder_FormatString( scratch.arena, "%s%s", baseCompileCommand, sourceFile );

					if ( !isFirstEntry ) {
						StringBuilder_Appendf( scratch.arena, &content, ",\n" );
					}

					StringBuilder_Appendf( scratch.arena, &content, "\t{\n" );
					StringBuilder_Appendf( scratch.arena, &content, "\t\t\"directory\": \"%s\",\n", directory );
					StringBuilder_Appendf( scratch.arena, &content, "\t\t\"file\": \"%s\",\n", Builder_JSONEscapeString( scratch.arena, sourceFile ) );
					StringBuilder_Appendf( scratch.arena, &content, "\t\t\"command\": \"%s\"\n", Builder_JSONEscapeString( scratch.arena, command ) );
					StringBuilder_Appendf( scratch.arena, &content, "\t}" );

					isFirstEntry = false;
				}
			}
		}
	}

	StringBuilder_Appendf( scratch.arena, &content, "\n]\n" );

	uint64_t length;
	char *contentString = StringBuilder_ToString( scratch.arena, &content, &length );

	if ( !Builder_WriteEntireFile( compilationDatabaseFilename, contentString, length ) ) {
		Builder_Error( "Failed to write \"%s\".\n", compilationDatabaseFilename );
		Builder_RewindScratch( &scratch );
		return false;
	}

	printf( "Done\n\n" );

	Builder_RewindScratch( &scratch );

	return true;
}

#endif // BUILDER_COMPILATION_DATABASE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
