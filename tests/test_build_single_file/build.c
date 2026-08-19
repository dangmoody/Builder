#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "single_file",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_build_single_file",
		.sourceFiles	= MakeStringList( "main.c" ),
	};

	return Build( &options );
}
