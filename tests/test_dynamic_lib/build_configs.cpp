#include <builder.h>

static void GetBuildConfigs( BuilderOptions *options ) {
	BuildConfig library = {
		.name			= "library",
		.binaryFolder	= "bin",
#if defined( _WIN32 )
		.binaryName		= "test_dynamic_lib",
#elif defined( __linux__ )
		.binaryName		= "libtest_dynamic_lib",
#endif
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles	= { "lib/*.cpp" },
		.defines		= { "DYNAMIC_LIBRARY_EXPORTS" },
#ifdef __linux__
		.ignoreWarnings	= { "-fPIC" },
#endif
	};

	BuildConfig program = {
		.dependsOn			= { library },
		.name				= "program",
		.binaryFolder		= "bin",
		.binaryName			= "test_dynamic_library_program",
		.sourceFiles		= { "program/*.cpp" },
		.additionalIncludes	= { "lib" },
		.additionalLibPaths	= { "bin" },
		// TODO(DM): 07/10/2025: does this mean we want build scripts to ignore file extensions?
#if defined( _WIN32 )
		.additionalLibs		= { "test_dynamic_lib.lib" },
#elif defined( __linux__ )
		.additionalLibs		= { "libtest_dynamic_lib" },
		.ignoreWarnings		= { "-fPIC" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}