#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = {};

	const char *sourceFiles[] = { "main.c", NULL };

	BuildConfig config = {
		.binaryName		= "test_single_file",
		.sourceFiles	= sourceFiles,
	};

	AddBuildConfig( &options, &config );

	return Build( &options );
}
