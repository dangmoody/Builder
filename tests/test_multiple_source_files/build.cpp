#include "../../builder.h"

#include <core/array.inl>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "marco_polo";

	// you can add individual files
	array_add( &options->source_files, "src\\main.cpp" );

	// or entire folders at a time
	// you can also filter by wildcard
	array_add( &options->source_files, "src\\*.win64.cpp" );
}