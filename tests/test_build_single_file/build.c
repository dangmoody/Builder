#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define STRING_LIST( ... ) (const char *[]) { __VA_ARGS__, NULL }

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	const char *sourceFiles[] = { "main.c", NULL };

	BuildConfig config = {
		.binaryName		= "test_build_single_file",
		.sourceFiles	= (const char *[]) {
			"main.c"
		},
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
