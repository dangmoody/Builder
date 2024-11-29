#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
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
		.additional_libs		= { "test_static_lib.lib" },
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	add_build_config( options, &program );
}