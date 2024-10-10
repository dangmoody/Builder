#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "test_static_library_program";

	options->source_files.push_back( "program.cpp" );

	options->additional_includes.push_back( "../lib" );

	options->additional_lib_paths.push_back( "../lib/bin" );
	options->additional_libs.push_back( "test_static_lib.lib" );
}