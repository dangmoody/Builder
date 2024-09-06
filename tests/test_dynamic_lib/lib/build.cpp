#include "../../../builder.h"

#include <core/array.inl>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "test_dynamic_lib";

	options->binary_type = BINARY_TYPE_DYNAMIC_LIBRARY;

	array_add( &options->source_files, "lib.cpp" );

	array_add( &options->defines, "DYNAMIC_LIBRARY_EXPORTS" );
}