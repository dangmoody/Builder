#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig library_debug = {
		.name				= "library-debug",
		.source_files		= { "src/library/*.cpp" },
		.binary_name		= "the-library",
		.binary_folder		= "bin/debug/library",
		.binary_type		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.optimization_level	= OPTIMIZATION_LEVEL_O0,
		.defines			= { "LIBRARY_EXPORTS", "_DEBUG" },
	};

	BuildConfig library_release = {
		.name				= "library-release",
		.source_files		= { "src/library/*.cpp" },
		.binary_name		= "the-library",
		.binary_folder		= "bin/release/library",
		.binary_type		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.optimization_level	= OPTIMIZATION_LEVEL_O3,
		.defines			= { "LIBRARY_EXPORTS", "NDEBUG" },
	};

	BuildConfig app_debug = {
		.depends_on				= { library_debug },
		.name					= "app-debug",
		.source_files			= { "src/app/main.cpp" },
		.binary_name			= "the-app",
		.binary_folder			= "bin/debug/app",
		.optimization_level		= OPTIMIZATION_LEVEL_O0,
		.defines				= { "_DEBUG" },
		.additional_includes	= { "src/library" },
		.additional_lib_paths	= { "bin/debug/library" },
		.additional_libs		= { "the-library.lib" },
	};

	BuildConfig app_release = {
		.depends_on				= { library_release },
		.name					= "app-release",
		.source_files			= { "src/app/main.cpp" },
		.binary_name			= "the-app",
		.binary_folder			= "bin/release/app",
		.optimization_level		= OPTIMIZATION_LEVEL_O3,
		.defines				= { "NDEBUG" },
		.additional_includes	= { "src/library" },
		.additional_lib_paths	= { "bin/release/library" },
		.additional_libs		= { "the-library.lib" },
	};

	// if you know that you only want to build with visual studio then you dont need to add the build configs like this
	// it will happen for you automatically
	add_build_config( options, &app_debug );
	add_build_config( options, &app_release );

	// if you know you dont wanna build with visual studio then either set the bool to false or just dont write this part
	options->generate_solution = true;
	options->solution.name = "test-sln";
	options->solution.path = "visual_studio";
	options->solution.platforms = { "x64" };
	options->solution.projects = {
		{
			.name = "the-library",
			.code_folders = { "src/library" },
			.file_extensions = { "cpp", "h" },
			.configs = {
				{ "debug",   library_debug,   { /* debugger arguments */ } },
				{ "release", library_release, { /* debugger arguments */ } },
			}
		},

		{
			.name = "app",
			.code_folders = { "src/app" },
			.file_extensions = { "cpp", "h" },
			.configs = {
				{ "debug",   app_debug,   { /* debugger arguments */ } },
				{ "release", app_release, { /* debugger arguments */ } },
			}
		}
	};
}