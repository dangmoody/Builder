#include "lib.h"

#include "../../../builder.h"

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder	= "bin",
		.binary_name	= "test_static_lib",
		.binary_type	= BINARY_TYPE_STATIC_LIBRARY,
	};

	options->configs.push_back( config );
}

void SayHello() {
	printf( "Hello from the static library!\n" );
}