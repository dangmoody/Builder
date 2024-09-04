#include "../../builder.h"

#include <core/array.inl>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "sdl_test";

	array_add( &options->source_files, "sdl_test.cpp" );

	array_add( &options->additional_includes, "SDL2\\include" );
	array_add( &options->additional_lib_paths, "SDL2\\lib" );

	array_add( &options->additional_libs, "SDL2.lib" );
	array_add( &options->additional_libs, "SDL2main.lib" );
}