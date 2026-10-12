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
Builder can be used to generate Zed's tasks.json and debug.json files:

	#define BUILDER_IMPLEMENTATION
	#include "builder.h"

	#define BUILDER_10X_IMPLEMENTATION
	#include "builder_zed.h"

	int main( int argc, char **argv ) {
		BuilderOptions options = { 0 };

		BuildConfig *config = CreateBuildConfig( &options );
		*config = (BuildConfig) {
			.name			= "my_awesome_program",
			.sourceFiles	= MakeStringList( "src/my_code.c" ),
		};

		options.defaultConfig = config;

		if ( HasCommandLineArg( argc, argv, "--zed" ) ) {
			ZedTaskConfig taskConfigs[] = {
				{ .config = config },
			};

			ZedDebugConfig debugConfigs[] = {
				{
					.label		= "Debug my_awesome_program",
					.binaryName	= "bin/my_awesome_program",
					.adapter	= ZED_DEBUGGER_ADAPTER_CODELLDB,
					.request	= ZED_DEBUGGER_REQUEST_LAUNCH,
				},
			};

			ZedJSONOptions zedOptions = {
				.taskConfigs		= taskConfigs,
				.taskConfigsCount	= BUILDER_COUNT_OF( taskConfigs ),
				.debugConfigs		= debugConfigs,
				.debugConfigsCount	= BUILDER_COUNT_OF( debugConfigs ),
			};

			return Builder_GenerateZedJSONFiles( &options, &zedOptions, argc, argv ) ? 0 : 1;
		}

		return Build( &options, argc, argv );
	}

You'd then generate the JSON files with:

	build.exe --zed

By default, if you call Builder_GenerateZedJSONFiles() without filling in any other settings Builder will generate a JSON entry for each BuildConfig you created.

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "builder.h"

#ifdef BUILDER_10X_IMPLEMENTATION

#if !defined( BUILDER_IMPLEMENTATION )
#error "BUILDER_10X_IMPLEMENTATION requires BUILDER_IMPLEMENTATION to also be defined, and \"builder.h\" to be included before \"builder_zed.h\", in this translation unit."
#endif

static const char	*tenXDefaultIncludeFilter = "*.c,*.cc,*.cpp,*.c++,*.cp,*.cxx,*.h,*.hh,*.hpp,*.h++,*.hp,*.hxx,*.inl,*.cs,*.rs,*.java,*.jav,*.js,*.jsc,*.jsx,*.json,*.cls,*.ts,*.py,*.rpy,*.php,*.php3,*.phl,*.phtml,*.rhtml,*.tpl,*.phps,*.lua,*.html,*.html5,*.htm,*.xml,*.xaml,*.css,*.ssi,*.haml,*.yaml,*.bat,*.wbf,*.wbt,*.txt,*.cmake,*.make,*.makefile,*.mak,*.mk,*.sh,*.bash,*.csv,*.asp,*.pl,*.mac,*.ws,*.vbs,*.perl,*.src,*.rss,*.inc,*.f,*.go,*.prl,*.plx,*.rb,*.lsp,*.lpx,*.ps1,*.command,*.cbl,*.cob,*.qs,*.wxs,*.ph,*.msc,*.glsl,*.hlsl,*.fx,*.vert,*.tesc,*.tese,*.geom,*.frag,*.comp,*.pssl,*.scons,*.cu,*.jai,*.md,";

typedef struct TenXPlatform {
	const char	*name;
	StringList  defines;
} TenXPlatform;

typedef struct TenXConfigAndPlatformOptions {
	const char	*name;
    const char	*path;
	const char  *buildCommand;
	const char  *rebuildCommand;
	const char  *buildFileCommand;
	const char  *cleanCommand;
	const char  *buildWorkingDirectory;
	const char  *fileSyntaxCheckCommand;
	const char  *cancelCommand;
	const char  *executablePath;
	const char  *workingDirectory;
	const char  *debugCommand;
	const char  *debuggerPath;
	const char  *debugSolutionPath;
	StringList  args;
	StringList  forceIncludeList;
	StringList  prioritiseFileList;
} TenXConfigAndPlatformOptions;

typedef struct TenXWorkspaceOptions {
	StringList	includeFilterList;
	StringList	excludeFilterList;
	bool		syncFiles;
	bool		recursive;
	bool		showEmptyFolders;
	bool		includeFilesWithoutExtension;
	bool		isFolder;
	bool		useVisualStudioEnvBat;
	bool		captureExeOutput;
	bool		allowAutoErrorUnderlining;
	TenXPlatform *platforms;
	uint32_t	 platformCount;
	StringList globalDefines;

	const char	*name;	// workspace filename, "Workspace.10x" if empty
	const char	*path;	// folder to write the workspace into, next to the build exe if empty
	const char  *buildCommand;
	const char  *rebuildCommand;
	const char  *buildFileCommand;
	const char  *cleanCommand;
	const char  *buildWorkingDirectory;
	const char  *fileSyntaxCheckCommand;
	const char  *cancelCommand;
	const char  *executablePath;
	const char  *workingDirectory;
	const char  *debugCommand;
	const char  *debuggerPath;
	const char  *debugSolutionPath;
	StringList  args;
	StringList  forceIncludeList;
	StringList  prioritiseFileList;
} TenXWorkspaceOptions;

static TenXWorkspaceOptions TenX_CreateDefaultWorkspaceOptions()
{
	TenXWorkspaceOptions out = { 0 };

    static TenXPlatform platforms[] = {
        { .name = "x64" },
        { .name = "Win32" }
    };

	out.includeFilterList = MakeStringList("*.c", "*.cc", "*.cpp", "*.c++", "*.cp", "*.cxx", "*.h", "*.hh", "*.hpp", "*.h++", "*.hp", "*.hxx", "*.inl", "*.cs", "*.rs", "*.java", "*.jav", "*.js", "*.jsc", "*.jsx", "*.json", "*.cls", "*.ts", "*.py", "*.rpy", "*.php", "*.php3", "*.phl", "*.phtml", "*.rhtml", "*.tpl", "*.phps", "*.lua", "*.html", "*.html5", "*.htm", "*.xml", "*.xaml", "*.css", "*.ssi", "*.haml", "*.yaml", "*.bat", "*.wbf", "*.wbt", "*.txt", "*.cmake", "*.make", "*.makefile", "*.mak", "*.mk", "*.sh", "*.bash", "*.csv", "*.asp", "*.pl", "*.mac", "*.ws", "*.vbs", "*.perl", "*.src", "*.rss", "*.inc", "*.f", "*.go", "*.prl", "*.plx", "*.rb", "*.lsp", "*.lpx", "*.ps1", "*.command", "*.cbl", "*.cob", "*.qs", "*.wxs", "*.ph", "*.msc", "*.glsl", "*.hlsl", "*.fx", "*.vert", "*.tesc", "*.tese", "*.geom", "*.frag", "*.comp", "*.pssl", "*.scons", "*.cu", "*.jai", "*.md");
    out.syncFiles = true;
	out.recursive = true;
	out.showEmptyFolders = true;
	out.allowAutoErrorUnderlining = true;
    out.platforms =  platforms;
    out.platformCount = BUILDER_COUNT_OF( platforms );
	out.name = "";
	out.path = "";
	out.buildCommand = "";
	out.rebuildCommand = "";
	out.buildFileCommand = "";
	out.cleanCommand = "";
	out.buildWorkingDirectory = "";
	out.fileSyntaxCheckCommand = "";
	out.cancelCommand = "";
	out.executablePath = "";
	out.workingDirectory = "";
	out.debugCommand = "";
	out.debuggerPath = "";
	out.debugSolutionPath = "";

	return out;
}

static const char* TenX_MakeConfigAndPlatformName( BuildConfig *config, TenXPlatform *platform ) {
    return Builder_FormatString( "%s:%s", config->name , platform->name );
}

static const char* TenX_StringOrEmpty( const char* string ) {
	return Builder_StringIsEmpty( string ) ? "" : string;
}

static const char* TenX_BoolToString( bool value ) {
	return value ? "True" : "False";
}

bool Builder_Generate10xWorkspace( BuilderOptions *options, TenXWorkspaceOptions *workspaceOptions, int argc, char **argv );

bool Builder_Generate10xWorkspace( BuilderOptions *options, TenXWorkspaceOptions *workspaceOptions, int argc, char **argv ) {
	BUILDER_ASSERT( options );

	Builder_SetCmdArgs( options, argc, argv );
	Builder_SetCWD( options, argv );

	scratch_t scratch = Builder_GetScratch( NULL );

	const char *workspaceName = !Builder_StringIsEmpty( workspaceOptions->name ) ? workspaceOptions->name : "Workspace.10x";

	// no path means next to the build exe, which Builder_SetCWD() just made the CWD
	const char *workspaceFilename = workspaceName;

	if ( !Builder_StringIsEmpty( workspaceOptions->path ) ) {
		if ( !Builder_CreateFolderIfItDoesntExist( workspaceOptions->path ) ) {
			Builder_Error( "Failed to create \"%s\" folder.\n", workspaceOptions->path );
			Builder_RewindScratch( &scratch );
			return false;
		}

		workspaceFilename = Builder_FormatString( scratch.arena, "%s%c%s", workspaceOptions->path, BUILDER_PATH_SEPARATOR, workspaceName );
	}

	stringBuilder_t workspaceFileContent = { 0 };

	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "<?xml version=\"1.0\"?>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "<N10X>\n" );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t<Workspace>\n" );

	// include filter

	if ( workspaceOptions->includeFilterList.count > 0 ) {
		stringBuilder_t includeFilter = { 0 };

		for ( builderStringChunk_t *filterChunk = workspaceOptions->includeFilterList.head; filterChunk; filterChunk = filterChunk->next ) {
			for ( uint32_t filterChunkIndex = 0; filterChunkIndex < filterChunk->count; filterChunkIndex++ ) {
				StringBuilder_Appendf(scratch.arena, &includeFilter, Builder_FormatString( scratch.arena, "%s,", filterChunk->items[filterChunkIndex]));
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilter>%s</IncludeFilter>\n", StringBuilder_ToString( scratch.arena, &includeFilter, NULL ) ) );

	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilter>%s</IncludeFilter>\n", tenXDefaultIncludeFilter ) );
	}

	// exclude filter

	if (workspaceOptions->excludeFilterList.count > 0) {
		stringBuilder_t excludeFilter = { 0 };

		for ( builderStringChunk_t *filterChunk = workspaceOptions->excludeFilterList.head; filterChunk; filterChunk = filterChunk->next ) {
			for ( uint32_t filterChunkIndex = 0; filterChunkIndex < filterChunk->count; filterChunkIndex++ ) {
				StringBuilder_Appendf( scratch.arena, &excludeFilter, Builder_FormatString(scratch.arena, "%s,", filterChunk->items[filterChunkIndex] ) );
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<ExcludeFilter>%s</ExcludeFilter>\n", StringBuilder_ToString( scratch.arena, &excludeFilter, NULL ) ) );

	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ExcludeFilter></ExcludeFilter>\n" );
	}

	// top-level settings

	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<SyncFiles>%s</SyncFiles>\n", TenX_BoolToString( workspaceOptions->syncFiles ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<Recursive>%s</Recursive>\n", TenX_BoolToString( workspaceOptions->recursive ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<ShowEmptyFolders>%s</ShowEmptyFolders>\n", TenX_BoolToString( workspaceOptions->showEmptyFolders ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<IncludeFilesWithoutExt>%s</IncludeFilesWithoutExt>\n", TenX_BoolToString( workspaceOptions->includeFilesWithoutExtension ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<UseVisualStudioEnvBat>%s</UseVisualStudioEnvBat>\n", TenX_BoolToString( workspaceOptions->useVisualStudioEnvBat ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<CaptureExeOutput>%s</CaptureExeOutput>\n", TenX_BoolToString( workspaceOptions->captureExeOutput ) ) );
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t<AllowAutoErrorUnderlining>%s</AllowAutoErrorUnderlining>\n", TenX_BoolToString( workspaceOptions->allowAutoErrorUnderlining ) ) );

	// config names

	if (options->configs.count > 0) {
		StringBuilder_Appendf(scratch.arena, &workspaceFileContent, "\t\t<Configurations>\n");
		for (buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
			for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
				if ( chunk->items[configIndex]->name != 0 ) {
					StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<Configuration>%s</Configuration>\n", chunk->items[configIndex]->name ) );
				}
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</Configurations>\n" );
	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Configurations></Configurations>" );
	}

	// platforms

	if ( workspaceOptions->platformCount > 0 ) {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<Platforms>\n" );
		for ( uint32_t platformIndex = 0; platformIndex < workspaceOptions->platformCount; platformIndex++ ) {
			TenXPlatform *platform = &workspaceOptions->platforms[platformIndex];

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

	for ( builderStringChunk_t *definesChunk = workspaceOptions->globalDefines.head; definesChunk; definesChunk = definesChunk->next ) {
		for ( uint32_t defineIndex = 0; defineIndex < definesChunk->count; defineIndex++ ) {
			StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t<Define>%s</Define>\n", definesChunk->items[defineIndex] ) );
		}
	}
	StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</Defines>\n" );

	// config properties

	if (options->configs.count > 0)
	{
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ConfigProperties>\n" );
		for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
			for ( uint32_t configIndex = 0; configIndex < chunk->count; configIndex++ ) {
				BuildConfig *config = chunk->items[configIndex];

				for ( uint32_t platformIndex = 0; platformIndex < workspaceOptions->platformCount; platformIndex++ ) {
					TenXPlatform *platform = &workspaceOptions->platforms[platformIndex];

					if ( !Builder_StringIsEmpty( platform->name ) )
					{
						//<Name>Debug:x64</Name>
						//<BuildCommand></BuildCommand>
						//<RebuildCommand></RebuildCommand>
						//<BuildFileCommand></BuildFileCommand>
						//<CleanCommand></CleanCommand>
						//<BuildWorkingDirectory></BuildWorkingDirectory>
						//<FileSyntaxCheckCommand></FileSyntaxCheckCommand>
						//<CancelBuild></CancelBuild>
						//<Exe></Exe>
						//<Args></Args>
						//<WorkingDirectory></WorkingDirectory>
						//<DebugCommand></DebugCommand>
						//<DebuggerExe></DebuggerExe>
						//<DebugSln></DebugSln>
						//<Defines></Defines>
						//<ForceIncludes></ForceIncludes>
						//<PrioritiseFiles></PrioritiseFiles>

						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t<ConfigAndPlatform>\n" );

						const char * name = Builder_FormatString( scratch.arena, "%s:%s", config->name, platform->name );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Name>%s</Name>\n", name ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildCommand>%s</BuildCommand>\n", TenX_StringOrEmpty( workspaceOptions->buildCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<RebuildCommand>%s</RebuildCommand>\n", TenX_StringOrEmpty( workspaceOptions->rebuildCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildFileCommand>%s</BuildFileCommand>\n", TenX_StringOrEmpty( workspaceOptions->buildFileCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<CleanCommand>%s</CleanCommand>\n", TenX_StringOrEmpty( workspaceOptions->cleanCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<BuildWorkingDirectory>%s</BuildWorkingDirectory>\n", TenX_StringOrEmpty( workspaceOptions->buildWorkingDirectory ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<FileSyntaxCheckCommand>%s</FileSyntaxCheckCommand>\n", TenX_StringOrEmpty( workspaceOptions->fileSyntaxCheckCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<CancelBuild>%s</CancelBuild>\n", TenX_StringOrEmpty( workspaceOptions->cancelCommand ) ) );
						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t<Exe>%s</Exe>\n", TenX_StringOrEmpty( workspaceOptions->executablePath ) ) );

						if ( workspaceOptions->args.count > 0 ) {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Args>\n" );
							for ( builderStringChunk_t *argsChunk = workspaceOptions->args.head; argsChunk; argsChunk = argsChunk->next ) {
								for ( uint32_t argIndex = 0; argIndex < argsChunk->count; argIndex++ ) {
									StringBuilder_Appendf( scratch.arena, &workspaceFileContent, Builder_FormatString( scratch.arena, "\t\t\t\t\t<Arg>%s</Arg>\n", argsChunk->items[argIndex] ) );
								}
							}
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t</Args>\n" );
						} else {
							StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t\t<Args></Args>\n" );
						}

						StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t\t</ConfigAndPlatform>\n" );

						// C:\dev\Builder_dev\scripts>C:\dev\Builder_dev\tests\builder_tests.exe -t Test10xWorkspace
					}
				}
			}
		}
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t</ConfigProperties>\n" );
	} else {
		StringBuilder_Appendf( scratch.arena, &workspaceFileContent, "\t\t<ConfigProperties></ConfigProperties>\n" );
	}

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
