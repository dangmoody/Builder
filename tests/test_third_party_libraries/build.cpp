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

// your own codebase probably has its own CopyFile() function
// this one is just for the sake of the demo
// this implementation is also shit so dont actually use it as reference
static void CopyFile( const char *from, const char *to ) {
	FILE* srcFile = fopen( from, "rb" );
	FILE* dstFile = fopen( to, "wb" );

	assert( srcFile );
	assert( dstFile );

	int c = -1;

	while ( ( c = fgetc( srcFile ) ) != EOF ) {
		fputc( c, dstFile );
	}

	fclose( dstFile );
	dstFile = NULL;

	fclose( srcFile );
	srcFile = NULL;
}

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options ) {
	BuildConfig config = {
		.binaryFolder		= "bin",
		.binaryName			= "sdl_test",
		.sourceFiles		= { "sdl_test.cpp" },
		.additionalIncludes	= { "SDL2/include" },
		.additionalLibPaths	= { "SDL2/lib" },
#if defined( _WIN32 )
		.additionalLibs		= { "SDL2.lib", "SDL2main.lib" },
#elif defined( __linux__ )
		.additionalLibs		= { "SDL2" },
#endif
	};

	AddBuildConfig( options, &config );
}

#ifdef _WIN32
BUILDER_CALLBACK void OnPostBuild() {
	std::string src = std::string( "SDL2/lib/libSDL2" ) + DYNAMIC_LIBRARY_FILE_EXTENSION;
	std::string dst = std::string( "bin/SDL2" ) + DYNAMIC_LIBRARY_FILE_EXTENSION;

	CopyFile( src.c_str(), dst.c_str() );
}
#endif