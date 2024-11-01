#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder			= "bin",
		.binary_name			= "test_static_library_program",
		.source_files			= { "program.cpp" },
		.additional_includes	= { "../lib" },
		.additional_lib_paths	= { "../lib/bin" },
		.additional_libs		= { "test_static_lib.lib" },
	};

	options->configs.push_back( config );
}