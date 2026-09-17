#define BUILDER_IMPLEMENTATION
#include "../../../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig selfConfig = {
		.name			= "self",
		.sourceFiles	= MakeStringList( "build.c" ),
	};
	options.selfRebuildConfig = &selfConfig;

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "leak",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_sanitizer_leak",
		.sourceFiles	= MakeStringList( "main.c" ),
		.sanitizers		= SANITIZER_LEAK,
	};

	return Build( &options, argc, argv );
}
