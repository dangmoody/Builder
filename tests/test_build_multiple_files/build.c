#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {0};

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "multiple_files",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_multiple_files",
		.sourceFiles	= MakeStringList( "src/main.c", "src/test1.c", "src/test2.c" ),
		.defines		= MakeStringList( "MYCONFIG_DOES_A_THING" ),
	};

	// layering onto a config that's already been filled in
	if ( HasCommandLineArg( &options, "--release" ) ) {
		AddDefines( config, "NDEBUG" );
	} else {
		AddDefines( config, "_DEBUG" );
	}

	return Build( &options );
}
