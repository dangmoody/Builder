#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig config = {
		.binaryName		= "test_build_single_file",
		.sourceFiles = (const char *[]) {
			"main.c",
			NULL
		},
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
