#define BUILDER_IMPLEMENTATION
#include "../../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "undefined_behavior",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_sanitizer_undefined_behavior",
		.sourceFiles	= MakeStringList( "main.c" ),
		.sanitizers		= SANITIZER_UNDEFINED_BEHAVIOR,
	};

	return Build( &options, argc, argv );
}
