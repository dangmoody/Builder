#include <stdio.h>

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

#define CLANG	0
#define MSVC	1
#define GCC		2
#define USE_COMPILER CLANG

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
#if USE_COMPILER == CLANG
	options->compiler_path = "C:\\Program Files\\LLVM\\bin\\clang.exe";
	options->compiler_version = "18.1.8";
#elif USE_COMPILER == MSVC
	// MSVC compiler path can be absolute
	//options->compiler_path = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\cl.exe";
	// but we recommend just doing this (in combination with compiler_version below) and letting builder figure out what version of MSVC you want
	options->compiler_path = "cl";
	options->compiler_version = "14.44.35207";
#elif USE_COMPILER == GCC
	options->compiler_path = "C:\\Program Files\\mingw64\\bin\\gcc.exe";
	options->compiler_version = "15.1.0";
#endif
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "This called the compiler, yes?\n" );

	return 0;
}