#include <builder.h>

static void get_build_configs( BuilderOptions* options ) {
	BuildConfig static_lib = {
		.name			= "library",
		.binary_folder	= "bin",
		.binary_name	= "test_static_lib",
		.binary_type	= BINARY_TYPE_STATIC_LIBRARY,
		.source_files	= { "lib/lib.cpp" },
	};

	BuildConfig program = {
		.depends_on				= { static_lib },
		.name					= "program",
		.binary_folder			= "bin",
		.binary_name			= "test_static_library_program",
		.source_files			= { "program/program.cpp" },
		.additional_includes	= { "lib" },
		.additional_lib_paths	= { "bin" },
		// TODO(DM): 07/10/2025: does this mean we want build scripts to ignore file extensions?
#ifdef _WIN32
		.additional_libs		= { "test_static_lib.lib" },
#else
		.additional_libs		= { "test_static_lib.a" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	add_build_config( options, &program );
}