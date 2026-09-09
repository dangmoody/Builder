#define BUILDER_IMPLEMENTATION
#define BUILDER_VS_CODE_IMPLEMENTATION
#include "../../builder.h"
#include "../../builder_vs_code.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	config->name = "config";
	config->binaryType = BINARY_TYPE_EXE;
	config->binaryName = "test_generate_vs_code_json";
	AddSourceFiles( config, "main.c" );

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		config->binaryFolder = "bin/release";
	} else {
		config->binaryFolder = "bin/debug";
	}

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--vscode" ) ) {
		VSCodeCppPropertiesConfig cppPropertiesConfigs[] = {
			{ .config = config, .intelliSenseMode = VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64 },
		};

		VSCodeTaskConfig taskConfigs[] = {
			{ .config = config },
			{ .config = config, .additionalBuildArgs = MakeStringList( "--release" ) },
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

		return Builder_GenerateVSCodeJSONFiles( &options, &vsCodeOptions, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
