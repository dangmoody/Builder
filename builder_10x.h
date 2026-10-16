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
Builder can be used to generate a 10X workspace (.10x) file:

	#define BUILDER_IMPLEMENTATION
	#include "builder.h"

	#define BUILDER_10X_IMPLEMENTATION
	#include "builder_10x.h"

	int main( int argc, char **argv ) {
		BuilderOptions options = { 0 };

		BuildConfig *debug = CreateBuildConfig( &options );
		*debug = (BuildConfig) {
			.name			= "Debug",
			.binaryType		= BINARY_TYPE_EXE,
			.binaryName		= "my_awesome_program",
			.binaryFolder	= "bin/debug",
			.sourceFiles	= MakeStringList( "src/main.c" ),
			.defines		= MakeStringList( "MY_DEBUG" ),
		};

		BuildConfig *release = CreateBuildConfig( &options );
		*release = (BuildConfig) {
			.name			= "Release",
			.binaryType		= BINARY_TYPE_EXE,
			.binaryName		= "my_awesome_program",
			.binaryFolder	= "bin/release",
			.sourceFiles	= MakeStringList( "src/main.c" ),
			.defines		= MakeStringList( "MY_RELEASE" ),
			.optimization	= OPTIMIZATION_PROGRAM_SPEED,
			.removeSymbols	= true,
		};

		options.defaultConfig = debug;

		if ( HasCommandLineArg( argc, argv, "--10x" ) ) {
			TenXPlatform platforms[] = {
				{ .name = "x64",	.defines = MakeStringList( "MY_PLATFORM_X64" ) },
				{ .name = "Win32",	.defines = MakeStringList( "MY_PLATFORM_WIN32" ) },
			};

			TenXConfig configs[] = {
				{
					.name					= "Debug",
					.config					= debug,
					.additionalBuildArgs	= MakeStringList( "--config=Debug" ),
					.runArgs				= "--verbose",
				},
				{
					.name					= "Release",
					.config					= release,
					.additionalBuildArgs	= MakeStringList( "--config=Release" ),
					.prioritiseFiles		= MakeStringList( "src/main.c" ),
					.binaryName				= "my_awesome_program_shipping",
					.binaryRelativePath		= "bin/shipping",
					.runCommand				= "$(Executable) --fullscreen",
					.cleanCommand			= "\"scripts/clean shipping.bat\" --all",
				},
			};

			TenXWorkspace workspace = {
				.name						= "my_awesome_program",
				.configs					= configs,
				.configsCount				= BUILDER_COUNT_OF( configs ),
				.platforms					= platforms,
				.platformCount				= BUILDER_COUNT_OF( platforms ),
				.globalDefines				= MakeStringList( "MY_GLOBAL_DEFINE" ),
				.includeFilterList			= MakeStringList( "*.c", "*.h" ),
				.excludeFilterList			= MakeStringList( "*.obj", "*.pdb" ),
				.syncFiles					= true,
				.recursive					= true,
				.captureExeOutput			= true,
				.allowAutoErrorUnderlining	= true,
			};

			return Builder_Generate10xWorkspace( &options, &workspace, argc, argv ) ? 0 : 1;
		}

		return Build( &options, argc, argv );
	}

You'd then generate the workspace file with:

	build.exe --10x

That writes "my_awesome_program.10x" next to your build exe, with one entry for every config and platform pair (Debug:x64, Debug:Win32, Release:x64, Release:Win32).
Each entry builds by running your build exe with that config's additionalBuildArgs, and runs/debugs config->binaryFolder + config->binaryName.
Set binaryName and binaryRelativePath on a TenXConfig to run/debug a different program instead (like Release does above), and runCommand/runArgs to change how 10X runs it.
The default build command wraps your build exe's path in quotes, so it still works if the path has spaces in it.
Commands you set yourself (buildCommand, rebuildCommand, cleanCommand, etc.) are written as is, so quote any paths with spaces in them yourself (like Release's cleanCommand above).

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "builder.h"

#if defined( _WIN32 )
#include <windows.h>
#elif defined( __linux__ )
#include <limits.h>
#include <stdlib.h>
#endif

// A platform in the workspace's platform dropdown (e.g. "x64" or "ARM64").
// defines are the preprocessor defines 10X uses for this platform.
typedef struct TenXPlatform {
	// The name of the platform as it appears in 10X. E.g. "x64".
	const char	*name;

	// Defines 10X's parser uses when this platform is selected, on top of TenXWorkspace::globalDefines.
	StringList	defines;
} TenXPlatform;

// A config in the workspace's config dropdown (e.g. "Debug" or "Release").
// Each one wraps a BuildConfig and sets the build, run, and debug commands 10X uses for it.
typedef struct TenXConfig {
	// The name of the config as it appears in 10X. E.g. "Debug".
	// Must be unique across TenXWorkspace::configs, but doesn't have to match BuildConfig::name.
	const char	*name;

	// The BuildConfig this entry builds.
	BuildConfig	*config;

	// Added to the end of the default BuildCommand.
	// Ignored if you set buildCommand yourself.
	StringList	additionalBuildArgs;

	// Defines 10X's parser uses for this config only, on top of config->defines and TenXWorkspace::globalDefines.
	// These only affect 10X's parsing, they don't get passed to the compiler.
	StringList	defines;

	// ---- Builder has defaults for these ----

	// Leave NULL to use TenXWorkspace::buildCommand, or your build exe if that's NULL too.
	// additionalBuildArgs are appended either way.
	// Written as is, so wrap any paths with spaces in them in quotes yourself.
	const char	*buildCommand;

	// Leave NULL for the full path to the folder your build exe is in.
	const char	*buildWorkingDirectory;

	// The name (without extension) of the program 10X runs and debugs. Only used for BINARY_TYPE_EXE configs.
	// Leave NULL to use config->binaryName.
	const char	*binaryName;

	// The folder the program 10X runs and debugs is in, relative to your build exe. Only used for BINARY_TYPE_EXE configs.
	// Leave NULL to use config->binaryFolder.
	// The Exe written to the workspace is the full path to this folder + binaryName + extension.
	const char	*binaryRelativePath;

	// The CWD of the program when 10X runs it.
	// Leave NULL for the folder your build exe is in.
	const char	*workingDirectory;

	// The command 10X runs when you run the program (10X's "Run Command" setting).
	// Leave NULL for: $(Executable)
	const char	*runCommand;

	// Command line arguments passed to the program when 10X runs it, as a single string. E.g. "--verbose --fullscreen".
	const char	*runArgs;

	// Leave NULL to use TenXWorkspace::debuggerExe.
	const char	*debuggerExe;

	// The Visual Studio solution 10X opens when debugging with Visual Studio.
	// Leave NULL to use TenXWorkspace::debugSln.
	const char	*debugSln;

	// ---- Left empty in the workspace file if NULL ----
	// The commands here are written as is, so wrap any paths with spaces in them in quotes yourself.

	// The command 10X runs when you rebuild. additionalBuildArgs are appended to it.
	const char	*rebuildCommand;

	// The command 10X runs when you clean.
	const char	*cleanCommand;

	// The command 10X runs to build just the current file. additionalBuildArgs are appended to it.
	const char	*buildFileCommand;

	// The command 10X runs to syntax check the current file.
	const char	*fileSyntaxCheckCommand;

	// The command 10X runs to cancel a build that is in progress.
	const char	*cancelBuild;

	// Headers 10X's parser treats as included at the top of every file.
	StringList	forceIncludes;

	// Files 10X parses before everything else.
	StringList	prioritiseFiles;

} TenXConfig;

// Everything that goes into the generated .10x workspace file.
// Pass this to Builder_Generate10xWorkspace().
typedef struct TenXWorkspace {

	// Workspace filename, without extension. Defaults to BuilderOptions::defaultConfig->name if empty.
	const char		*name;

	// Destination folder for the workspace file in disk. Defaults to the same folder as your build executable if empty.
	const char		*path;

	// The command 10X runs to build the project. If empty, builder will default to run your build exe.
	// Written as is, so wrap any paths with spaces in them in quotes yourself.
	const char		*buildCommand;

	// The debugger 10X uses for every config that doesn't set TenXConfig::debuggerExe.
	// Leave NULL to use 10X's default debugger.
	const char		*debuggerExe;

	// The Visual Studio solution 10X opens when debugging, for every config that doesn't set TenXConfig::debugSln.
	const char		*debugSln;

	// The folder 10X's terminal opens in.
	const char		*terminalDirectory;

	// Keep the workspace's file list in sync with the files on disk.
	bool			syncFiles;

	// Include files in subfolders, not just the top level.
	bool			recursive;

	// Show folders in the workspace tree even if no files in them match the filters.
	bool			showEmptyFolders;

	// Include files with no extension, which includeFilterList can't match.
	bool			includeFilesWithoutExtension;

	// Treat the workspace as a plain folder instead of a project.
	bool			isFolder;

	// Run the Visual Studio environment batch file (vcvars) before build commands, so MSVC is on the PATH.
	bool			useVisualStudioEnvBat;

	// Show your program's output in 10X's output panel when it runs.
	bool			captureExeOutput;

	// Underline errors in the editor using the build output.
	bool			allowAutoErrorUnderlining;

	// Platforms in the workspace's platform dropdown.
	// Leave NULL (and platformCount at 0) for "x64" and "Win32".
	TenXPlatform	*platforms;
	uint32_t		platformCount;

	// Configs in the workspace's config dropdown.
	// One <ConfigAndPlatform> entry is written for every config and platform pair.
	TenXConfig		*configs;
	uint32_t		configsCount;

	// Defines 10X's parser uses for every config and platform.
	// Builder always adds BUILDER_IMPLEMENTATION, BUILDER_10X_IMPLEMENTATION, and the defines for your OS and compiler.
	StringList		globalDefines;

	// Wildcards for the files to include in the workspace. E.g. "*.c".
	// Leave empty to use tenXDefaultIncludeFilter.
	StringList		includeFilterList;

	// Wildcards for the files to exclude from the workspace.
	StringList		excludeFilterList;

} TenXWorkspace;

bool Builder_Generate10xWorkspace( BuilderOptions *options, TenXWorkspace *workspace, int argc, char **argv );

#ifdef BUILDER_10X_IMPLEMENTATION

#if !defined( BUILDER_IMPLEMENTATION )
#error "BUILDER_10X_IMPLEMENTATION requires BUILDER_IMPLEMENTATION to also be defined, and \"builder.h\" to be included before \"builder_10x.h\", in this translation unit."
#endif

static const char *Builder_TenXGetFullCWD( arena_t *arena ) {
	char cwd[BUILDER_MAX_PATH] = { 0 };
#if defined( _WIN32 )
	if ( GetCurrentDirectory( BUILDER_MAX_PATH, cwd ) == 0 ) {
		Builder_Error( "Failed to get the CWD.  GetLastError: 0x%X\n", GetLastError() );
		return NULL;
	}
#elif defined( __linux__ )
	if ( !getcwd( cwd, BUILDER_MAX_PATH ) ) {
		int err = errno;
		Builder_Error( "Failed to get the CWD.  errno: %d (\"%s\")\n", err, strerror( err ) );
		return NULL;
	}
#else
	#error Unrecognised platform.
#endif

	return Builder_FormatString( arena, "%s", cwd );
}

static const char *Builder_TenXStringOrEmpty( const char *string ) {
	return Builder_StringIsEmpty( string ) ? "" : string;
}

static const char *Builder_TenXBoolToString( bool value ) {
	return value ? "True" : "False";
}

static const char *Builder_TenXFirstNonEmpty( const char *string, const char *fallback ) {
	return !Builder_StringIsEmpty( string ) ? string : fallback;
}

// Appends prefix + item + suffix for every item in list.
static void Builder_TenXAppendStringList( arena_t *arena, stringBuilder_t *builder, const StringList *list, const char *prefix, const char *suffix ) {
	for ( builderStringChunk_t *chunk = list->head; chunk; chunk = chunk->next ) {
		for ( uint32_t itemIndex = 0; itemIndex < chunk->count; itemIndex++ ) {
			StringBuilder_Appendf( arena, builder, "%s%s%s", prefix, chunk->items[itemIndex], suffix );
		}
	}
}

// Appends one <itemTag>item</itemTag> line per item in list, at the given indent.
static void Builder_TenXAppendListItems( arena_t *arena, stringBuilder_t *builder, const char *indent, const char *itemTag, const StringList *list ) {
	const char *prefix = Builder_FormatString( arena, "%s<%s>", indent, itemTag );
	const char *suffix = Builder_FormatString( arena, "</%s>\n", itemTag );

	Builder_TenXAppendStringList( arena, builder, list, prefix, suffix );
}

// Appends <tag> with one <itemTag> per item in list inside it, or just <tag></tag> if list is empty.
static void Builder_TenXAppendListTag( arena_t *arena, stringBuilder_t *builder, const char *indent, const char *tag, const char *itemTag, const StringList *list ) {
	if ( list->count == 0 ) {
		StringBuilder_Appendf( arena, builder, "%s<%s></%s>\n", indent, tag, tag );
		return;
	}

	StringBuilder_Appendf( arena, builder, "%s<%s>\n", indent, tag );
	Builder_TenXAppendListItems( arena, builder, Builder_FormatString( arena, "%s\t", indent ), itemTag, list );
	StringBuilder_Appendf( arena, builder, "%s</%s>\n", indent, tag );
}

// Turns relativePath into a full path, relative to the CWD.
// The folder doesn't have to exist yet - binary folders usually won't until the first build.
static bool Builder_TenXResolveRelativePath( const char *relativePath, char *outPath, size_t outPathSize ) {
	if ( Builder_StringIsEmpty( relativePath ) ) {
		relativePath = ".";
	}

#if defined( _WIN32 )
	DWORD length = GetFullPathNameA( relativePath, (DWORD) outPathSize, outPath, NULL );
	return length > 0 && length < outPathSize;
#elif defined( __linux__ )
	// realpath() fails if the path doesn't exist, so fall back to sticking the CWD on the front ourselves
	char resolvedPath[PATH_MAX];
	if ( realpath( relativePath, resolvedPath ) ) {
		int length = snprintf( outPath, outPathSize, "%s", resolvedPath );
		return length > 0 && (size_t) length < outPathSize;
	}

	if ( relativePath[0] == '/' ) {
		int length = snprintf( outPath, outPathSize, "%s", relativePath );
		return length > 0 && (size_t) length < outPathSize;
	}

	char cwd[PATH_MAX];
	if ( !getcwd( cwd, PATH_MAX ) ) {
		return false;
	}

	int length = snprintf( outPath, outPathSize, "%s/%s", cwd, relativePath );
	return length > 0 && (size_t) length < outPathSize;
#else
	#error Unrecognised platform.
#endif
}

bool Builder_Generate10xWorkspace( BuilderOptions *options, TenXWorkspace *workspace, int argc, char **argv ) {
	BUILDER_ASSERT( options );

	Builder_SetCmdArgs( options, argc, argv );
	Builder_SetCWD( argv );

	if ( !Builder_SelfRebuild( options, argc, argv ) ) {
		return false;
	}

	// initial workspace validation.
	bool validWorkspace = true;
	{
		if ( workspace->configsCount < 1 ) {
			Builder_Error( "You must setup at least one config (TenXConfig) when generating a 10X Workspace (TenXWorkspace).\n" );
			validWorkspace = false;
		}

		for ( uint32_t configIndex = 0; configIndex < workspace->configsCount; configIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[configIndex];

			if ( Builder_StringIsEmpty( tenXConfig->name ) ) {
				Builder_Error( "Config %u for the 10X Workspace doesn't have a name set.  You need to fill that in.\n", configIndex );
				validWorkspace = false;
			} else if ( !tenXConfig->config ) {
				Builder_Error( "Config \"%s\" for the 10X Workspace doesn't have a BuildConfig set.  You need to fill that in.\n", tenXConfig->name );
				validWorkspace = false;
			}

			if ( !Builder_StringIsEmpty( tenXConfig->name ) ) {
				for ( uint32_t otherIndex = 0; otherIndex < configIndex; otherIndex++ ) {
					if ( !Builder_StringIsEmpty( workspace->configs[otherIndex].name ) && Builder_StringEquals( workspace->configs[otherIndex].name, tenXConfig->name ) ) {
						Builder_Error( "10X Workspace has more than one config named \"%s\".  Config names must be unique.\n", tenXConfig->name );
						validWorkspace = false;
						break;
					}
				}
			}
		}

		// the workspace filename falls back to the default config's name. I know Builder doesn't allow empty config names, but
		// we could potentially execute 10X's workspace generation before the build step?
		if ( Builder_StringIsEmpty( workspace->name ) && ( !options->defaultConfig || Builder_StringIsEmpty( options->defaultConfig->name ) ) ) {
			Builder_Error( "The 10X Workspace doesn't have a name, and there is no BuilderOptions::defaultConfig with a name to fall back to.  You need to set one of them.\n" );
			validWorkspace = false;
		}

		if ( !validWorkspace ) {
			return false;
		}
	}

	if ( workspace->platformCount < 1 ) {
		// default platforms 10X generates
		static TenXPlatform defaultPlatforms[] = {
			{ .name = "x64" },
			{ .name = "Win32" }
		};

		workspace->platforms = defaultPlatforms;
		workspace->platformCount = BUILDER_COUNT_OF( defaultPlatforms );

		Builder_Warning( "No platforms defined for 10X Workspace. Appended default 'x64' and 'Win32' platforms.\n" );
	}

	scratch_t scratch = Builder_GetScratch( NULL );

	const char *workspaceName = Builder_FormatString( scratch.arena, "%s.10x", !Builder_StringIsEmpty( workspace->name ) ? workspace->name : options->defaultConfig->name );

	// no path means next to the build exe, which Builder_SetCWD() just made the CWD
	const char *workspaceFilename = workspaceName;

	if ( !Builder_StringIsEmpty( workspace->path ) ) {
		if ( !Builder_CreateFolderIfItDoesntExist( workspace->path ) ) {
			Builder_Error( "Failed to create \"%s\" folder.\n", workspace->path );
			Builder_RewindScratch( &scratch );
			return false;
		}

		workspaceFilename = Builder_FormatString( scratch.arena, "%s%c%s", workspace->path, BUILDER_PATH_SEPARATOR, workspaceName );
	}

	arena_t *arena = scratch.arena;
	stringBuilder_t workspaceFileContent = { 0 };

	StringBuilder_Appendf( arena, &workspaceFileContent, "<?xml version=\"1.0\"?>\n" );
	StringBuilder_Appendf( arena, &workspaceFileContent, "<N10X>\n" );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t<Workspace>\n" );

	// include filter
	if ( workspace->includeFilterList.count > 0 ) {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<IncludeFilter>" );
		Builder_TenXAppendStringList( arena, &workspaceFileContent, &workspace->includeFilterList, "", "," );
		StringBuilder_Appendf( arena, &workspaceFileContent, "</IncludeFilter>\n" );
	} else {
		static const char *tenXDefaultIncludeFilter = "*.c,*.cc,*.cpp,*.c++,*.cp,*.cxx,*.h,*.hh,*.hpp,*.h++,*.hp,*.hxx,*.inl,*.cs,*.rs,*.java,*.jav,*.js,*.jsc,*.jsx,*.json,*.cls,*.ts,*.py,*.rpy,*.php,*.php3,*.phl,*.phtml,*.rhtml,*.tpl,*.phps,*.lua,*.html,*.html5,*.htm,*.xml,*.xaml,*.css,*.ssi,*.haml,*.yaml,*.bat,*.wbf,*.wbt,*.txt,*.cmake,*.make,*.makefile,*.mak,*.mk,*.sh,*.bash,*.csv,*.asp,*.pl,*.mac,*.ws,*.vbs,*.perl,*.src,*.rss,*.inc,*.f,*.go,*.prl,*.plx,*.rb,*.lsp,*.lpx,*.ps1,*.command,*.cbl,*.cob,*.qs,*.wxs,*.ph,*.msc,*.glsl,*.hlsl,*.fx,*.vert,*.tesc,*.tese,*.geom,*.frag,*.comp,*.pssl,*.scons,*.cu,*.jai,*.md,";
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<IncludeFilter>%s</IncludeFilter>\n", tenXDefaultIncludeFilter );
	}

	// exclude filter
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<ExcludeFilter>" );
	Builder_TenXAppendStringList( arena, &workspaceFileContent, &workspace->excludeFilterList, "", "," );
	StringBuilder_Appendf( arena, &workspaceFileContent, "</ExcludeFilter>\n" );

	// top-level settings
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<SyncFiles>%s</SyncFiles>\n", Builder_TenXBoolToString( workspace->syncFiles ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<Recursive>%s</Recursive>\n", Builder_TenXBoolToString( workspace->recursive ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<ShowEmptyFolders>%s</ShowEmptyFolders>\n", Builder_TenXBoolToString( workspace->showEmptyFolders ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<IncludeFilesWithoutExt>%s</IncludeFilesWithoutExt>\n", Builder_TenXBoolToString( workspace->includeFilesWithoutExtension ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<IsFolder>%s</IsFolder>\n", Builder_TenXBoolToString( workspace->isFolder ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<UseVisualStudioEnvBat>%s</UseVisualStudioEnvBat>\n", Builder_TenXBoolToString( workspace->useVisualStudioEnvBat ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<CaptureExeOutput>%s</CaptureExeOutput>\n", Builder_TenXBoolToString( workspace->captureExeOutput ) );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<AllowAutoErrorUnderlining_v3>%s</AllowAutoErrorUnderlining_v3>\n", Builder_TenXBoolToString( workspace->allowAutoErrorUnderlining ) );

	// config names
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<Configurations>\n" );
	for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Configuration>%s</Configuration>\n", workspace->configs[tenXConfigIndex].name );
	}
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t</Configurations>\n" );

	// platforms
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<Platforms>\n" );
	for ( uint32_t platformIndex = 0; platformIndex < workspace->platformCount; platformIndex++ ) {
		TenXPlatform *platform = &workspace->platforms[platformIndex];

		if ( !Builder_StringIsEmpty( platform->name ) ) {
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Platform>%s</Platform>\n", platform->name );
		}
	}
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t</Platforms>\n" );

	// additional include paths
	if ( options->configs.count > 0 ) {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<AdditionalIncludePaths>\n" );
		for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
			for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
				Builder_TenXAppendListItems( arena, &workspaceFileContent, "\t\t\t", "AdditionalIncludePath", &chunk->items[configIndex]->additionalIncludes );
			}
		}

#ifdef _WIN32
		if( Builder_GetMSVCInstall( arena, &g_msvcInstall ) ) {
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_msvcInstall.includePath );
		}

		if ( Builder_GetWindowsSDKInstall( arena, &g_windowsSDKInstall ) ) {
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.ucrtIncludePath );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.umIncludePath );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.sharedIncludePath );
		}
#endif

		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t</AdditionalIncludePaths>\n" );
	} else {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<AdditionalIncludePaths></AdditionalIncludePaths>\n" );
	}

	// global defines
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<Defines>\n" );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>BUILDER_IMPLEMENTATION</Define>\n" );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>BUILDER_10X_IMPLEMENTATION</Define>\n" );

#ifdef _WIN32
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>_WIN32</Define>\n" );
#elif __linux__
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>__linux__</Define>\n" );
#endif

#if defined(__clang__)
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>__clang__</Define>\n" );
#elif defined(__GNUC__)
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>__GNUC__</Define>\n" );
#elif defined(__GNUG__)
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>__GNUG__</Define>\n" );
#elif defined(_MSC_VER)
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Define>_MSC_VER</Define>\n" );
#endif

	Builder_TenXAppendListItems( arena, &workspaceFileContent, "\t\t\t", "Define", &workspace->globalDefines );
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t</Defines>\n" );

	// config properties
	if ( workspace->configsCount > 0 ) {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<ConfigProperties>\n" );

		// the build exe is the build command if neither the config nor the workspace set one
		char exePath[BUILDER_MAX_PATH] = { 0 };
		Builder_GetExePath( argv, exePath );

		// quoted because paths can have spaces in them
		// a buildCommand set by the user is a whole command line, so that's written as is
		const char *defaultBuildCommand = Builder_FormatString( arena, "\"%s\"", exePath );

		const char *fullCWD = Builder_TenXGetFullCWD( arena );

		for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[tenXConfigIndex];

			const char *buildCommand = Builder_TenXFirstNonEmpty( tenXConfig->buildCommand, Builder_TenXFirstNonEmpty( workspace->buildCommand, defaultBuildCommand ) );

			// appended to the build, rebuild, and build file commands
			stringBuilder_t additionalBuildArgsBuilder = { 0 };
			Builder_TenXAppendStringList( arena, &additionalBuildArgsBuilder, &tenXConfig->additionalBuildArgs, " ", "" );
			const char *additionalBuildArgs = Builder_TenXStringOrEmpty( StringBuilder_ToString( arena, &additionalBuildArgsBuilder, NULL ) );

			// the rebuild and build file commands are left empty if not set, so don't append additionalBuildArgs to nothing
			const char *rebuildArgs = !Builder_StringIsEmpty( tenXConfig->rebuildCommand ) ? additionalBuildArgs : "";
			const char *buildFileArgs = !Builder_StringIsEmpty( tenXConfig->buildFileCommand ) ? additionalBuildArgs : "";

			// the Exe tag is only written for programs 10X can actually run
			const char *exe = NULL;
			if ( tenXConfig->config->binaryType == BINARY_TYPE_EXE ) {
				const char *binaryName = Builder_TenXFirstNonEmpty( tenXConfig->binaryName, tenXConfig->config->binaryName );
				const char *binaryFolder = Builder_TenXFirstNonEmpty( tenXConfig->binaryRelativePath, tenXConfig->config->binaryFolder );

				char binaryFolderPath[BUILDER_MAX_PATH] = { 0 };
				if ( !Builder_TenXResolveRelativePath( binaryFolder, binaryFolderPath, BUILDER_MAX_PATH ) ) {
					Builder_Error( "Failed to get the full path of binary folder \"%s\" for 10X config \"%s\".\n", Builder_TenXStringOrEmpty( binaryFolder ), tenXConfig->name );
					Builder_RewindScratch( &scratch );
					return false;
				}

				exe = Builder_FormatString( arena, "%s%c%s%s", binaryFolderPath, BUILDER_PATH_SEPARATOR, binaryName, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
			}

			for ( uint32_t tenXPlatformIndex = 0; tenXPlatformIndex < workspace->platformCount; tenXPlatformIndex++ ) {
				TenXPlatform *tenXPlatform = &workspace->platforms[tenXPlatformIndex];

				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<ConfigAndPlatform>\n" );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Name>%s:%s</Name>\n", tenXConfig->name, tenXPlatform->name );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<BuildCommand>%s%s</BuildCommand>\n", buildCommand, additionalBuildArgs );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<RebuildCommand>%s%s</RebuildCommand>\n", Builder_TenXStringOrEmpty( tenXConfig->rebuildCommand ), rebuildArgs );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<BuildFileCommand>%s%s</BuildFileCommand>\n", Builder_TenXStringOrEmpty( tenXConfig->buildFileCommand ), buildFileArgs );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<CleanCommand>%s</CleanCommand>\n", Builder_TenXStringOrEmpty( tenXConfig->cleanCommand ) );

				// the build command is the build exe, so build from where it lives
				// binaryFolder wouldn't work here - it's relative, may be NULL, and doesn't exist until the first build
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<BuildWorkingDirectory>%s</BuildWorkingDirectory>\n", Builder_TenXStringOrEmpty( Builder_TenXFirstNonEmpty( tenXConfig->buildWorkingDirectory, fullCWD ) ) );

				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<FileSyntaxCheckCommand>%s</FileSyntaxCheckCommand>\n", Builder_TenXStringOrEmpty( tenXConfig->fileSyntaxCheckCommand ) );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<CancelBuild>%s</CancelBuild>\n", Builder_TenXStringOrEmpty( tenXConfig->cancelBuild ) );

				if ( exe ) {
					StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Exe>%s</Exe>\n", exe );
				}

				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Args>%s</Args>\n", Builder_TenXStringOrEmpty( tenXConfig->runArgs ) );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<WorkingDirectory>%s</WorkingDirectory>\n", Builder_TenXStringOrEmpty( Builder_TenXFirstNonEmpty( tenXConfig->workingDirectory, fullCWD ) ) );

				// deliberately called using tenXConfig->runCommand here, as for some reason 10X decided to map the "Run Command" setting
				// to the DebugCommand XML tag.
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<DebugCommand>%s</DebugCommand>\n", Builder_TenXFirstNonEmpty( tenXConfig->runCommand, "$(Executable)" ) );

				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<DebuggerExe>%s</DebuggerExe>\n", Builder_TenXStringOrEmpty( Builder_TenXFirstNonEmpty( tenXConfig->debuggerExe, workspace->debuggerExe ) ) );
				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<DebugSln>%s</DebugSln>\n", Builder_TenXStringOrEmpty( Builder_TenXFirstNonEmpty( tenXConfig->debugSln, workspace->debugSln ) ) );

				// BuildConfig, TenXConfig, and platform defines go in the same list
				if ( tenXConfig->config->defines.count > 0 || tenXConfig->defines.count > 0 || tenXPlatform->defines.count > 0 ) {
					StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Defines>\n" );
					Builder_TenXAppendListItems( arena, &workspaceFileContent, "\t\t\t\t\t", "Define", &tenXConfig->config->defines );
					Builder_TenXAppendListItems( arena, &workspaceFileContent, "\t\t\t\t\t", "Define", &tenXConfig->defines );
					Builder_TenXAppendListItems( arena, &workspaceFileContent, "\t\t\t\t\t", "Define", &tenXPlatform->defines );
					StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t</Defines>\n" );
				} else {
					StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Defines></Defines>\n" );
				}

				Builder_TenXAppendListTag( arena, &workspaceFileContent, "\t\t\t\t", "ForceIncludes", "ForceInclude", &tenXConfig->forceIncludes );
				Builder_TenXAppendListTag( arena, &workspaceFileContent, "\t\t\t\t", "PrioritiseFiles", "PrioritiseFile", &tenXConfig->prioritiseFiles );

				StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t</ConfigAndPlatform>\n" );
			}
		}

		for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[tenXConfigIndex];

			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Config>\n" );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Name>%s</Name>\n", tenXConfig->name );
			Builder_TenXAppendListTag( arena, &workspaceFileContent, "\t\t\t\t", "Defines", "Define", &tenXConfig->config->defines );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t</Config>\n" );
		}

		for ( uint32_t tenXPlatformIndex = 0; tenXPlatformIndex < workspace->platformCount; tenXPlatformIndex++ ) {
			TenXPlatform *tenXPlatform = &workspace->platforms[tenXPlatformIndex];

			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t<Platform>\n" );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t\t<Name>%s</Name>\n", tenXPlatform->name );
			Builder_TenXAppendListTag( arena, &workspaceFileContent, "\t\t\t\t", "Defines", "Define", &tenXPlatform->defines );
			StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t\t</Platform>\n" );
		}

		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t</ConfigProperties>\n" );
	} else {
		StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<ConfigProperties></ConfigProperties>\n" );
	}

	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<TerminalDirectory>%s</TerminalDirectory>\n", Builder_TenXStringOrEmpty( workspace->terminalDirectory ) );

	// i think this is not used? can't find any reference to it in the settings window
	StringBuilder_Appendf( arena, &workspaceFileContent, "\t\t<Children></Children>\n" );

	StringBuilder_Appendf( arena, &workspaceFileContent, "\t</Workspace>\n" );
	StringBuilder_Appendf( arena, &workspaceFileContent, "</N10X>" );

	uint64_t length;
	char *workspaceFileContentString = StringBuilder_ToString( arena, &workspaceFileContent, &length );

	if ( !Builder_WriteEntireFile( workspaceFilename, workspaceFileContentString, length ) ) {
		Builder_Error( "Failed to write \"%s\".\n", workspaceFilename );
		Builder_RewindScratch( &scratch );
		return false;
	}

	printf( "Done\n" );

	Builder_RewindScratch( &scratch );

	// everything built in here goes into a file and is then done with - nothing outlives this call
	return true;
}

#endif // BUILDER_10X_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
