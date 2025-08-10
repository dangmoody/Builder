#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder	= "bin",
		.binary_name	= "marco_polo",

		.source_files = {
			// you can add individual files
			"src/main.cpp",

			// or entire folders at a time
			// you can also filter by wildcard
			"src/*.win64.cpp"
		},
	};

	add_build_config( options, &config );
}