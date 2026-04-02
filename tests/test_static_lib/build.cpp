#include <builder.h>

#include "../test_compiler_override.h"

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions* options, CommandLineArgs* args ) {
	ApplyCompilerOverride( options, args );

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
#ifdef _WIN32
		.additionalLibs		= { "test_static_lib" },
#else
		.additionalLibs		= { ":test_static_lib" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	AddBuildConfig( options, &program );
}
