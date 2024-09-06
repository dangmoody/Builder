#include "lib.h"

#include "../../../builder.h"

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "test_static_lib";

	options->binary_type = BINARY_TYPE_STATIC_LIBRARY;
}

void SayHello() {
	printf( "Hello from the static library!\n" );
}