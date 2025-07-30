#include <builder.h>

#include <stdio.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->compiler_path = "C:\\Program Files\\LLVM\\bin\\clang.exe";
	options->compiler_version = "18.1.8";

	//options->compiler_path = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\cl.exe";
	//options->compiler_path = "cl";
	//options->compiler_version = "14.44.34468";	// deliberately wrong to test compiler version mismatch warning
}

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "This called the compiler, yes?\n" );

	return 0;
}