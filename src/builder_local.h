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

#include "../builder.h"

#include "core/include/core_types.h"
#include "core/include/array.h"
#include "core/include/core_string.h"

#ifdef _WIN64
#define NOMINMAX
#include <Windows.h>
#endif

#include <vector>
//#include <string>

#define ARG_HELP_SHORT		"-h"
#define ARG_HELP_LONG		"--help"
#define ARG_VERBOSE_SHORT	"-v"
#define ARG_VERBOSE_LONG	"--verbose"
#define ARG_NUKE			"--nuke"
#define ARG_CONFIG			"--config="

#define BUILD_INFO_FILE_EXTENSION	".build_info"

#ifdef _WIN64
#define ERROR_CODE_FORMAT "0x%X"
typedef DWORD errorCode_t;
#else
#error Unrecognised platform!
#endif

struct Hashmap;
struct buildInfoData_t;

enum buildContextFlagBits_t {
	BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS					= bit( 0 ),
	BUILD_CONTEXT_FLAG_SHOW_STDOUT							= bit( 1 ),
};
typedef u32 buildContextFlags_t;

struct buildContext_t {
	BuildConfig			config;

	Hashmap*			configIndices;

	std::vector<std::vector<std::string>>	includeDependencies;

	// TODO(DM): 10/08/2024: does this want to be inside BuilderOptions?
	// it would give users more control over their build
	buildContextFlags_t	flags;

	String				compilerPath;

	const char*			inputFile;
	String				inputFilePath;

	String				dotBuilderFolder;
	String				buildInfoFilename;

	bool8				verbose;
};

errorCode_t	GetLastErrorCode();

void		NukeFolder_r( const char* folder, const bool8 deleteRoot, const bool8 verbose );

const char*	GetSlashInPath( const char* path );
//bool8		PathHasSlash( const char* path );

bool8		FileIsSourceFile( const char* filename );
bool8		FileIsHeaderFile( const char* filename );
//void		GetAllSubfolders_r( const char* basePath, const char* folder, Array<const char*>* outSubfolders );

const char*	BuildConfig_GetFullBinaryName( const BuildConfig* config );
void		BuildConfig_AddDefaults( BuildConfig* outConfig );


bool8		GenerateVisualStudioSolution( buildContext_t* context, BuilderOptions* options );

inline u64 minull( const u64 x, const u64 y ) {
	return ( x < y ) ? x : y;
}