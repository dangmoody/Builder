#include <builder.h>

// this is just for the sake of the example
// your own codebase probably has its own implementation of this
#ifdef _WIN64
#include <Windows.h>
static void copy_file( const char* from, const char* to ) {
	CopyFileA( from, to, FALSE );
}
#else
#error TODO(DGM): 11/10/2024: fill me in!
#endif

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_folder = "bin";
	options->binary_name = "sdl_test";

	options->source_files.push_back( "sdl_test.cpp" );

	options->additional_includes.push_back( "SDL2\\include" );
	options->additional_lib_paths.push_back( "SDL2\\lib" );

	options->additional_libs.push_back( "SDL2.lib" );
	options->additional_libs.push_back( "SDL2main.lib" );
}

BUILDER_CALLBACK void on_pre_build( BuilderOptions* options ) {
	copy_file( "tests\\test_third_party_libraries\\SDL2\\lib\\SDL2.dll", "tests\\test_third_party_libraries\\bin\\SDL2.dll" );
}