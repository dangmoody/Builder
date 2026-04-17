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

	const char *appPath = path_remove_file_extension( path_app_path() );

	// tasks.json
	{
		const char *tasksJSONFilename = tprintf( "%s%ctasks.json", dotVSCodeFolder, PATH_SEPARATOR );

		StringBuilder tasksJSONContent = {};
		defer( string_builder_destroy( &tasksJSONContent ) );

		string_builder_reset( &tasksJSONContent );

		string_builder_appendf( &tasksJSONContent, "{\n" );
		string_builder_appendf( &tasksJSONContent, "\t// See https://go.microsoft.com/fwlink/?LinkId=733558\n" );
		string_builder_appendf( &tasksJSONContent, "// for the documentation about the tasks.json format\n" );
		string_builder_appendf( &tasksJSONContent, "\version\": \"2.0.0\",\n" );
		string_builder_appendf( &tasksJSONContent, "\t\"tasks\": [\n" );
		For ( u64, configIndex, 0, options->configs.size() ) {
			BuildConfig *config = &options->configs[configIndex];

			string_builder_appendf( &tasksJSONContent,          "\t\t{\n" );
			string_builder_appendf( &tasksJSONContent, tprintf( "\t\t\t\"label\": \"Build %s\"\n", config->name.c_str() ) );
			string_builder_appendf( &tasksJSONContent,          "\t\t\t\"type\": \"shell\"\n" );
			string_builder_appendf( &tasksJSONContent, tprintf( "\t\t\t\"command\": \"%s%c%s\"\n", appPath, PATH_SEPARATOR, BUILDER_PROGRAM_NAME ) );
			string_builder_appendf( &tasksJSONContent,          "\t\t\t\"args\": [ " );
			{
				string_builder_appendf( &tasksJSONContent, "%s", context->inputFile );
				string_builder_appendf( &tasksJSONContent, "%s%s", ARG_CONFIG, config->name.c_str() );
				// TODO: DM: 15/04/2026: add BuiderOptions::VSCodeJSONOptions
				/*if ( options.vsCodeJSONOptions ) {
					For ( u32, argIndex, 0, numArgs ) {
						string_builder_appendf( &tasksJSONContent, "\"%s\"" );

						if ( argIndex < numArgs - 1 ) {
							string_builder_appendf( &tasksJSONContent, ", " );
						}
					}
				}*/
			}
			string_builder_appendf( &tasksJSONContent, " ]\n" );
			string_builder_appendf( &tasksJSONContent, "\t\t}\n" );
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
		string_builder_appendf( &launchJSONContent, "// Use IntelliSense to learn about possible attributes.\n" );
		string_builder_appendf( &launchJSONContent, "// Hover to view descriptions of existing attributes.\n" );
		string_builder_appendf( &launchJSONContent, "// For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\n" );
		string_builder_appendf( &launchJSONContent, "\"version\": \"0.2.0\",\n" );
		string_builder_appendf( &launchJSONContent, "\t\"configurations\": [\n" );
		For ( u32, configIndex, 0, options->vsCodeJSONOptions.launchConfigs.size() ) {
			VSCodeLaunchConfig *launchConfig = &options->vsCodeJSONOptions.launchConfigs[configIndex];

			if ( launchConfig->config.binaryType != BINARY_TYPE_EXE ) {
				warning(
					"You have asked me to generate a VS Code launch config (in launch.json) for your BuildConfig \"%s\".\n"
					"That config produces a %s file, and I can only generate launch configs for EXEs.  Skipping this one...\n"
					, launchConfig->config.name.c_str()
					, GetFileExtensionFromBinaryType( launchConfig->config.binaryType )
				);
				continue;
			}

			const char *fullBinaryName = BuildConfig_GetFullBinaryName( &launchConfig->config );

			string_builder_appendf( &launchJSONContent,          "\t\t{\n" );
			string_builder_appendf( &launchJSONContent, tprintf( "\t\t\t\"name\": \"%s\"\n", launchConfig->config.name.c_str() ) );
#ifdef _WIN32
			string_builder_appendf( &launchJSONContent, "\t\t\t\"type\": \"cppvsdbg\"\n" );
#else
			string_builder_appendf( &launchJSONContent, "\t\t\t\"type\": \"gdb\"\n" );
#endif
			string_builder_appendf( &launchJSONContent, tprintf( "\t\t\t\"program\": \"%s\"\n", fullBinaryName ) );
			{
				string_builder_appendf( &launchJSONContent, tprintf( "\t\t\t\"args\": [\n" ) );

				For ( u32, argIndex, 0, launchConfig->args.size() ) {
					string_builder_appendf( &launchJSONContent, tprintf( "\"%s\"", launchConfig->args[argIndex].c_str() ) );

					if ( argIndex < launchConfig->args.size() - 1 ) {
						string_builder_appendf( &launchJSONContent, ",\n" );
					} else {
						string_builder_appendf( &launchJSONContent, "\n" );
					}
				}

				string_builder_appendf( &launchJSONContent, tprintf( "]\n" ) );
			}
			string_builder_appendf( &launchJSONContent, tprintf( "\t\t\t\"cwd\": \"%s\"\n", launchConfig->cwd.c_str() ) );	// TODO: DM: 16/04/2026: do we want to expose this to the user?
			string_builder_appendf( &launchJSONContent,          "\t\t}\n" );
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