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
// can we keep those there without needing this include to do it? header files shouldn't include other header files (unless it's core_types.h or something like that)!
#include "win_support.h"

#include "core/include/core_types.h"
#include "core/include/array.h"
#include "core/include/core_string.h"

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

struct Hashmap;
struct StringBuilder;

enum procFlagBits_t {
	PROC_FLAG_SHOW_ARGS		= bit( 0 ),
	PROC_FLAG_SHOW_STDOUT	= bit( 1 ),
};
typedef u32 procFlags_t;

struct compilationCommandArchetype_t {
	Array<const char *>	baseArgs;
	Array<const char *>	dependencyFlags;
	const char			*outputFlag = nullptr;
};

struct compilerBackend_t {
	void	*data;

	bool8	( *Init )( compilerBackend_t *backend, const buildContext_t *context, const std::string &compilerPath, const std::string &compilerVersion );
	void	( *Shutdown )( compilerBackend_t *backend );
	bool8	( *CompileSourceFile )( compilerBackend_t *backend, buildContext_t *buildContext, BuildConfig *config, compilationCommandArchetype_t &commandArchetype, const char *sourceFile, bool recordCompilation );
	bool8	( *LinkIntermediateFiles )( compilerBackend_t *backend, const Array<const char *> &intermediateFiles, BuildConfig *config );
	bool8	( *GetCompilationCommandArchetype )( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype );
	void	( *GetIncludeDependenciesFromSourceFileBuild )( compilerBackend_t *backend, std::vector<std::string> &includeDependencies );
	String	( *GetCompilerPath )( compilerBackend_t *backend );
	String	( *GetCompilerVersion )( compilerBackend_t *backend );
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
	Hashmap									*configIndices;
	Hashmap									*sourceFileIndices;
	std::vector<includeDependencies_t>		sourceFileIncludeDependencies;

	const char								*inputFile;
	String									inputFilePath;
	String									dotBuilderFolder;

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
int			BuilderMain( const int firstArg, int argc, const char * const * argv );

void		LogVerbose( const char *fmt, ... );

u64			GetLastFileWriteTime( const char *filename );

bool8		NukeFolder( const char *folder, const bool8 deleteRootFolder, const bool8 printDeletions );

const char	*GetNextSlashInPath( const char *path );

bool8		FileIsSourceFile( const char *filename );
bool8		FileIsHeaderFile( const char *filename );

const char	*GetFileExtensionFromBinaryType( const BinaryType type );

const char	*BuildConfig_GetFullBinaryName( const BuildConfig *config );

void		RecordCompilationDatabaseEntry( buildContext_t *buildContext, const char *sourceFileName, const Array<const char *> &compilationCommandArray );

s32			RunProc( Array<const char *> *args, Array<const char *> *environmentVariables, const procFlags_t procFlags = 0, String *outStdout = NULL );

bool8		WriteStringBuilderToFile( StringBuilder *stringBuilder, const char *filename );

bool8		GenerateVisualStudioSolution( buildContext_t *context, BuilderOptions *options );

bool8		GenerateVSCodeJSONFiles( buildContext_t *context, BuilderOptions *options );

inline u64 minull( const u64 x, const u64 y ) {
	return ( x < y ) ? x : y;
}

