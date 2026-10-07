#define BUILDER_IMPLEMENTATION
#include "../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build_tests.c" ),
	};

	BuildConfig *test = CreateBuildConfig( &options );
	*test = (BuildConfig) {
		.name			= "tests",
		.binaryName		= "builder_tests",
		.sourceFiles	= MakeStringList( "builder_tests.c" ),
		.defines		= MakeStringList( "_CRT_SECURE_NO_WARNINGS" ),
		.ignoreWarnings	= MakeStringList( "-Wno-switch" ),
	};

	options.defaultConfig = test;

	return Build( &options, argc, argv );
}
