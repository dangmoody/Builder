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

#ifdef _WIN64
#define NOMINMAX
#include <Windows.h>
#endif

#include <vector>

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

enum buildContextFlagBits_t {
	BUILD_CONTEXT_FLAG_SHOW_COMPILER_ARGS	= bit( 0 ),
	BUILD_CONTEXT_FLAG_SHOW_STDOUT			= bit( 1 ),
};
typedef u32 buildContextFlags_t;

struct buildContext_t {
	BuildConfig			config;

	// TODO(DM): 10/08/2024: does this want to be inside BuilderOptions?
	// it would give users more control over their build
	buildContextFlags_t	flags;

	const char*			fullBinaryName;

	const char*			inputFile;
	const char*			inputFilePath;

	const char*			dotBuilderFolder;
	const char*			buildInfoFilename;
};

errorCode_t	GetLastErrorCode();

bool8		PathHasSlash( const char* path );
bool8		FileIsSourceFile( const char* filename );
bool8		FileIsHeaderFile( const char* filename );
void		GetAllSubfolders_r( const char* basePath, const char* folder, Array<const char*>* outSubfolders );

const char*	BuildConfig_GetFullBinaryName( const BuildConfig* config );
void		BuildConfig_AddDefaults( BuildConfig* outConfig );

void		BuildInfo_Write( const buildContext_t* context, const std::vector<BuildConfig>& configs, const char* userConfigSourceFilename, const char* userConfigDLLFilename, const bool8 verbose );

bool8		GenerateVisualStudioSolution( buildContext_t* context, BuilderOptions* options, const char* userConfigSourceFilename, const char* userConfigBuildDLLFilename, const bool8 verbose );