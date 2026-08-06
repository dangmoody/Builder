#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	const char *sourceFiles[] = { "mathlib.c", NULL };

	BuildConfig config = {
		.binaryName		= "test_static_lib",
		.sourceFiles	= sourceFiles,
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
