#include <builder.h>

#include "../test_compiler_override.h"

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	ApplyCompilerOverride( options, args );

	BuildConfig library = {
		.name			= "library",
		.binaryFolder	= "bin",
		.binaryName		= "test_dynamic_lib",
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
#ifdef _WIN32
		.additionalLibs		= { "test_dynamic_lib" },
#else
		.additionalLibs		= { ":test_dynamic_lib.so" },
#endif
#ifdef __linux__
		.ignoreWarnings		= { "-fPIC" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}
