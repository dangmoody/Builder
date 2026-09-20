#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#include "../test_compiler_override.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };
	ApplyCompilerOverride( &options, argc, argv );

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "multiple_files",
		.binaryName		= "test_build_multiple_files",
		.sourceFiles	= MakeStringList( "src/main.c", "src/test1.c", "src/test2.c" ),
		.defines		= MakeStringList( "MYCONFIG_DOES_A_THING" ),
		.binaryType		= BINARY_TYPE_EXE,
	};

	// layering onto a config that's already been filled in
	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		AddDefines( config, "NDEBUG" );
	} else {
		AddDefines( config, "_DEBUG" );
	}


	return Build( &options, argc, argv );
}
