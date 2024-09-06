#include "../../../builder.h"

#include <core/array.inl>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "test_dynamic_library_program";

	array_add( &options->source_files, "program.cpp" );

	array_add( &options->additional_includes, "../lib" );

	array_add( &options->additional_lib_paths, "../lib/bin" );
	array_add( &options->additional_libs, "test_dynamic_lib.lib" );
}