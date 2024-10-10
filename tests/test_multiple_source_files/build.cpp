#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "marco_polo";

	// you can add individual files
	options->source_files.push_back( "src\\main.cpp" );

	// or entire folders at a time
	// you can also filter by wildcard
	options->source_files.push_back( "src\\*.win64.cpp" );
}