#include <builder.h>

// this is just for the sake of the example
// your own codebase probably has its own implementation of this
#ifdef _WIN64
#include <Windows.h>

#define DYNAMIC_LIBRARY_FILE_EXTENSION ".dll"
#else
#include <fcntl.h>

#define DYNAMIC_LIBRARY_FILE_EXTENSION ".so"
#endif

#include <assert.h>

// your own codebase probably has its own copy_file() function
// this one is just for the sake of the demo
// this implementation is also shit so dont actually use it as reference
static void copy_file( const char* from, const char* to ) {
	FILE* file_src = fopen( from, "rb" );
	FILE* file_dst = fopen( to, "wb" );

	assert( file_src );
	assert( file_dst );

	int c = -1;

	while ( ( c = fgetc( file_src ) ) != EOF ) {
		fputc( c, file_dst );
	}

	fclose( file_dst );
	file_dst = NULL;

	fclose( file_src );
	file_src = NULL;
}

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder			= "bin",
		.binary_name			= "sdl_test",
		.source_files			= { "sdl_test.cpp" },
		.additional_includes	= { "SDL2/include" },
		.additional_lib_paths	= { "SDL2/lib" },
#if defined( _WIN32 )
		.additional_libs		= { "SDL2.lib", "SDL2main.lib" },
#elif defined( __linux__ )
		.additional_libs		= { "SDL2" },
#endif
	};

	add_build_config( options, &config );
}

#ifdef _WIN32
BUILDER_CALLBACK void on_post_build() {
	std::string src = std::string( "SDL2/lib/libSDL2" ) + DYNAMIC_LIBRARY_FILE_EXTENSION;
	std::string dst = std::string( "bin/SDL2" ) + DYNAMIC_LIBRARY_FILE_EXTENSION;

	copy_file( src.c_str(), dst.c_str() );
}
#endif