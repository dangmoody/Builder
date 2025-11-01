#include <stdio.h>

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->compiler_path = "../../clang/bin/clang";
	options->compiler_version = "20.1.5";
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "Hello from Clang!\n" );

	return 0;
}