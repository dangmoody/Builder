#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	BuildConfig config = {
		.binaryFolder	= "bin",
		.binaryName		= "marco_polo",

		.sourceFiles = {
			// you can add individual files
			"src/main.cpp",

			// or entire folders at a time
			// you can also filter by wildcard
#if defined( _WIN32 )
			"src/*.win64.cpp",
#elif defined( __linux__ )
			"src/*.linux.cpp",
#endif
		},
	};

	AddBuildConfig( options, &config );
}
