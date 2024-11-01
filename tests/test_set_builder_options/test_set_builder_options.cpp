// this test is for configuring as many things through set_builder_options() as possible

#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig debug = {
		.name = "debug",
		.binary_name = "kenneth",
		.binary_folder = "bin\\debug",
		.remove_symbols = false,
		.optimization_level = OPTIMIZATION_LEVEL_O0,
	};

	BuildConfig release = {
		.name = "release",
		.binary_name = "kenneth",
		.binary_folder = "bin\\release",
		.remove_symbols = true,
		.optimization_level = OPTIMIZATION_LEVEL_O3,
	};

	options->configs.push_back( debug );
	options->configs.push_back( release );
}

int main() {
	int exitCode = 420;

	printf( "This should return %d...\n", exitCode );

	return exitCode;
}