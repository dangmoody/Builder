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

#pragma once

#include "../include/builder.h"

// TODO(DM): 01/04/2026: we only need this is because buildContext_t needs windowsSDK_t and msvcInstall_t
// can we keep those there without needing this include to do it? header files shouldn't include other header files (unless it's int_types.h or something like that)!
#include "win_support.h"

#include "int_types.h"
#include "array.h"
#include "string.h"

#ifdef _WIN64
#define NOMINMAX
#include <Windows.h>
#endif

#include <vector>
//#include <string>

// cmd line args
#define ARG_HELP_SHORT			"-h"
#define ARG_HELP_LONG			"--help"
#define ARG_VERBOSE_SHORT		"-v"
#define ARG_VERBOSE_LONG		"--verbose"
#define ARG_NUKE				"--nuke"
#define ARG_CONFIG				"--config="
#define ARG_VISUAL_STUDIO_BUILD	"--visual-studio-build"


struct buildContext_t;

struct hashmap_t;
struct stringBuilder_t;
struct linearAllocator_t;


// memory conversion helpers
#define MEM_KILOBYTES( x )	( Cast( u64, (x) ) * 1000 )
#define MEM_MEGABYTES( x )	( MEM_KILOBYTES( x ) * 1000 )
#define MEM_GIGABYTES( x )	( MEM_MEGABYTES( x ) * 1000 )

enum procFlagBits_t {
	PROC_FLAG_SHOW_ARGS		= BIT( 0 ),
	PROC_FLAG_SHOW_STDOUT	= BIT( 1 ),
};
typedef u32 procFlags_t;

struct compilationCommandArchetype_t {
	array_t<const char *>	baseArgs;
	array_t<const char *>	dependencyFlags;
	const char				*outputFlag = nullptr;
};

struct compilerBackend_t {
	void		*data;

	bool8		( *Init )( compilerBackend_t *backend, const buildContext_t *context, const char *compilerPath, const char *compilerVersion );
	void		( *Shutdown )( compilerBackend_t *backend );
	bool8		( *CompileSourceFile )( compilerBackend_t *backend, buildContext_t *buildContext, BuildConfig *config, compilationCommandArchetype_t &commandArchetype, const char *sourceFile, bool recordCompilation, u64 sourceFileIndex, std::vector<std::string> *outIncludeDependencies );
	bool8		( *LinkIntermediateFiles )( compilerBackend_t *backend, const std::vector<std::string> &intermediateFiles, BuildConfig *config, const BuilderOptions *options );
	bool8		( *GetCompilationCommandArchetype )( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype );
	string_t	( *GetCompilerPath )( compilerBackend_t *backend );
	string_t	( *GetCompilerVersion )( compilerBackend_t *backend );
};

void	CreateCompilerBackend_Clang( compilerBackend_t *outBackend );
void	CreateCompilerBackend_MSVC( compilerBackend_t *outBackend );
void	CreateCompilerBackend_GCC( compilerBackend_t *outBackend );

struct includeDependencies_t {
	std::string					filename;
	std::vector<std::string>	includeDependencies;
};

struct compilationDatabaseEntry_t {
	std::vector<std::string>	arguments;
	std::string					directory;
	std::string					file;
	std::string					outputFile;
};

struct buildContext_t {
	linearAllocator_t						*allocator;

	hashmap_t								*configIndices;
	hashmap_t								*sourceFileIndices;
	std::vector<includeDependencies_t>		sourceFileIncludeDependencies;

	const char								*inputFile;
	string_t								inputFilePath;
	string_t								dotBuilderFolder;
	string_t								includeDependenciesFilename;

	bool8									forceRebuild;
	bool8									consolidateCompilerArgs;
	std::vector<compilationDatabaseEntry_t>	compilationDatabase;

#ifdef _WIN32
	windowsSDK_t							winSDK;
	msvcInstall_t							msvcInstall;
#endif
};

extern bool8	g_verbose;

// shared entry point
// used in the actual builder program
// also used by tests so they dont have to start a separate subprocess to build
int						BuilderMain( const int firstArg, int argc, const char * const * argv );

void					LogVerbose( const char *fmt, ... );

u64						GetLastFileWriteTime( const char *filename );

bool8					NukeFolder( const char *folder, const bool8 deleteRootFolder, const bool8 printDeletions );

const char				*GetNextSlashInPath( const char *path );

bool8					FileIsSourceFile( const char *filename );
bool8					FileIsHeaderFile( const char *filename );

const char				*GetFileExtensionFromBinaryType( const BinaryType type );

const char				*BuildConfig_GetFullBinaryName( const BuildConfig *config, linearAllocator_t *allocator );

void					RecordCompilationDatabaseEntry( buildContext_t *buildContext, const char *sourceFileName, const array_t<const char *> &compilationCommandArray, u64 sourceFileIndex );

s32						RunProc( array_t<const char *> *args, array_t<const char *> *environmentVariables, const procFlags_t procFlags = 0, string_t *outStdout = NULL );

bool8					WriteStringBuilderToFile( stringBuilder_t *stringBuilder, const char *filename );

bool8					PathMatchesFilter( const string_t* filename, const string_t* filter );

std::vector<std::string> GetSourceFilesMatchingPattern( const string_t* basePath, const string_t* folderPattern, const string_t* filePattern );

bool8					GenerateVisualStudioSolution( buildContext_t *context, BuilderOptions *options );

bool8					GenerateVSCodeJSONFiles( buildContext_t *context, BuilderOptions *options );

bool8					GenerateZedJSONFiles( buildContext_t *context, BuilderOptions *options );

inline const char *LanguageVersionToString( const LanguageVersion version ) {
	switch ( version ) {
		case LANGUAGE_VERSION_UNSET:	return NULL;
		case LANGUAGE_VERSION_C89:		return "c89";
		case LANGUAGE_VERSION_C99:		return "c99";
		case LANGUAGE_VERSION_C11:		return "c11";
		case LANGUAGE_VERSION_C17:		return "c17";
		case LANGUAGE_VERSION_C23:		return "c23";
		case LANGUAGE_VERSION_CPP11:	return "c++11";
		case LANGUAGE_VERSION_CPP14:	return "c++14";
		case LANGUAGE_VERSION_CPP17:	return "c++17";
		case LANGUAGE_VERSION_CPP20:	return "c++20";
		case LANGUAGE_VERSION_CPP23:	return "c++23";
	}

	return NULL;
}

