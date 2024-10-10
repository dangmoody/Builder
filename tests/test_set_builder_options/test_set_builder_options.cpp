// this test is for configuring as many things through set_builder_options() as possible

#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_name = "kenneth";
	options->binary_folder = std::string( "bin\\" + options->config );

	if ( options->config == "debug" ) {
		options->remove_symbols = false;
		options->optimization_level = OPTIMIZATION_LEVEL_O0;
	} else if ( options->config == "release" ) {
		options->remove_symbols = true;
		options->optimization_level = OPTIMIZATION_LEVEL_O3;
	}
}

int main() {
	int exitCode = 420;

	printf( "This should return %d...\n", exitCode );

	return exitCode;
}