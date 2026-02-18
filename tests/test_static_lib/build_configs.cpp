#include <builder.h>

static void GetBuildConfigs( BuilderOptions* options ) {
	BuildConfig static_lib = {
		.name			= "library",
		.binaryFolder	= "bin",
		.binaryName		= "test_static_lib",
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
		.sourceFiles	= { "lib/lib.cpp" },
	};

	BuildConfig program = {
		.dependsOn			= { static_lib },
		.name				= "program",
		.binaryFolder		= "bin",
		.binaryName			= "test_static_library_program",
		.sourceFiles		= { "program/program.cpp" },
		.additionalIncludes	= { "lib" },
		.additionalLibPaths	= { "bin" },
		// TODO(DM): 07/10/2025: does this mean we want build scripts to ignore file extensions?
#ifdef _WIN32
		.additionalLibs		= { "test_static_lib.lib" },
#else
		.additionalLibs		= { "test_static_lib.a" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}