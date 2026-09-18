#define BUILDER_IMPLEMENTATION
#include "../../../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "address",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_sanitizer_address",
		.sourceFiles	= MakeStringList( "main.c" ),
		.sanitizers		= SANITIZER_ADDRESS,
	};

	return Build( &options, argc, argv );
}
