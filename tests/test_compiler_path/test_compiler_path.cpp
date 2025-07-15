#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->compiler_path = "C:/Program Files/LLVM/bin/clang.exe";
}

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "This called the compiler, yes?\n" );

	return 0;
}