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

typedef struct ZedTaskConfig {
	// The config you want Builder to build through Zed.
	BuildConfig	*config;

	// When you run this config, what command line arguments do you want to be passed through?
	StringList	args;
} ZedTaskConfig;

typedef enum ZedDebuggerAdapter {
	ZED_DEBUGGER_ADAPTER_CODELLDB	= 0,
	ZED_DEBUGGER_ADAPTER_GDB,
} ZedDebuggerAdapter;

typedef enum ZedDebuggerRequest {
	ZED_DEBUGGER_REQUEST_LAUNCH	= 0,
	ZED_DEBUGGER_REQUEST_ATTACH,
} ZedDebuggerRequest;

typedef struct ZedDebugConfig {
	const char			*label;

	const char			*binaryName;

	// When you run this debug config, what command line arguments do you want to be passed through?
	StringList			args;

	// You'd never guess, but this sets the "cwd" field in a Zed debug config.
	// Leave NULL to default to '${ZED_WORKTREE_ROOT}'.
	const char			*cwd;

	// Which debugger adapter do you want to use?
	ZedDebuggerAdapter	adapter;

	// When you run this debug config, do you want to launch the executable or attach to it?
	ZedDebuggerRequest	request;
} ZedDebugConfig;

typedef struct ZedJSONOptions {
	// The command that Zed will invoke when running a task - this is your build script's own
	// compiled binary (there is no separate standalone "Builder" executable).
	// Leave NULL to default to argv[0], i.e. however this build script was itself invoked.
	const char		*buildCommand;

	// The configs that will go into tasks.json.
	// Leave NULL (and taskConfigsCount at 0) to default to one task per BuilderOptions::configs entry.
	ZedTaskConfig	*taskConfigs;
	uint32_t		taskConfigsCount;

	// The configs that will go into debug.json.
	// Leave NULL (and debugConfigsCount at 0) to default to one debug config per BuilderOptions::configs entry.
	ZedDebugConfig	*debugConfigs;
	uint32_t		debugConfigsCount;
} ZedJSONOptions;

bool	Builder_GenerateZedJSONFiles( BuilderOptions *options, ZedJSONOptions *zedOptions, int argc, char **argv );


#ifdef BUILDER_ZED_IMPLEMENTATION

#if !defined( BUILDER_IMPLEMENTATION )
#error "BUILDER_ZED_IMPLEMENTATION requires BUILDER_IMPLEMENTATION to also be defined, and \"builder.h\" to be included before \"builder_zed.h\", in this translation unit."
#endif

bool Builder_GenerateZedJSONFiles( BuilderOptions *options, ZedJSONOptions *zedOptions, int argc, char **argv ) {
	BUILDER_ASSERT( options );
	BUILDER_ASSERT( zedOptions );

	const char *dotZedFolder = ".zed";

	if ( !Builder_CreateFolderIfItDoesntExist( dotZedFolder ) ) {
		Builder_Error( "Failed to create \"%s\" folder.\n", dotZedFolder );
		return false;
	}

	BUILDER_ASSERT( argc > 0 && argv );

	// everything built in here goes into a file and is then done with - nothing outlives this call
	scratch_t scratch = Builder_GetScratch( NULL );

	const char *buildCommand = ( zedOptions->buildCommand && zedOptions->buildCommand[0] ) ? zedOptions->buildCommand : argv[0];

	// tasks.json
	{
		if ( zedOptions->taskConfigsCount == 0 ) {
			zedOptions->taskConfigs = Builder_ArenaAlloc( scratch.arena, ZedTaskConfig, options->configs.count );
			zedOptions->taskConfigsCount = options->configs.count;

			uint32_t configIndex = 0;

			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
				for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < chunk->count; chunkConfigIndex++ ) {
					zedOptions->taskConfigs[configIndex++] = (ZedTaskConfig) {
						.config	= chunk->items[chunkConfigIndex],
					};
				}
			}
		}

		char *tasksJSONFilename = Builder_FormatString( scratch.arena, "%s%ctasks.json", dotZedFolder, BUILDER_PATH_SEPARATOR );

		printf( "Generating %s ... ", tasksJSONFilename );

		stringBuilder_t tasksJSONContent = { 0 };

		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "// Project tasks configuration. See https://zed.dev/docs/tasks for documentation.\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "// This file was auto-generated by Builder.\n" );
		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "[\n" );

		for ( uint32_t configIndex = 0; configIndex < zedOptions->taskConfigsCount; configIndex++ ) {
			ZedTaskConfig *taskConfig = &zedOptions->taskConfigs[configIndex];

			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t{\n" );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\"label\": \"Build %s\",\n", taskConfig->config->name );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\"command\": \"%s\",\n", buildCommand );
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\"args\": [\n" );
			{
				StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"%s%s\"", ARG_CONFIG, taskConfig->config->name );

				if ( taskConfig->args.count > 0 ) {
					StringBuilder_Appendf( scratch.arena, &tasksJSONContent, ",\n" );

					uint32_t argIndex = 0;

					for ( builderStringChunk_t *argChunk = taskConfig->args.head; argChunk; argChunk = argChunk->next ) {
						for ( uint32_t chunkArgIndex = 0; chunkArgIndex < argChunk->count; chunkArgIndex++ ) {
							StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t\t\"%s\"", argChunk->items[chunkArgIndex] );

							argIndex++;

							StringBuilder_Appendf( scratch.arena, &tasksJSONContent, argIndex < taskConfig->args.count ? ",\n" : "\n" );
						}
					}
				} else {
					StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\n" );
				}
			}
			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "\t\t]\n" );

			StringBuilder_Appendf( scratch.arena, &tasksJSONContent, configIndex < zedOptions->taskConfigsCount - 1 ? "\t},\n" : "\t}\n" );
		}

		StringBuilder_Appendf( scratch.arena, &tasksJSONContent, "]\n" );

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

	// debug.json
	{
		bool ok = true;

		if ( zedOptions->debugConfigsCount == 0 ) {
			zedOptions->debugConfigs = Builder_ArenaAlloc( scratch.arena, ZedDebugConfig, options->configs.count );
			zedOptions->debugConfigsCount = options->configs.count;

			uint32_t configIndex = 0;

			for ( buildConfigPtrChunk_t *chunk = options->configs.head; chunk; chunk = chunk->next ) {
				for ( uint32_t chunkConfigIndex = 0; chunkConfigIndex < chunk->count; chunkConfigIndex++ ) {
					const BuildConfig *config = chunk->items[chunkConfigIndex];

					const char *extension = Builder_GetFileExtensionFromBinaryType( config->binaryType );

					char *fullBinaryPath = config->binaryFolder
						? Builder_FormatString( scratch.arena, "%s%c%s%s", config->binaryFolder, BUILDER_PATH_SEPARATOR, config->binaryName, extension )
						: Builder_FormatString( scratch.arena, "%s%s", config->binaryName, extension );

					// Zed wants forward slashes regardless of platform
					for ( char *c = fullBinaryPath; *c; c++ ) {
						if ( *c == '\\' ) {
							*c = '/';
						}
					}

					zedOptions->debugConfigs[configIndex++] = (ZedDebugConfig) {
						.label		= Builder_FormatString( scratch.arena, "Debug %s", config->name ),
						.binaryName	= Builder_FormatString( scratch.arena, "${ZED_WORKTREE_ROOT}/%s", fullBinaryPath ),
						.cwd		= "${ZED_WORKTREE_ROOT}",
						.adapter	= ZED_DEBUGGER_ADAPTER_CODELLDB,
						.request	= ZED_DEBUGGER_REQUEST_LAUNCH,
					};
				}
			}
		}

		char *debugJSONFilename = Builder_FormatString( scratch.arena, "%s%cdebug.json", dotZedFolder, BUILDER_PATH_SEPARATOR );

		printf( "Generating %s ... ", debugJSONFilename );

		stringBuilder_t debugJSONContent = { 0 };

		StringBuilder_Appendf( scratch.arena, &debugJSONContent, "// Project-local debug tasks.\n" );
		StringBuilder_Appendf( scratch.arena, &debugJSONContent, "// For more documentation on how to configure debug tasks,\n" );
		StringBuilder_Appendf( scratch.arena, &debugJSONContent, "// see: https://zed.dev/docs/debugger\n" );
		StringBuilder_Appendf( scratch.arena, &debugJSONContent, "// This file was auto-generated by Builder.\n" );
		StringBuilder_Appendf( scratch.arena, &debugJSONContent, "[\n" );

		for ( uint32_t debugConfigIndex = 0; debugConfigIndex < zedOptions->debugConfigsCount; debugConfigIndex++ ) {
			ZedDebugConfig *debugConfig = &zedOptions->debugConfigs[debugConfigIndex];

			if ( !debugConfig->label || !debugConfig->label[0] ) {
				Builder_Error( "When generating Zed debug configs (for your debug.json), the label must be set to something.  It cannot be empty.\n" );
				ok = false;
				break;
			}

			StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t{\n" );
			StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"label\": \"%s\",\n", debugConfig->label );
			StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"program\": \"%s\",\n", debugConfig->binaryName );

			if ( debugConfig->args.count > 0 ) {
				StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"args\": [\n" );

				uint32_t argIndex = 0;

				for ( builderStringChunk_t *argChunk = debugConfig->args.head; argChunk; argChunk = argChunk->next ) {
					for ( uint32_t chunkArgIndex = 0; chunkArgIndex < argChunk->count; chunkArgIndex++ ) {
						StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\t\"%s\"", argChunk->items[chunkArgIndex] );

						argIndex++;

						StringBuilder_Appendf( scratch.arena, &debugJSONContent, argIndex < debugConfig->args.count ? ",\n" : "\n" );
					}
				}

				StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t],\n" );
			}

			{
				const char *cwd = ( debugConfig->cwd && debugConfig->cwd[0] ) ? debugConfig->cwd : "${ZED_WORKTREE_ROOT}";
				StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"cwd\": \"%s\",\n", cwd );
			}

			{
				const char *adapterStr = NULL;

				switch ( debugConfig->adapter ) {
					case ZED_DEBUGGER_ADAPTER_CODELLDB:	adapterStr = "CodeLLDB";	break;
					case ZED_DEBUGGER_ADAPTER_GDB:		adapterStr = "GDB";			break;
				}

				StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"adapter\": \"%s\",\n", adapterStr );
			}

			{
				const char *requestStr = NULL;

				switch ( debugConfig->request ) {
					case ZED_DEBUGGER_REQUEST_LAUNCH:	requestStr = "launch";	break;
					case ZED_DEBUGGER_REQUEST_ATTACH:	requestStr = "attach";	break;
				}

				StringBuilder_Appendf( scratch.arena, &debugJSONContent, "\t\t\"request\": \"%s\"\n", requestStr );
			}

			StringBuilder_Appendf( scratch.arena, &debugJSONContent, debugConfigIndex < zedOptions->debugConfigsCount - 1 ? "\t},\n" : "\t}\n" );
		}

		if ( ok ) {
			StringBuilder_Appendf( scratch.arena, &debugJSONContent, "]\n" );

			uint64_t length;
			char *debugJSONString = StringBuilder_ToString( scratch.arena, &debugJSONContent, &length);

			if ( !Builder_WriteEntireFile( debugJSONFilename, debugJSONString, length ) ) {
				Builder_Error( "Failed to write \"%s\".\n", debugJSONFilename );
				ok = false;
			} else {
				printf( "Done\n" );
			}

		}

		if ( !ok ) {
			Builder_RewindScratch( &scratch );
			return false;
		}
	}

	printf( "\n" );

	Builder_RewindScratch( &scratch );

	return true;
}

#endif // BUILDER_ZED_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
