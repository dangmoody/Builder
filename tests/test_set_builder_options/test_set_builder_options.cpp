// this test is for configuring as many things through set_builder_options() as possible

#include <stdio.h>

// you can use this #define if you need to seperate your build script from your actual program code
#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

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

	add_build_config( options, &debug );
	add_build_config( options, &release );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

int main() {
	int exitCode = 420;

	printf( "This should return %d...\n", exitCode );

	return exitCode;
}