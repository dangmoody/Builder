#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig *config = CreateBuildConfig( &options, "single_file", BINARY_TYPE_EXE );
	SetBinaryName( config, "test_build_single_file" );
	AddSourceFiles( config, "main.c" );

	return Build( &options );
}
