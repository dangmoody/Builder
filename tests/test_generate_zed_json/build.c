#define BUILDER_IMPLEMENTATION
#define BUILDER_ZED_IMPLEMENTATION
#include "../../builder.h"
#include "../../builder_zed.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig config = {
		.name			= "config",
		.binaryName		= "test_generate_zed_json",
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

	if ( HasCommandLineArg( &options, "--zed" ) ) {
		ZedTaskConfig taskConfigs[] = {
			{ .config = &config },
			{ .config = &config, .args = (const char *[]) { "--release", NULL } },
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

		return Builder_GenerateZedJSONFiles( &options, &zedOptions ) ? 0 : 1;
	}

	return Build( &options );
}
