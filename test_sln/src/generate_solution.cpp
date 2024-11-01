#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig debug = {
		.name = "debug",
		.source_files = { "src/main.cpp" },
		.binary_name = "test",
		.binary_folder = "../bin/debug",
		.optimization_level = OPTIMIZATION_LEVEL_O0,
		.defines = { "_DEBUG" },
	};

	BuildConfig release = {
		.name = "release",
		.source_files = { "src/main.cpp" },
		.binary_name = "test",
		.binary_folder = "../bin/release",
		.optimization_level = OPTIMIZATION_LEVEL_O3,
		.defines = { "NDEBUG" },
	};

	// if you know youre only building with visual studio then you could optionally comment out these two lines
	options->configs.push_back( debug );
	options->configs.push_back( release );

	// if you know you dont wanna build with visual studio then either set the bool to false or just dont write this part
	options->generate_solution = true;
	options->solution.name = "test-sln";
	options->solution.path = "..";
	options->solution.platforms = { "win64" };
	options->solution.projects = {
		{
			.name = "test-project",
			.source_files = { "src/*.cpp" },
			.configs = {
				{ debug,   { /* debugger arguments */ }, "bin/debug"   },
				{ release, { /* debugger arguments */ }, "bin/release" },
			}
		}
	};
}