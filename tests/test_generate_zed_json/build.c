#define BUILDER_IMPLEMENTATION
#define BUILDER_ZED_IMPLEMENTATION
#include "../../builder.h"
#include "../../builder_zed.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	config->name = "config";
	config->binaryType = BINARY_TYPE_EXE;
	config->binaryName = "test_generate_zed_json";
	AddSourceFiles( config, "main.c" );

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		config->binaryFolder = "bin/release";
	} else {
		config->binaryFolder = "bin/debug";
	}

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--zed" ) ) {
		ZedTaskConfig taskConfigs[] = {
			{ .config = config },
			{ .config = config, .args = MakeStringList( "--release" ) },
		};

		ZedDebugConfig debugConfigs[] = {
			{
				.label		= "Debug test_generate_zed_json (debug)",
				.binaryName	= "bin/debug/test_generate_zed_json",
				.adapter	= ZED_DEBUGGER_ADAPTER_CODELLDB,
				.request	= ZED_DEBUGGER_REQUEST_LAUNCH,
			},
			{
				.label		= "Debug test_generate_zed_json (release)",
				.binaryName	= "bin/release/test_generate_zed_json",
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
