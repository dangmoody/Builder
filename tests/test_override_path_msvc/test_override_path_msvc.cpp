#include <stdio.h>

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	// If using MSVC, the compiler path can be absolute:
	//options->compiler_path = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\cl.exe";

	// But we recommend just doing this (in combination with compiler_version below), and then let Builder figure out where that lives for you:
	options->compiler_path = "cl";
	options->compiler_version = "14.44.35207";
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "Hello from MSVC!\n" );

	return 0;
}