#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "test_dynamic_lib";

	options->binary_type = BINARY_TYPE_DYNAMIC_LIBRARY;

	options->source_files.push_back( "lib.cpp" );

	options->defines.push_back( "DYNAMIC_LIBRARY_EXPORTS" );
}