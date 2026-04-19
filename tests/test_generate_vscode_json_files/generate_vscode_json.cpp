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

	options->generateVSCodeJSONFiles = true;

	options->vsCodeJSONOptions = {
		.taskConfigs = {
			{ config                  },
			{ config, { "--release" } },
		},
		.launchConfigs = {
			{ .binaryName = "bin/debug/" + config.binaryName,   .debuggerType = VSCODE_DEBUGGER_TYPE_CPPVSDBG   },
			{ .binaryName = "bin/release/" + config.binaryName, .debuggerType = VSCODE_DEBUGGER_TYPE_CPPDBG_GDB },
		},
	};
}
