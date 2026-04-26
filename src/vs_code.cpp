#include "builder_local.h"

#include "core/include/string_builder.h"
#include "core/include/file.h"
#include "core/include/paths.h"
#include "core/include/string_helpers.h"
#include "core/include/debug.h"

bool8 GenerateVSCodeJSONFiles( buildContext_t *context, BuilderOptions *options ) {
	const char *dotVSCodeFolder = tprintf( "%s%c.vscode", context->inputFilePath.data, PATH_SEPARATOR );

	if ( !folder_create_if_it_doesnt_exist( dotVSCodeFolder ) ) {
		error( "Failed to create .vscode folder at \"%s\".\n", context->inputFilePath.data );
		return false;
	}

	const char *builderPath = options->vsCodeJSONOptions.builderPath.empty() ? BUILDER_PROGRAM_NAME : options->vsCodeJSONOptions.builderPath.c_str();

	// tasks.json
	{
		const char *tasksJSONFilename = tprintf( "%s%ctasks.json", dotVSCodeFolder, PATH_SEPARATOR );

		StringBuilder tasksJSONContent = {};
		defer( string_builder_destroy( &tasksJSONContent ) );

		string_builder_reset( &tasksJSONContent );

		string_builder_appendf( &tasksJSONContent, "{\n" );
		string_builder_appendf( &tasksJSONContent, "\t// See https://go.microsoft.com/fwlink/?LinkId=733558\n" );
		string_builder_appendf( &tasksJSONContent, "\t// for the documentation about the tasks.json format\n" );
		string_builder_appendf( &tasksJSONContent, "\t\"version\": \"2.0.0\",\n" );	// TODO(DM): 17/04/2026: do we need to expose the version to VSCodeJSONOptions?
		string_builder_appendf( &tasksJSONContent, "\t\"tasks\": [\n" );
		For ( u64, configIndex, 0, options->vsCodeJSONOptions.taskConfigs.size() ) {
			VSCodeTaskConfig *taskConfig = &options->vsCodeJSONOptions.taskConfigs[configIndex];

			BuildConfig *config = &taskConfig->config;

			string_builder_appendf( &tasksJSONContent, "\t\t{\n" );
			string_builder_appendf( &tasksJSONContent, "\t\t\t\"label\": \"Build %s\",\n", taskConfig->config.name.c_str() );
			string_builder_appendf( &tasksJSONContent, "\t\t\t\"type\": \"shell\",\n" );
			string_builder_appendf( &tasksJSONContent, "\t\t\t\"command\": \"%s\",\n", builderPath );
			string_builder_appendf( &tasksJSONContent, "\t\t\t\"args\": [\n" );
			{
				string_builder_appendf( &tasksJSONContent, "\t\t\t\t\"%s\",\n", context->inputFile );
				string_builder_appendf( &tasksJSONContent, "\t\t\t\t\"%s%s\"", ARG_CONFIG, config->name.c_str() );

				if ( taskConfig->additionalBuildArgs.size() ) {
					string_builder_appendf( &tasksJSONContent, ",\n" );

					For ( u32, argIndex, 0, taskConfig->additionalBuildArgs.size() ) {
						string_builder_appendf( &tasksJSONContent, "\t\t\t\t\"%s\"", taskConfig->additionalBuildArgs[argIndex].c_str() );

						if ( argIndex < taskConfig->additionalBuildArgs.size() - 1 ) {
							string_builder_appendf( &tasksJSONContent, ",\n" );
						} else {
							string_builder_appendf( &tasksJSONContent, "\n" );
						}
					}
				} else {
					string_builder_appendf( &tasksJSONContent, "\n" );
				}
			}
			string_builder_appendf( &tasksJSONContent, "\t\t\t],\n" );
			string_builder_appendf( &tasksJSONContent, "\t\t},\n" );
		}
		string_builder_appendf( &tasksJSONContent, "\t]\n" );
		string_builder_appendf( &tasksJSONContent, "}\n" );

		if ( !WriteStringBuilderToFile( &tasksJSONContent, tasksJSONFilename ) ) {
			error( "Failed to write \"%s\".\n", tasksJSONFilename );
			return false;
		}
	}

	// launch.json
	{
		const char *launchJSONFilename = tprintf( "%s%claunch.json", dotVSCodeFolder, PATH_SEPARATOR );

		StringBuilder launchJSONContent = {};
		defer( string_builder_destroy( &launchJSONContent ) );

		string_builder_reset( &launchJSONContent );

		string_builder_appendf( &launchJSONContent, "{\n" );
		string_builder_appendf( &launchJSONContent, "\t// Use IntelliSense to learn about possible attributes.\n" );
		string_builder_appendf( &launchJSONContent, "\t// Hover to view descriptions of existing attributes.\n" );
		string_builder_appendf( &launchJSONContent, "\t// For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\n" );
		string_builder_appendf( &launchJSONContent, "\t\"version\": \"0.2.0\",\n" );	// TODO(DM): 17/04/2026: do we need to expose the version to VSCodeJSONOptions?
		string_builder_appendf( &launchJSONContent, "\t\"configurations\": [\n" );

		For ( u32, configIndex, 0, options->vsCodeJSONOptions.launchConfigs.size() ) {
			VSCodeLaunchConfig *launchConfig = &options->vsCodeJSONOptions.launchConfigs[configIndex];

			string_builder_appendf( &launchJSONContent,          "\t\t{\n" );
			string_builder_appendf( &launchJSONContent, tprintf( "\t\t\t\"name\": \"%s\",\n", launchConfig->binaryName.c_str() ) );
			{
				VSCodeDebuggerType debuggerType = ( launchConfig->debuggerType == VSCODE_DEBUGGER_TYPE_UNSET ) ? VSCODE_DEBUGGER_TYPE_CPPDBG_GDB : launchConfig->debuggerType;

				bool8 hasPlatformConfigs = !launchConfig->linuxDebugger.miMode.empty() || !launchConfig->windowsDebugger.miMode.empty();

				switch ( debuggerType ) {
					case VSCODE_DEBUGGER_TYPE_CPPDBG_GDB:
						string_builder_appendf( &launchJSONContent, "\t\t\t\"type\": \"cppdbg\",\n" );
						if ( !hasPlatformConfigs ) {
							string_builder_appendf( &launchJSONContent, "\t\t\t\"MIMode\": \"gdb\",\n" );
						}
						break;

					case VSCODE_DEBUGGER_TYPE_CPPDBG_LLDB:
						string_builder_appendf( &launchJSONContent, "\t\t\t\"type\": \"cppdbg\",\n" );
						if ( !hasPlatformConfigs ) {
							string_builder_appendf( &launchJSONContent, "\t\t\t\"MIMode\": \"lldb\",\n" );
						}
						break;

					case VSCODE_DEBUGGER_TYPE_CPPVSDBG:
						string_builder_appendf( &launchJSONContent, "\t\t\t\"type\": \"cppvsdbg\",\n" );
						break;

					case VSCODE_DEBUGGER_TYPE_UNSET:
						break;
				}
			}

			string_builder_appendf( &launchJSONContent, "\t\t\t\"request\": \"launch\",\n" );
			string_builder_appendf( &launchJSONContent, "\t\t\t\"program\": \"%s\",\n", launchConfig->binaryName.c_str() );

			if ( !launchConfig->args.empty() ) {
				string_builder_appendf( &launchJSONContent, "\t\t\t\"args\": [\n" );

				For ( u32, argIndex, 0, launchConfig->args.size() ) {
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\"%s\"", launchConfig->args[argIndex].c_str() );

					if ( argIndex < launchConfig->args.size() - 1 ) {
						string_builder_appendf( &launchJSONContent, ",\n" );
					} else {
						string_builder_appendf( &launchJSONContent, "\n" );
					}
				}

				string_builder_appendf( &launchJSONContent, "\t\t\t],\n" );
			}

			{
				const char *cwd = launchConfig->cwd.empty() ? "${workspaceFolder}" : launchConfig->cwd.c_str();
				string_builder_appendf( &launchJSONContent, "\t\t\t\"cwd\": \"%s\",\n", cwd );	// TODO(DM): 16/04/2026: do we want to expose this to VSCodeJSONOptions?
			}

			if ( !launchConfig->linuxDebugger.miMode.empty() ) {
				string_builder_appendf( &launchJSONContent, "\t\t\t\"linux\": {\n" );
				string_builder_appendf( &launchJSONContent, "\t\t\t\t\"MIMode\": \"%s\",\n", launchConfig->linuxDebugger.miMode.c_str() );
				if ( !launchConfig->linuxDebugger.miDebuggerPath.empty() ) {
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\"miDebuggerPath\": \"%s\",\n", launchConfig->linuxDebugger.miDebuggerPath.c_str() );
				}
				string_builder_appendf( &launchJSONContent, "\t\t\t},\n" );
			}

			if ( !launchConfig->windowsDebugger.miMode.empty() ) {
				string_builder_appendf( &launchJSONContent, "\t\t\t\"windows\": {\n" );
				string_builder_appendf( &launchJSONContent, "\t\t\t\t\"MIMode\": \"%s\",\n", launchConfig->windowsDebugger.miMode.c_str() );
				if ( !launchConfig->windowsDebugger.miDebuggerPath.empty() ) {
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\"miDebuggerPath\": \"%s\",\n", launchConfig->windowsDebugger.miDebuggerPath.c_str() );
				}
				string_builder_appendf( &launchJSONContent, "\t\t\t},\n" );
			}

			if ( !launchConfig->setupCommands.empty() ) {
				string_builder_appendf( &launchJSONContent, "\t\t\t\"setupCommands\": [\n" );

				For ( u32, cmdIndex, 0, launchConfig->setupCommands.size() ) {
					const VSCodeSetupCommand *cmd = &launchConfig->setupCommands[cmdIndex];
					string_builder_appendf( &launchJSONContent, "\t\t\t\t{\n" );
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\t\"description\": \"%s\",\n", cmd->description.c_str() );
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\t\"text\": \"%s\",\n", cmd->text.c_str() );
					string_builder_appendf( &launchJSONContent, "\t\t\t\t\t\"ignoreFailures\": %s,\n", cmd->ignoreFailures ? "true" : "false" );
					string_builder_appendf( &launchJSONContent, "\t\t\t\t},\n" );
				}

				string_builder_appendf( &launchJSONContent, "\t\t\t],\n" );
			}

			string_builder_appendf( &launchJSONContent, "\t\t},\n" );
		}

		string_builder_appendf( &launchJSONContent, "\t]\n" );
		string_builder_appendf( &launchJSONContent, "}\n" );

		if ( !WriteStringBuilderToFile( &launchJSONContent, launchJSONFilename ) ) {
			error( "Failed to write \"%s\".\n", launchJSONFilename );
			return false;
		}
	}

	return true;
}