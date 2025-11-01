#include <stdio.h>

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->compiler_path = "../../tools/gcc/bin/gcc";
	options->compiler_version = "15.1.0";
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "Hello from GCC!\n" );

	return 0;
}