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
	BuildConfig config = {
		.binary_folder			= "bin",
		.binary_name			= "sdl_test",
		.source_files			= { "sdl_test.cpp" },
		.additional_includes	= { "SDL2\\include" },
		.additional_lib_paths	= { "SDL2\\lib" },
		.additional_libs		= { "SDL2.lib", "SDL2main.lib" },
	};

	add_build_config( options, &config );
}

BUILDER_CALLBACK void on_post_build() {
	copy_file( "SDL2\\lib\\SDL2.dll", "bin\\SDL2.dll" );
}