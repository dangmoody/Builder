#include <builder.h>

static void get_build_configs( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder	= "bin",
		.binary_name	= "marco_polo",

		.source_files = {
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

	add_build_config( options, &config );
}