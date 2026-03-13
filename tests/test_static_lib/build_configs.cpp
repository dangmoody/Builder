#include <builder.h>

static void GetBuildConfigs( BuilderOptions *options ) {
	BuildConfig staticLib = {
		.name			= "library",
		.binaryFolder	= "bin",
		.binaryName		= "test_static_lib",
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
		.sourceFiles	= { "lib/lib.cpp" },
	};

	BuildConfig program = {
		.dependsOn			= { staticLib },
		.name				= "program",
		.binaryFolder		= "bin",
		.binaryName			= "test_static_library_program",
		.sourceFiles		= { "program/program.cpp" },
		.additionalIncludes	= { "lib" },
		.additionalLibPaths	= { "bin" },
	};

	// TODO(DM): 07/10/2025: does this mean we want build scripts to ignore file extensions?
	if ( options->compilerPath == "cl" ) {
		program.additionalLibs = { "test_static_lib.lib" };
	} else {
#ifdef _WIN32
		program.additionalLibs = { "test_static_lib" };
#else
		program.additionalLibs = { ":test_static_lib.a" };
#endif
	}

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}