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

===========================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "builder.h"

typedef struct VSCodeTaskConfig {
	// The config you want Builder to build through VS Code.
	BuildConfig	*config;

	// Any additional args you want to send to Builder when building this config.
	StringList	additionalBuildArgs;
} VSCodeTaskConfig;

typedef enum VSCodeDebuggerType {
	VSCODE_DEBUGGER_TYPE_UNSET	= 0,
	VSCODE_DEBUGGER_TYPE_CPPDBG_GDB,	// Linux/Mac: cppdbg with MIMode gdb
	VSCODE_DEBUGGER_TYPE_CPPDBG_LLDB,	// Linux/Mac: cppdbg with MIMode lldb
	VSCODE_DEBUGGER_TYPE_CPPVSDBG,		// Windows: cppvsdbg (MSVC debugger)
} VSCodeDebuggerType;

// Builder does not currently support MacOS, so there are no MacOS IntelliSense modes here.
typedef enum VSCodeIntelliSenseMode {
	VSCODE_INTELLISENSE_MODE_UNSET	= 0,
	VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64,
	VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X86,
	VSCODE_INTELLISENSE_MODE_LINUX_CLANG_ARM64,
	VSCODE_INTELLISENSE_MODE_LINUX_CLANG_ARM,
	VSCODE_INTELLISENSE_MODE_LINUX_GCC_X64,
	VSCODE_INTELLISENSE_MODE_LINUX_GCC_X86,
	VSCODE_INTELLISENSE_MODE_LINUX_GCC_ARM64,
	VSCODE_INTELLISENSE_MODE_LINUX_GCC_ARM,
	VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_X64,
	VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_X86,
	VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_ARM64,
	VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_ARM,
	VSCODE_INTELLISENSE_MODE_WINDOWS_CLANG_X64,
	VSCODE_INTELLISENSE_MODE_WINDOWS_CLANG_X86,
} VSCodeIntelliSenseMode;

typedef struct VSCodeDebuggerPlatformConfig {
	// The MI debug mode to use on this platform. E.g. "gdb" or "lldb".
	const char	*miMode;

	// The path to the debugger on this platform. E.g. "/usr/bin/gdb".
	const char	*miDebuggerPath;
} VSCodeDebuggerPlatformConfig;

typedef struct VSCodeSetupCommand {
	const char	*description;
	const char	*text;
	bool		ignoreFailures;
} VSCodeSetupCommand;

typedef struct VSCodeLaunchConfig {
	// The config you want to run when you select this launch config in VS Code.
	const char						*binaryName;

	// When you run this config, what command line arguments do you want to be passed through?
	StringList						args;

	// You'd never guess, but this sets the "cwd" field in a VS Code launch config.
	// Leave NULL to default to '${workspaceFolder}'.
	const char						*cwd;

	// Which VS Code debugger to use for this launch config.
	// Defaults to VSCODE_DEBUGGER_TYPE_CPPDBG_GDB if unset.
	VSCodeDebuggerType				debuggerType;

	// Platform-specific debugger config for Linux.
	// When set, Builder emits a "linux": { ... } block with MIMode and miDebuggerPath
	// instead of putting MIMode at the top level of the config.
	VSCodeDebuggerPlatformConfig	linuxDebugger;

	// Platform-specific debugger config for Windows.
	// When set, Builder emits a "windows": { ... } block with MIMode and miDebuggerPath
	// instead of putting MIMode at the top level of the config.
	VSCodeDebuggerPlatformConfig	windowsDebugger;

	// GDB/LLDB MI commands to send to the debugger during initialisation,
	// before attaching to or launching the program.
	VSCodeSetupCommand				*setupCommands;
	uint32_t						setupCommandsCount;
} VSCodeLaunchConfig;

typedef struct VSCodeCppPropertiesConfig {
	// Overrides config->name as the configuration name in c_cpp_properties.json.
	// Use this when the same BuildConfig is needed for multiple platforms/compilers
	// (e.g. one entry named "Linux" and one named "Win32" from the same config).
	// If NULL, config->name is used.
	const char				*name;

	// The config from which Builder extracts the IntelliSense settings.
	// Builder uses: config->additionalIncludes (includePath), config->defines,
	// and config->languageVersion (cStandard or cppStandard).
	BuildConfig				*config;

	// The IntelliSense mode to use.
	// If unset, the "intelliSenseMode" field is omitted from the output.
	VSCodeIntelliSenseMode	intelliSenseMode;
} VSCodeCppPropertiesConfig;

typedef struct VSCodeJSONOptions {
	// The command that VS Code will invoke when running a task - this is your build script's own
	// compiled binary (there is no separate standalone "Builder" executable).
	// Leave NULL to default to argv[0], i.e. however this build script was itself invoked.
	const char					*buildCommand;

	// The configs that will go into c_cpp_properties.json.
	// Builder will also use BuilderOptions::compilerPath for the "compilerPath" field.
	// Leave NULL (and cppPropertiesConfigsCount at 0) to default to one entry per BuilderOptions::configs entry.
	VSCodeCppPropertiesConfig	*cppPropertiesConfigs;
	uint32_t					cppPropertiesConfigsCount;

	// The configs that will go into tasks.json.
	// Leave NULL (and taskConfigsCount at 0) to default to one task per BuilderOptions::configs entry.
	VSCodeTaskConfig			*taskConfigs;
	uint32_t					taskConfigsCount;

	// The configs that will go into launch.json.
	// Leave NULL (and launchConfigsCount at 0) to default to one launch config per BuilderOptions::configs entry.
	VSCodeLaunchConfig			*launchConfigs;
	uint32_t					launchConfigsCount;
} VSCodeJSONOptions;

bool	Builder_GenerateVSCodeJSONFiles( BuilderOptions *options, VSCodeJSONOptions *vsCodeOptions, int argc, char **argv );


#ifdef BUILDER_VS_CODE_IMPLEMENTATION

#if !defined( BUILDER_IMPLEMENTATION )
#error "BUILDER_VS_CODE_IMPLEMENTATION requires BUILDER_IMPLEMENTATION to also be defined, and \"builder.h\" to be included before \"builder_vs_code.h\", in this translation unit."
#endif

bool Builder_GenerateVSCodeJSONFiles( BuilderOptions *options, VSCodeJSONOptions *vsCodeOptions, int argc, char **argv ) {
	BUILDER_ASSERT( options );
	BUILDER_ASSERT( vsCodeOptions );

	const char *dotVSCodeFolder = ".vscode";

	if ( !Builder_CreateFolderIfItDoesntExist( dotVSCodeFolder ) ) {
		Builder_Error( "Failed to create \"%s\" folder.\n", dotVSCodeFolder );
		return false;
	}

	BUILDER_ASSERT( argc > 0 && argv );

	// everything built in here goes into a file and is then done with - nothing outlives this call
	scratch_t scratch = Builder_GetScratch( NULL );

	const char *buildCommand = ( vsCodeOptions->buildCommand && vsCodeOptions->buildCommand[0] ) ? vsCodeOptions->buildCommand : argv[0];

	// c_cpp_properties.json
	{
		if ( vsCodeOptions->cppPropertiesConfigsCount == 0 ) {
			vsCodeOptions->cppPropertiesConfigs = Builder_ArenaAlloc( scratch.arena, VSCodeCppPropertiesConfig, options->configs.count );
			vsCodeOptions->cppPropertiesConfigsCount = options->configs.count;

			uint32_t configIndex = 0;

			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
				for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < chunk->count; chunkConfigIndex++ ) {
					vsCodeOptions->cppPropertiesConfigs[configIndex++] = (VSCodeCppPropertiesConfig) {
						.config = chunk->items[chunkConfigIndex],
					};
				}
			}
		}

		char *cppPropertiesJSONFilename = Builder_FormatString( scratch.arena, "%s%cc_cpp_properties.json", dotVSCodeFolder, BUILDER_PATH_SEPARATOR );

		printf( "Generating %s ... ", cppPropertiesJSONFilename );

		stringBuilder_t cppPropertiesJSONContent = { 0 };

		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "// This file was auto-generated by Builder.\n" );
		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "{\n" );
		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\"configurations\": [\n" );

		for ( uint32_t configIndex = 0; configIndex < vsCodeOptions->cppPropertiesConfigsCount; configIndex++ ) {
			VSCodeCppPropertiesConfig *cppPropertiesConfig = &vsCodeOptions->cppPropertiesConfigs[configIndex];
			BuildConfig *config = cppPropertiesConfig->config;

			const char *configName = ( cppPropertiesConfig->name && cppPropertiesConfig->name[0] ) ? cppPropertiesConfig->name : config->name;

			StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t{\n" );
			StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"name\": \"%s\",\n", configName );

			if ( config->additionalIncludes.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"includePath\": [\n" );

				for ( builderStringChunk_t *chunk = config->additionalIncludes.head; chunk; chunk = chunk->next ) {
					for ( uint32_t includeIndex = 0; includeIndex < chunk->count; includeIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\t\"%s\",\n", chunk->items[includeIndex] );
					}
				}

				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t],\n" );
			}

			if ( config->defines.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"defines\": [\n" );

				for ( builderStringChunk_t *chunk = config->defines.head; chunk; chunk = chunk->next ) {
					for ( uint32_t defineIndex = 0; defineIndex < chunk->count; defineIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\t\"%s\",\n", chunk->items[defineIndex] );
					}
				}

				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t],\n" );
			}

			if ( options->compilerPath && options->compilerPath[0] ) {
				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"compilerPath\": \"%s\",\n", options->compilerPath );
			}

			if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
				const char *standardKey = NULL;

				switch ( config->languageVersion ) {
					case LANGUAGE_VERSION_C89:
					case LANGUAGE_VERSION_C99:
					case LANGUAGE_VERSION_C11:
					case LANGUAGE_VERSION_C17:
					case LANGUAGE_VERSION_C23:
						standardKey = "cStandard";
						break;

					case LANGUAGE_VERSION_CPP11:
					case LANGUAGE_VERSION_CPP14:
					case LANGUAGE_VERSION_CPP17:
					case LANGUAGE_VERSION_CPP20:
					case LANGUAGE_VERSION_CPP23:
						standardKey = "cppStandard";
						break;

					case LANGUAGE_VERSION_UNSET:
						break;
				}

				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"%s\": \"%s\",\n", standardKey, GetLanguageVersionString( config->languageVersion ) );
			}

			if ( cppPropertiesConfig->intelliSenseMode != VSCODE_INTELLISENSE_MODE_UNSET ) {
				const char *intelliSenseModeString = NULL;

				switch ( cppPropertiesConfig->intelliSenseMode ) {
					case VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64:		intelliSenseModeString = "linux-clang-x64";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X86:		intelliSenseModeString = "linux-clang-x86";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_CLANG_ARM64:	intelliSenseModeString = "linux-clang-arm64";	break;
					case VSCODE_INTELLISENSE_MODE_LINUX_CLANG_ARM:		intelliSenseModeString = "linux-clang-arm";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_GCC_X64:		intelliSenseModeString = "linux-gcc-x64";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_GCC_X86:		intelliSenseModeString = "linux-gcc-x86";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_GCC_ARM64:		intelliSenseModeString = "linux-gcc-arm64";		break;
					case VSCODE_INTELLISENSE_MODE_LINUX_GCC_ARM:		intelliSenseModeString = "linux-gcc-arm";		break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_X64:		intelliSenseModeString = "windows-msvc-x64";	break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_X86:		intelliSenseModeString = "windows-msvc-x86";	break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_ARM64:	intelliSenseModeString = "windows-msvc-arm64";	break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_MSVC_ARM:		intelliSenseModeString = "windows-msvc-arm";	break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_CLANG_X64:	intelliSenseModeString = "windows-clang-x64";	break;
					case VSCODE_INTELLISENSE_MODE_WINDOWS_CLANG_X86:	intelliSenseModeString = "windows-clang-x86";	break;
					case VSCODE_INTELLISENSE_MODE_UNSET:
						break;
				}

				StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t\t\"intelliSenseMode\": \"%s\",\n", intelliSenseModeString );
			}

			StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\t},\n" );
		}

		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t],\n" );
		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "\t\"version\": 4\n" );
		StringBuilder_Appendf( scratch.arena, &cppPropertiesJSONContent, "}\n" );

		uint64_t length;
		char *cppPropertiesJSONString = StringBuilder_ToString( scratch.arena, &cppPropertiesJSONContent, &length );

		bool wroteFile = Builder_WriteEntireFile( cppPropertiesJSONFilename, cppPropertiesJSONString, length );

		if ( wroteFile ) {
			printf( "Done\n" );
		} else {
			Builder_Error( "Failed to write \"%s\".\n", cppPropertiesJSONFilename );
		}

		if ( !wroteFile ) {
			Builder_RewindScratch( &scratch );
			return false;
		}
	}

	// tasks.json
	{
		if ( vsCodeOptions->taskConfigsCount == 0 ) {
			vsCodeOptions->taskConfigs = Builder_ArenaAlloc( scratch.arena, VSCodeTaskConfig, options->configs.count );
			vsCodeOptions->taskConfigsCount = options->configs.count;

			uint32_t configIndex = 0;

			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
				for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < chunk->count; chunkConfigIndex++ ) {
					vsCodeOptions->taskConfigs[configIndex++] = (VSCodeTaskConfig) {
						.config = chunk->items[chunkConfigIndex],
					};
				}
			}
		}

		char *tasksJSONFilename = Builder_FormatString( scratch.arena, "%s%ctasks.json", dotVSCodeFolder, BUILDER_PATH_SEPARATOR );

		printf( "Generating %s ... ", tasksJSONFilename );

		stringBuilder_t tasksJSONContent = { 0 };

		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "{\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t// See https://go.microsoft.com/fwlink/?LinkId=733558\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t// for the documentation about the tasks.json format\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t// This file was auto-generated by Builder.\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\"version\": \"2.0.0\",\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\"tasks\": [\n" );

		for ( uint32_t configIndex = 0; configIndex < vsCodeOptions->taskConfigsCount; configIndex++ ) {
			VSCodeTaskConfig *taskConfig = &vsCodeOptions->taskConfigs[configIndex];

			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t{\n" );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"label\": \"Build %s\",\n", taskConfig->config->name );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"type\": \"shell\",\n" );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"command\": \"%s\",\n", buildCommand );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"args\": [\n" );
			{
				StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\t\"%s%s\"", ARG_CONFIG, taskConfig->config->name );

				if ( taskConfig->additionalBuildArgs.count > 0 ) {
					StringBuilder_Appendf( scratch.arena, &tasksJSONContent, ",\n" );

					uint32_t argIndex = 0;

					for ( builderStringChunk_t *argChunk = taskConfig->additionalBuildArgs.head; argChunk; argChunk = argChunk->next ) {
						for ( uint32_t chunkArgIndex = 0; chunkArgIndex < argChunk->count; chunkArgIndex++ ) {
							StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\t\"%s\"", argChunk->items[chunkArgIndex] );

							argIndex++;

							StringBuilder_Appendf( scratch.arena, &tasksJSONContent, argIndex < taskConfig->additionalBuildArgs.count ? ",\n" : "\n" );
						}
					}
				} else {
					StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\n" );
				}
			}
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t],\n" );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t},\n" );
		}

		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t]\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "}\n" );

		uint64_t length;
		char *tasksJSONString = StringBuilder_ToString( scratch.arena, &tasksJSONContent, &length );

		bool wroteFile = Builder_WriteEntireFile( tasksJSONFilename, tasksJSONString, length );

		if ( wroteFile ) {
			printf( "Done\n" );
		} else {
			Builder_Error( "Failed to write \"%s\".\n", tasksJSONFilename );
		}

		if ( !wroteFile ) {
			Builder_RewindScratch( &scratch );
			return false;
		}
	}

	// launch.json
	{

		if ( vsCodeOptions->launchConfigsCount == 0 ) {
			vsCodeOptions->launchConfigs = Builder_ArenaAlloc( scratch.arena, VSCodeLaunchConfig, options->configs.count );
			vsCodeOptions->launchConfigsCount = options->configs.count;

			uint32_t configIndex = 0;

			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
				for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < chunk->count; chunkConfigIndex++ ) {
					const BuildConfig *config = chunk->items[chunkConfigIndex];

					const char *extension = Builder_GetFileExtensionFromBinaryType( config->binaryType );

					char *fullBinaryPath = config->binaryFolder
						? Builder_FormatString( scratch.arena, "%s%c%s%s", config->binaryFolder, BUILDER_PATH_SEPARATOR, config->binaryName, extension )
						: Builder_FormatString( scratch.arena, "%s%s", config->binaryName, extension );

					// vs code wants forward slashes regardless of platform
					for ( char *c = fullBinaryPath; *c; c++ ) {
						if ( *c == '\\' ) {
							*c = '/';
						}
					}

					VSCodeLaunchConfig launchConfig = {
						.binaryName	= Builder_FormatString( scratch.arena, "${workspaceFolder}/%s", fullBinaryPath ),
						.cwd		= "${workspaceFolder}",
					};

#if defined( _WIN32 )
					launchConfig.debuggerType = VSCODE_DEBUGGER_TYPE_CPPVSDBG;
#elif defined( __linux__ )
					launchConfig.linuxDebugger = (VSCodeDebuggerPlatformConfig) {
						.miMode			= "gdb",
						.miDebuggerPath	= "/usr/bin/gdb",
					};
#endif

					vsCodeOptions->launchConfigs[configIndex++] = launchConfig;
				}
			}
		}

		char *launchJSONFilename = Builder_FormatString( scratch.arena, "%s%claunch.json", dotVSCodeFolder, BUILDER_PATH_SEPARATOR );

		printf( "Generating %s ... ", launchJSONFilename );

		stringBuilder_t launchJSONContent = { 0 };

		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "{\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t// Use IntelliSense to learn about possible attributes.\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t// Hover to view descriptions of existing attributes.\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t// For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t// This file was auto-generated by Builder.\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\"version\": \"0.2.0\",\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\"configurations\": [\n" );

		for ( uint32_t configIndex = 0; configIndex < vsCodeOptions->launchConfigsCount; configIndex++ ) {
			VSCodeLaunchConfig *launchConfig = &vsCodeOptions->launchConfigs[configIndex];

			StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t{\n" );
			StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"name\": \"%s\",\n", launchConfig->binaryName );
			{
				VSCodeDebuggerType debuggerType = ( launchConfig->debuggerType == VSCODE_DEBUGGER_TYPE_UNSET ) ? VSCODE_DEBUGGER_TYPE_CPPDBG_GDB : launchConfig->debuggerType;

				bool hasPlatformConfigs = ( launchConfig->linuxDebugger.miMode && launchConfig->linuxDebugger.miMode[0] )
					|| ( launchConfig->windowsDebugger.miMode && launchConfig->windowsDebugger.miMode[0] );

				switch ( debuggerType ) {
					case VSCODE_DEBUGGER_TYPE_CPPDBG_GDB:
						StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"type\": \"cppdbg\",\n" );
						if ( !hasPlatformConfigs ) {
							StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"MIMode\": \"gdb\",\n" );
						}
						break;

					case VSCODE_DEBUGGER_TYPE_CPPDBG_LLDB:
						StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"type\": \"cppdbg\",\n" );
						if ( !hasPlatformConfigs ) {
							StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"MIMode\": \"lldb\",\n" );
						}
						break;

					case VSCODE_DEBUGGER_TYPE_CPPVSDBG:
						StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"type\": \"cppvsdbg\",\n" );
						break;

					case VSCODE_DEBUGGER_TYPE_UNSET:
						break;
				}
			}

			StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"request\": \"launch\",\n" );
			StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"program\": \"%s\",\n", launchConfig->binaryName );

			if ( launchConfig->args.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"args\": [\n" );

				uint32_t argIndex = 0;

				for ( builderStringChunk_t *argChunk = launchConfig->args.head; argChunk; argChunk = argChunk->next ) {
					for ( uint32_t chunkArgIndex = 0; chunkArgIndex < argChunk->count; chunkArgIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\"%s\"", argChunk->items[chunkArgIndex] );

						argIndex++;

						StringBuilder_Appendf( scratch.arena, &launchJSONContent, argIndex < launchConfig->args.count ? ",\n" : "\n" );
					}
				}

				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t],\n" );
			}

			{
				const char *cwd = ( launchConfig->cwd && launchConfig->cwd[0] ) ? launchConfig->cwd : "${workspaceFolder}";
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"cwd\": \"%s\",\n", cwd );
			}

			if ( launchConfig->linuxDebugger.miMode && launchConfig->linuxDebugger.miMode[0] ) {
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"linux\": {\n" );
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\"MIMode\": \"%s\",\n", launchConfig->linuxDebugger.miMode );

				if ( launchConfig->linuxDebugger.miDebuggerPath && launchConfig->linuxDebugger.miDebuggerPath[0] ) {
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\"miDebuggerPath\": \"%s\",\n", launchConfig->linuxDebugger.miDebuggerPath );
				}

				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t},\n" );
			}

			if ( launchConfig->windowsDebugger.miMode && launchConfig->windowsDebugger.miMode[0] ) {
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"windows\": {\n" );
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\"MIMode\": \"%s\",\n", launchConfig->windowsDebugger.miMode );

				if ( launchConfig->windowsDebugger.miDebuggerPath && launchConfig->windowsDebugger.miDebuggerPath[0] ) {
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\"miDebuggerPath\": \"%s\",\n", launchConfig->windowsDebugger.miDebuggerPath );
				}

				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t},\n" );
			}

			if ( launchConfig->setupCommandsCount > 0 ) {
				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\"setupCommands\": [\n" );

				for ( uint32_t cmdIndex = 0; cmdIndex < launchConfig->setupCommandsCount; cmdIndex++ ) {
					const VSCodeSetupCommand *cmd = &launchConfig->setupCommands[cmdIndex];

					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t{\n" );
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\t\"description\": \"%s\",\n", cmd->description );
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\t\"text\": \"%s\",\n", cmd->text );
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t\t\"ignoreFailures\": %s,\n", cmd->ignoreFailures ? "true" : "false" );
					StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t\t},\n" );
				}

				StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t\t],\n" );
			}

			StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t\t},\n" );
		}

		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "\t]\n" );
		StringBuilder_Appendf( scratch.arena, &launchJSONContent, "}\n" );

		uint64_t length;
		char *launchJSONString = StringBuilder_ToString( scratch.arena, &launchJSONContent, &length );

		bool wroteFile = Builder_WriteEntireFile( launchJSONFilename, launchJSONString, length );

		if ( wroteFile ) {
			printf( "Done\n" );
		} else {
			Builder_Error( "Failed to write \"%s\".\n", launchJSONFilename );
		}

		if ( !wroteFile ) {
			Builder_RewindScratch( &scratch );
			return false;
		}
	}

	printf( "\n" );

	Builder_RewindScratch( &scratch );

	return true;
}

#endif // BUILDER_VS_CODE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
