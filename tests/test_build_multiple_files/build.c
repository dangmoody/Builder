#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig config = {
		.binaryName		= "test_multiple_files",
		.sourceFiles	= (const char *[]) {
			"src/main.c",
			"src/test1.c",
			"src/test2.c",
			NULL
		},
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
