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
#ifdef __linux__
		.ignoreWarnings		= { "-fPIC" },
#endif
	};

	if ( options->compilerPath == "cl" ) {
		program.additionalLibs = { "test_dynamic_lib.lib" };
	} else {
		program.additionalLibs = { "test_dynamic_lib" };
	}

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}