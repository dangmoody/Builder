#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	(void) args;

	BuildConfig config = {
		.name			= "config",
		.sourceFiles	= { "src/main.cpp" },
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_app",
	};

	if ( HasCommandLineArg( args, "--release" ) ) {
		config.binaryFolder = "bin/release";
	} else {
		config.binaryFolder = "bin/debug";
	}

	AddBuildConfig( options, &config );

	options->generateZedJSONFiles = true;

	options->zedJSONOptions = {
		.taskConfigs = {
			{ config                  },
			{ config, { "--release" } },
		},
		.debugConfigs = {
			{ .label = "config (debug)",   .binaryName = "bin/debug/" + config.binaryName   },
			{ .label = "config (release)", .binaryName = "bin/release/" + config.binaryName },
		},
	};
}
