#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = {};

	const char *sourceFiles[] = {
		"main.c",
		"test1.c",
		"test2.c",
		NULL
	};

	BuildConfig config = {
		.binaryName		= "test_multiple_files",
		.sourceFiles	= sourceFiles,
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
