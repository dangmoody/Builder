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
Builder can be used to generate a 10x workspace (.10x) file:

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
					.runArgs				= MakeStringList( "--verbose" ),
				},
				{
					.name					= "Release",
					.config					= release,
					.additionalBuildArgs	= MakeStringList( "--config=Release" ),
					.prioritiseFiles		= MakeStringList( "src/main.c" ),
				},
			};

			TenXWorkspace workspace = {
				.configs				= configs,
				.configsCount			= BUILDER_COUNT_OF( configs ),
				.platforms				= platforms,
				.platformCount			= BUILDER_COUNT_OF( platforms ),
				.globalDefines			= MakeStringList( "MY_GLOBAL_DEFINE" ),
				.includeFilterList		= MakeStringList( "*.c", "*.h" ),
				.excludeFilterList		= MakeStringList( "*.obj", "*.pdb" ),
				.syncFiles				= true,
				.recursive				= true,
				.captureExeOutput		= true,
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

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "builder.h"

// A platform in the workspace's platform dropdown (e.g. "x64" or "ARM64").
// defines are the preprocessor defines 10x uses for this platform.
typedef struct TenXPlatform {
	// The name of the platform as it appears in 10x. E.g. "x64".
	const char	*name;
	
	// Defines 10x's parser uses when this platform is selected, on top of TenXWorkspace::globalDefines.
	StringList  defines;
} TenXPlatform;
	
// A config in the workspace's config dropdown (e.g. "Debug" or "Release").
// Each one wraps a BuildConfig and sets the build, run, and debug commands 10x uses for it.
typedef struct TenXConfig
{
	// The name of the config as it appears in 10x. E.g. "Debug".
	// Must be unique across TenXWorkspace::configs, but doesn't have to match BuildConfig::name.
	const char	*name;
	
	// The BuildConfig this entry builds.
	BuildConfig	*config;
	
	// Added to the end of the default BuildCommand.
	// Ignored if you set buildCommand yourself.
	StringList	additionalBuildArgs;
	
	// ---- Builder has defaults for these ----
	
	// Leave NULL to use TenXWorkspace::buildCommand, or your build exe if that's NULL too.
	// additionalBuildArgs are appended either way.
	const char	*buildCommand;
	
	// Leave NULL for the full path to the folder your build exe is in.
	const char	*buildWorkingDirectory;
	
	// The program 10x runs and debugs. Only written for BINARY_TYPE_EXE configs.
	// Leave NULL for the full path to config->binaryFolder + config->binaryName + extension.
	const char	*exe;
	
	// The CWD of the program when 10x runs it.
	// Leave NULL for the folder your build exe is in.
	const char	*workingDirectory;
	
	// Leave NULL for: $(Executable)
	const char	*debugCommand;
	
	// Leave NULL to use TenXWorkspace::debuggerExe.
	const char	*debuggerExe;
	
	// The Visual Studio solution 10x opens when debugging with Visual Studio.
	// Leave NULL to use TenXWorkspace::debugSln.
	const char	*debugSln;
	
	// ---- Left empty in the workspace file if NULL ----
	
	// The command 10x runs when you rebuild. additionalBuildArgs are appended to it.
	const char	*rebuildCommand;
	
	// The command 10x runs when you clean.
	const char	*cleanCommand;
	
	// The command 10x runs to build just the current file. additionalBuildArgs are appended to it.
	const char	*buildFileCommand;
	
	// The command 10x runs to syntax check the current file.
	const char	*fileSyntaxCheckCommand;
	
	// The command 10x runs to cancel a build that is in progress.
	const char	*cancelBuild;
	
	// Command line arguments passed to the program when 10x runs it.
	StringList	runArgs;
	
	// Headers 10x's parser treats as included at the top of every file.
	StringList	forceIncludes;
	
	// Files 10x parses before everything else.
	StringList	prioritiseFiles;
	
} TenXConfig;
	
// Everything that goes into the generated .10x workspace file.
// Pass this to Builder_Generate10xWorkspace().
typedef struct TenXWorkspace {
	
	// Workspace filename, without extension. Defaults to BuilderOptions::defaultConfig->name if empty.
	const char	*name;
	
	// Destination folder for the workspace file in disk. Defaults to the same folder as your build executable if empty.
	const char	*path;
	
	// The command 10x runs to build the project. If empty, builder will default to run your build exe.
	const char  *buildCommand;
	
	// The debugger 10x uses for every config that doesn't set TenXConfig::debuggerExe.
	// Leave NULL to use 10x's default debugger.
	const char	*debuggerExe;
	
	// The Visual Studio solution 10x opens when debugging, for every config that doesn't set TenXConfig::debugSln.
	const char	*debugSln;
	
	// The folder 10x's terminal opens in.
	const char *terminalDirectory;
	
	// Keep the workspace's file list in sync with the files on disk.
	bool		syncFiles;
	
	// Include files in subfolders, not just the top level.
	bool		recursive;
	
	// Show folders in the workspace tree even if no files in them match the filters.
	bool		showEmptyFolders;
	
	// Include files with no extension, which includeFilterList can't match.
	bool		includeFilesWithoutExtension;
	
	// Treat the workspace as a plain folder instead of a project.
	bool		isFolder;
	
	// Run the Visual Studio environment batch file (vcvars) before build commands, so MSVC is on the PATH.
	bool		useVisualStudioEnvBat;
	
	// Show your program's output in 10x's output panel when it runs.
	bool		captureExeOutput;
	
	// Underline errors in the editor using the build output.
	bool		allowAutoErrorUnderlining;
	
	// Platforms in the workspace's platform dropdown.
	// Leave NULL (and platformCount at 0) for "x64" and "Win32".
	TenXPlatform *platforms;
	uint32_t	 platformCount;
	
	// Configs in the workspace's config dropdown.
	// One <ConfigAndPlatform> entry is written for every config and platform pair.
	TenXConfig	*configs;
	uint32_t	configsCount;
	
	// Defines 10x's parser uses for every config and platform.
	// Builder always adds BUILDER_IMPLEMENTATION, BUILDER_10X_IMPLEMENTATION, and the defines for your OS and compiler.
	StringList globalDefines;
	
	// Wildcards for the files to include in the workspace. E.g. "*.c".
	// Leave empty to use tenXDefaultIncludeFilter.
	StringList	includeFilterList;
	
	// Wildcards for the files to exclude from the workspace.
	StringList	excludeFilterList;
	
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

static const char *Builder_TenXGetFullBinaryPath( arena_t *arena, TenXConfig *tenXConfig ) {
	BuildConfig *buildConfig = tenXConfig->config;
	const char *extension = Builder_GetFileExtensionFromBinaryType( buildConfig->binaryType );

	const char *cwd = Builder_TenXGetFullCWD( arena );
	if ( !cwd ) {
		return NULL;
	}

	char *fullPath = buildConfig->binaryFolder
		? Builder_FormatString( arena, "%s%c%s%c%s%s", cwd, BUILDER_PATH_SEPARATOR, buildConfig->binaryFolder, BUILDER_PATH_SEPARATOR, buildConfig->binaryName, extension )
		: Builder_FormatString( arena, "%s%c%s%s", cwd, BUILDER_PATH_SEPARATOR, buildConfig->binaryName, extension );

	for ( char *c = fullPath; *c; c++ ) {
		if ( *c == '/' || *c == '\\' ) {
			*c = BUILDER_PATH_SEPARATOR;
		}
	}

	return fullPath;
}

static const char *Builder_TenXStringOrEmpty( const char* string ) {
	return Builder_StringIsEmpty( string ) ? "" : string;
}

static const char* Builder_TenXBoolToString( bool value ) {
	return value ? "True" : "False";
}

bool Builder_Generate10xWorkspace( BuilderOptions *options, TenXWorkspace *workspace, int argc, char **argv ) {
	BUILDER_ASSERT( options );

	Builder_SetCmdArgs( options, argc, argv );
	Builder_SetCWD( options, argv );

	// initial workspace validation.
	bool validWorkspace = true;
	{
		if ( workspace->configsCount < 1 ) {
			Builder_Error( "You must setup at least one config (TenXConfig) when generating when generating a 10x Workspace (TenXWorkspace).\n" );
			validWorkspace = false;
		}

		for ( uint32_t configIndex = 0; configIndex < workspace->configsCount; configIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[configIndex];

			if ( Builder_StringIsEmpty( tenXConfig->name ) ) {
				Builder_Error( "Config %u for the 10x Workspace doesn't have a name set.  You need to fill that in.\n", configIndex );
				validWorkspace = false;
			} else if ( !tenXConfig->config ) {
				Builder_Error( "Config \"%s\" for the 10x Workspace doesn't have a BuildConfig set.  You need to fill that in.\n", tenXConfig->name );
				validWorkspace = false;
			}

			if ( !Builder_StringIsEmpty( tenXConfig->name ) ) {
				for ( uint32_t otherIndex = 0; otherIndex < configIndex; otherIndex++ ) {
					if ( !Builder_StringIsEmpty( workspace->configs[otherIndex].name ) && Builder_StringEquals( workspace->configs[otherIndex].name, tenXConfig->name ) ) {
						Builder_Error( "10x Workspace has more than one config named \"%s\".  Config names must be unique.\n", tenXConfig->name );
						validWorkspace = false;
						break;
					}
				}
			}
		}

		// the workspace filename falls back to the default config's name. I know Builder doesn't allow empty config names, but 
		// we could potentially execute 10x's workspace generation before the build step?
		if ( Builder_StringIsEmpty( workspace->name ) && ( !options->defaultConfig || Builder_StringIsEmpty( options->defaultConfig->name ) ) ) {
			Builder_Error( "The 10x Workspace doesn't have a name, and there is no BuilderOptions::defaultConfig with a name to fall back to.  You need to set one of them.\n" );
			validWorkspace = false;
		}

		if ( !validWorkspace ) {
			return false;
		}
	}

	if ( workspace->platformCount < 1 )
	{
		// default platforms 10x generates
		static TenXPlatform defaultPlatforms[] = {
			{ .name = "x64" },
			{ .name = "Win32" }
		};

		workspace->platforms = defaultPlatforms;
		workspace->platformCount = 2;

		Builder_Warning( "No platforms defined for 10x Workspace. Appended default 'x64' and 'Win32' platforms.\n" );
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

		workspaceFilename = Builder_FormatString( scratch.arena, "%s%c%s%s", workspace->path, BUILDER_PATH_SEPARATOR, workspaceName, ".10x" );
	}

	stringBuilder_t workspaceFileContent = { 0 };

	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "<?xml version=\"1.0\"?>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "<N10X>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t<Workspace>\n" );

	// include filter
	if ( workspace->includeFilterList.count > 0 ) {
		stringBuilder_t includeFilter = { 0 };

		for ( builderStringChunk_t *filterChunk = workspace->includeFilterList.head; filterChunk; filterChunk = filterChunk->next ) {
			for ( uint32_t filterChunkIndex = 0; filterChunkIndex < filterChunk->count; filterChunkIndex++ ) {
				StringBuilder_Appendf(scratch.arena, &includeFilter, Builder_FormatString( scratch.arena, "%s,", filterChunk->items[filterChunkIndex]));
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilter>%s</IncludeFilter>\n", StringBuilder_ToString( scratch.arena, &includeFilter, NULL ) ) );

	} else {
		static const char	*tenXDefaultIncludeFilter = "*.c,*.cc,*.cpp,*.c++,*.cp,*.cxx,*.h,*.hh,*.hpp,*.h++,*.hp,*.hxx,*.inl,*.cs,*.rs,*.java,*.jav,*.js,*.jsc,*.jsx,*.json,*.cls,*.ts,*.py,*.rpy,*.php,*.php3,*.phl,*.phtml,*.rhtml,*.tpl,*.phps,*.lua,*.html,*.html5,*.htm,*.xml,*.xaml,*.css,*.ssi,*.haml,*.yaml,*.bat,*.wbf,*.wbt,*.txt,*.cmake,*.make,*.makefile,*.mak,*.mk,*.sh,*.bash,*.csv,*.asp,*.pl,*.mac,*.ws,*.vbs,*.perl,*.src,*.rss,*.inc,*.f,*.go,*.prl,*.plx,*.rb,*.lsp,*.lpx,*.ps1,*.command,*.cbl,*.cob,*.qs,*.wxs,*.ph,*.msc,*.glsl,*.hlsl,*.fx,*.vert,*.tesc,*.tese,*.geom,*.frag,*.comp,*.pssl,*.scons,*.cu,*.jai,*.md,";
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilter>%s</IncludeFilter>\n", tenXDefaultIncludeFilter ) );
	}

	// exclude filter
	if (workspace->excludeFilterList.count > 0) {
		stringBuilder_t excludeFilter = { 0 };

		for ( builderStringChunk_t *filterChunk = workspace->excludeFilterList.head; filterChunk; filterChunk = filterChunk->next ) {
			for ( uint32_t filterChunkIndex = 0; filterChunkIndex < filterChunk->count; filterChunkIndex++ ) {
				StringBuilder_Appendf( scratch.arena, &excludeFilter, Builder_FormatString(scratch.arena, "%s,", filterChunk->items[filterChunkIndex] ) );
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<ExcludeFilter>%s</ExcludeFilter>\n", StringBuilder_ToString( scratch.arena, &excludeFilter, NULL ) ) );

	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ExcludeFilter></ExcludeFilter>\n" );
	}

	// top-level settings
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<SyncFiles>%s</SyncFiles>\n", Builder_TenXBoolToString( workspace->syncFiles ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<Recursive>%s</Recursive>\n", Builder_TenXBoolToString( workspace->recursive ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<ShowEmptyFolders>%s</ShowEmptyFolders>\n", Builder_TenXBoolToString( workspace->showEmptyFolders ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilesWithoutExt>%s</IncludeFilesWithoutExt>\n", Builder_TenXBoolToString( workspace->includeFilesWithoutExtension ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IsFolder>%s</IsFolder>\n", Builder_TenXBoolToString( workspace->isFolder ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<UseVisualStudioEnvBat>%s</UseVisualStudioEnvBat>\n", Builder_TenXBoolToString( workspace->useVisualStudioEnvBat ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<CaptureExeOutput>%s</CaptureExeOutput>\n", Builder_TenXBoolToString( workspace->captureExeOutput ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<AllowAutoErrorUnderlining_v3>%s</AllowAutoErrorUnderlining_v3>\n", Builder_TenXBoolToString( workspace->allowAutoErrorUnderlining ) ) );

	// config names
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Configurations>\n" );
	for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<Configuration>%s</Configuration>\n", workspace->configs[tenXConfigIndex].name ) );
	}
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</Configurations>\n" );

	// platforms
	if ( workspace->platformCount > 0 ) {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Platforms>\n" );
		for ( uint32_t platformIndex = 0; platformIndex < workspace->platformCount; platformIndex++ ) {
			TenXPlatform *platform = &workspace->platforms[platformIndex];

			if ( !Builder_StringIsEmpty( platform->name ) )
			{
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<Platform>%s</Platform>\n", platform->name ) );
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</Platforms>\n" );
	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Platforms>\n\t\t\t<Platform>x64</Platform>\n\t\t\t<Platform>Win32</Platform>\n\t\t</Platforms>\n" );
	}

	// additional include paths
	if (options->configs.count > 0)
	{
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<AdditionalIncludePaths>\n" );
		for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
			for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
				BuildConfig *config = chunk->items[configIndex];

				for ( builderStringChunk_t *includeChunk = config->additionalIncludes.head; includeChunk; includeChunk = includeChunk->next ) {
					for ( uint32_t includeIndex = 0; includeIndex < includeChunk->count; includeIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", includeChunk->items[includeIndex] ) );
					}
				}
			}
		}

#ifdef _WIN32
		if( Builder_GetMSVCInstall( scratch.arena, &g_msvcInstall ) ) {
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_msvcInstall.includePath ) );
		}
		
		if ( Builder_GetWindowsSDKInstall( scratch.arena, &g_windowsSDKInstall ) ) {
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.ucrtIncludePath ) );
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.umIncludePath ) );
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<AdditionalIncludePath>%s</AdditionalIncludePath>\n", g_windowsSDKInstall.sharedIncludePath ) );
		}
#endif

		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</AdditionalIncludePaths>\n" );
	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<AdditionalIncludePaths></AdditionalIncludePaths>\n" );
	}

	// global defines
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Defines>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>BUILDER_IMPLEMENTATION</Define>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>BUILDER_10X_IMPLEMENTATION</Define>\n" );

#ifdef _WIN32
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>_WIN32</Define>\n" );
#elif __linux__
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>__linux__</Define>\n" );
#endif

#if defined(__clang__)
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>__clang__</Define>\n" );
#elif defined(__GNUC__)
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>__GNUC__</Define>\n" );
#elif defined(__GNUG__)
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>__GNUG__</Define>\n" );
#elif defined(_MSC_VER)
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Define>_MSC_VER</Define>\n" );
#endif

	for ( builderStringChunk_t *definesChunk = workspace->globalDefines.head; definesChunk; definesChunk = definesChunk->next ) {
		for ( uint32_t defineIndex = 0; defineIndex < definesChunk->count; defineIndex++ ) {
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<Define>%s</Define>\n", definesChunk->items[defineIndex] ) );
		}
	}
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</Defines>\n" );

	// config properties
	if ( workspace->configsCount > 0 ) {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ConfigProperties>\n" );
		for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[tenXConfigIndex];

			for ( uint32_t tenXPlatformIndex = 0; tenXPlatformIndex < workspace->platformCount; tenXPlatformIndex++ ) {
				TenXPlatform *tenXPlatform = &workspace->platforms[tenXPlatformIndex];

				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<ConfigAndPlatform>\n" );
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Name>%s:%s</Name>\n", tenXConfig->name, tenXPlatform->name ) );

				const char *buildCommand = tenXConfig->buildCommand;
				if ( Builder_StringIsEmpty( buildCommand ) ) {
					buildCommand = workspace->buildCommand;
					if ( Builder_StringIsEmpty( buildCommand ) ) {
						char exePath[BUILDER_MAX_PATH] = { 0 };
						Builder_GetExePath( argv, exePath );
						buildCommand = exePath;
					}
				}

				if ( tenXConfig->additionalBuildArgs.count > 0 ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildCommand>%s", buildCommand ) );
					for ( builderStringChunk_t *argsChunk = tenXConfig->additionalBuildArgs.head; argsChunk; argsChunk = argsChunk->next ) {
						for ( uint32_t argIndex = 0; argIndex < argsChunk->count; argIndex++ ) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, " %s", argsChunk->items[argIndex] ) );
						}
					}
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "</BuildCommand>\n" );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildCommand>%s</BuildCommand>\n", buildCommand ) );
				}

				if( !Builder_StringIsEmpty( tenXConfig->rebuildCommand ) ) {
					if ( tenXConfig->additionalBuildArgs.count > 0 ) {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<RebuildCommand>%s", tenXConfig->rebuildCommand ) );
						for ( builderStringChunk_t *argsChunk = tenXConfig->additionalBuildArgs.head; argsChunk; argsChunk = argsChunk->next ) {
							for ( uint32_t argIndex = 0; argIndex < argsChunk->count; argIndex++ ) {
								StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, " %s", argsChunk->items[argIndex] ) );
							}
						}
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "</RebuildCommand>\n" );
					} else {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<RebuildCommand>%s</RebuildCommand>\n", tenXConfig->rebuildCommand ) );
					}
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<RebuildCommand></RebuildCommand>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->buildFileCommand ) ) {
					if ( tenXConfig->additionalBuildArgs.count > 0 ) {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildFileCommand>%s", tenXConfig->buildFileCommand ) );
						for ( builderStringChunk_t *argsChunk = tenXConfig->additionalBuildArgs.head; argsChunk; argsChunk = argsChunk->next ) {
							for ( uint32_t argIndex = 0; argIndex < argsChunk->count; argIndex++ ) {
								StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, " %s", argsChunk->items[argIndex] ) );
							}
						}
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "</BuildFileCommand>\n" );
					} else {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildFileCommand>%s</BuildFileCommand>\n", tenXConfig->buildFileCommand ) );
					}
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<BuildFileCommand></BuildFileCommand>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->cleanCommand ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<CleanCommand>%s</CleanCommand>\n", tenXConfig->cleanCommand ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<CleanCommand></CleanCommand>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->buildWorkingDirectory ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildWorkingDirectory>%s</BuildWorkingDirectory>\n", tenXConfig->buildWorkingDirectory ) );
				} else {
					// the build command is the build exe, so build from where it lives
					// binaryFolder wouldn't work here - it's relative, may be NULL, and doesn't exist until the first build
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildWorkingDirectory>%s</BuildWorkingDirectory>\n", Builder_TenXGetFullCWD( scratch.arena ) ) );
				}

				if( !Builder_StringIsEmpty( tenXConfig->fileSyntaxCheckCommand ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<FileSyntaxCheckCommand>%s</FileSyntaxCheckCommand>\n", tenXConfig->fileSyntaxCheckCommand ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<FileSyntaxCheckCommand></FileSyntaxCheckCommand>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->cancelBuild ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<CancelBuild>%s</CancelBuild>\n", tenXConfig->cancelBuild ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<CancelBuild></CancelBuild>\n" );
				}

				if (tenXConfig->config->binaryType == BINARY_TYPE_EXE) {
					const char *exe = tenXConfig->exe;
					exe = Builder_StringIsEmpty(exe) ? Builder_TenXGetFullBinaryPath( scratch.arena, tenXConfig ) : exe;
					if ( !Builder_StringIsEmpty(exe) ) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Exe>%s</Exe>\n", exe ) );
					} else {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Exe></Exe>\n" );
					}
				}

				if ( tenXConfig->runArgs.count > 0 ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Args>\n" );
					for ( builderStringChunk_t *runArgsChunk = tenXConfig->runArgs.head; runArgsChunk; runArgsChunk = runArgsChunk->next ) {
						for ( uint32_t runArgIndex = 0; runArgIndex < runArgsChunk->count; runArgIndex++ ) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Arg>%s</Arg>\n", runArgsChunk->items[runArgIndex] ) );
						}
					}
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</Args>\n" );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Args></Args>\n", tenXConfig->buildFileCommand ) );
				}

				if( !Builder_StringIsEmpty( tenXConfig->workingDirectory ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<WorkingDirectory>%s</WorkingDirectory>\n", tenXConfig->workingDirectory ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<WorkingDirectory>%s</WorkingDirectory>\n", Builder_TenXGetFullCWD( scratch.arena ) ) );
				}

				if( !Builder_StringIsEmpty( tenXConfig->debugCommand ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<DebugCommand>%s</DebugCommand>\n", tenXConfig->debugCommand ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<DebugCommand>$(Executable)</DebugCommand>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->debuggerExe ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<DebuggerExe>%s</DebuggerExe>\n", tenXConfig->debuggerExe ) );
				} else if( !Builder_StringIsEmpty( workspace->debuggerExe ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<DebuggerExe>%s</DebuggerExe>\n", workspace->debuggerExe ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<DebuggerExe></DebuggerExe>\n" );
				}

				if( !Builder_StringIsEmpty( tenXConfig->debugSln ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<DebugSln>%s</DebugSln>\n", tenXConfig->debugSln ) );
				} else if( !Builder_StringIsEmpty( workspace->debugSln ) ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<DebugSln>%s</DebugSln>\n", workspace->debugSln ) );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<DebugSln></DebugSln>\n" );
				}

				if ( tenXPlatform->defines.count > 0 || tenXConfig->config->defines.count > 0 )
				{
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines>\n" );
					for ( builderStringChunk_t *platformDefineChunk = tenXPlatform->defines.head; platformDefineChunk; platformDefineChunk = platformDefineChunk->next ) {
						for (uint32_t platformDefineIndex = 0; platformDefineIndex < platformDefineChunk->count; platformDefineIndex++) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Define>%s</Define>\n", platformDefineChunk->items[platformDefineIndex] ) );
						}
					}
					for ( builderStringChunk_t *configDefineChunk = tenXConfig->config->defines.head; configDefineChunk; configDefineChunk = configDefineChunk->next ) {
						for ( uint32_t configDefineIndex = 0; configDefineIndex < configDefineChunk->count; configDefineIndex++ ) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Define>%s</Define>\n", configDefineChunk->items[configDefineIndex] ) );
						}
					}
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</Defines>\n" );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines></Defines>\n" );
				}

				if ( tenXConfig->forceIncludes.count > 0 )
				{
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<ForceIncludes>\n" );
					for ( builderStringChunk_t *forceIncludeChunk = tenXConfig->forceIncludes.head; forceIncludeChunk; forceIncludeChunk = forceIncludeChunk->next ) {
						for (uint32_t forceIncludeIndex = 0; forceIncludeIndex < forceIncludeChunk->count; forceIncludeIndex++) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<ForceInclude>%s</ForceInclude>\n", forceIncludeChunk->items[forceIncludeIndex] ) );
						}
					}
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</ForceIncludes>\n" );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<ForceIncludes></ForceIncludes>\n" );
				}

				if ( tenXConfig->prioritiseFiles.count > 0 )
				{
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<PrioritiseFiles>\n" );
					for ( builderStringChunk_t *prioFilesChunk = tenXConfig->prioritiseFiles.head; prioFilesChunk; prioFilesChunk = prioFilesChunk->next ) {
						for (uint32_t prioFilesIndex = 0; prioFilesIndex < prioFilesChunk->count; prioFilesIndex++) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<PrioritiseFile>%s</PrioritiseFile>\n", prioFilesChunk->items[prioFilesIndex] ) );
						}
					}
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</PrioritiseFiles>\n" );
				} else {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<PrioritiseFiles></PrioritiseFiles>\n" );
				}

				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t</ConfigAndPlatform>\n" );
			}
		}
		
		for ( uint32_t tenXConfigIndex = 0; tenXConfigIndex < workspace->configsCount; tenXConfigIndex++ ) {
			TenXConfig *tenXConfig = &workspace->configs[tenXConfigIndex];

			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Config>\n" );
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Name>%s</Name>\n", tenXConfig->name ) );

			if ( tenXConfig->config->defines.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines>\n" );
				for ( builderStringChunk_t *configDefineChunk = tenXConfig->config->defines.head; configDefineChunk; configDefineChunk = configDefineChunk->next ) {
					for ( uint32_t configDefineIndex = 0; configDefineIndex < configDefineChunk->count; configDefineIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Define>%s</Define>\n", configDefineChunk->items[configDefineIndex] ) );
					}
				}
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</Defines>\n" );
			} else {
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines></Defines>\n" );
			}
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t</Config>\n" );
		}

		for (uint32_t tenXPlatformIndex = 0; tenXPlatformIndex < workspace->platformCount; tenXPlatformIndex++) {
			TenXPlatform *tenXPlatform = &workspace->platforms[tenXPlatformIndex];

			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<Platform>\n" );
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Name>%s</Name>\n", tenXPlatform->name) );

			if ( tenXPlatform->defines.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines>\n" );
				for ( builderStringChunk_t *platformDefineChunnk = tenXPlatform->defines.head; platformDefineChunnk; platformDefineChunnk = platformDefineChunnk->next ) {
					for ( uint32_t platformDefineIndex = 0; platformDefineIndex < platformDefineChunnk->count; platformDefineIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Define>%s</Define>\n", platformDefineChunnk->items[platformDefineIndex] ) );
					}
				}
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</Defines>\n" );
			} else {
				StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Defines></Defines>\n" );
			}

			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t</Platform>\n" );
		}

		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</ConfigProperties>\n" );
	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ConfigProperties></ConfigProperties>\n" );
	}

	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<TerminalDirectory>%s</TerminalDirectory>\n", Builder_TenXStringOrEmpty( workspace->terminalDirectory ) ) );
	
	// i think this is not used? can't find any reference to it in the settings window
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Children></Children>\n" );

	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t</Workspace>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "</N10X>" );

	uint64_t length;
	char *workspaceFileContentString = StringBuilder_ToString( scratch.arena, &workspaceFileContent, &length );

	bool wroteFile = Builder_WriteEntireFile( workspaceFilename, workspaceFileContentString, length );

	if ( wroteFile ) {
		printf( "Done\n" );
	} else {
		Builder_Error( "Failed to write \"%s\".\n", workspaceFilename );
	}

	if ( !wroteFile ) {
		Builder_RewindScratch( &scratch );
		return false;
	}

	Builder_RewindScratch( &scratch );

	// everything built in here goes into a file and is then done with - nothing outlives this call
	return true;
}

#endif // BUILDER_10X_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
