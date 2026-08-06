#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define STRING_LIST( ... ) (const char *[]) { __VA_ARGS__, NULL }

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig config = {
		.binaryName		= "test_build_single_file",
		.sourceFiles = STRING_LIST(
			"main.c"
		),
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
