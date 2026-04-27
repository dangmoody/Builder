#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	(void) args;

	BuildConfig config = {
		.name				= "config",
		.sourceFiles		= { "src/main.cpp" },
		.defines			= { "MY_DEFINE=1" },
		.additionalIncludes	= { "${workspaceFolder}/include" },
		.binaryType			= BINARY_TYPE_EXE,
		.binaryName			= "test_app",
		.languageVersion	= LANGUAGE_VERSION_CPP17,
	};

	if ( HasCommandLineArg( args, "--release" ) ) {
		config.binaryFolder = "bin/release";
	} else {
		config.binaryFolder = "bin/debug";
	}

	AddBuildConfig( options, &config );

	options->generateVSCodeJSONFiles = true;

	options->vsCodeJSONOptions = {
		.cppPropertiesConfigs = {
#if defined( _WIN32 )
			{ config, VSCODE_INTELLISENSE_MODE_WINDOWS_CLANG_X64 },
#else
			{ config, VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64 },
#endif
		},
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
