#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder	= "bin",
		.binary_name	= "test_dynamic_lib",
		.binary_type	= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files	= { "lib.cpp" },
		.defines		= { "DYNAMIC_LIBRARY_EXPORTS" },
	};

	options->configs.push_back( config );
}