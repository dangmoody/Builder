#define BUILDER_IMPLEMENTATION
#define BUILDER_VS_CODE_IMPLEMENTATION
#include "../../builder.h"
#include "../../builder_vs_code.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig config = {
		.name			= "config",
		.binaryName		= "test_generate_vs_code_json",
		.sourceFiles	= (const char *[]) {
			"main.c",
			NULL
		},
	};

	if ( HasCommandLineArg( &options, "--release" ) ) {
		config.binaryFolder = "bin/release";
	} else {
		config.binaryFolder = "bin/debug";
	}

	options.defaultConfig = &config;

	AddBuildConfig( &options, &config );

	if ( HasCommandLineArg( &options, "--vscode" ) ) {
		VSCodeCppPropertiesConfig cppPropertiesConfigs[] = {
			{ .config = &config, .intelliSenseMode = VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64 },
		};

		VSCodeTaskConfig taskConfigs[] = {
			{ .config = &config },
			{ .config = &config, .additionalBuildArgs = (const char *[]) { "--release", NULL } },
		};

		VSCodeLaunchConfig launchConfigs[] = {
			{
				.binaryName	= "bin/debug/test_generate_vs_code_json",
				.debuggerType = VSCODE_DEBUGGER_TYPE_CPPDBG_GDB,
			},
			{
				.binaryName	= "bin/release/test_generate_vs_code_json",
				.debuggerType = VSCODE_DEBUGGER_TYPE_CPPDBG_GDB,
			},
		};

		VSCodeJSONOptions vsCodeOptions = {
			.cppPropertiesConfigs		= cppPropertiesConfigs,
			.cppPropertiesConfigsCount	= BUILDER_COUNT_OF( cppPropertiesConfigs ),
			.taskConfigs				= taskConfigs,
			.taskConfigsCount			= BUILDER_COUNT_OF( taskConfigs ),
			.launchConfigs				= launchConfigs,
			.launchConfigsCount			= BUILDER_COUNT_OF( launchConfigs ),
		};

		return Builder_GenerateVSCodeJSONFiles( &options, &vsCodeOptions ) ? 0 : 1;
	}

	return Build( &options );
}
