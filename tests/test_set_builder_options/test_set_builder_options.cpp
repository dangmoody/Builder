// this test is for configuring as many things through set_builder_options() as possible

#include "../../builder.h"

#include <stdio.h>
#include <string.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_name = "kenneth";
	options->binary_folder = tprintf( "bin\\%s", options->config );

	if ( strcmp( options->config, "debug" ) == 0 ) {
		options->remove_symbols = false;
		options->optimization_level = OPTIMIZATION_LEVEL_O0;
	} else if ( strcmp( options->config, "release" ) == 0 ) {
		options->remove_symbols = true;
		options->optimization_level = OPTIMIZATION_LEVEL_O3;
	}
}

int main() {
	int exitCode = 420;

	printf( "This should return %d...\n", exitCode );

	return exitCode;
}