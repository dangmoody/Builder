#define BUILDER_IMPLEMENTATION
#include "../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig selfConfig = {
		.name			= "self",
		.sourceFiles	= MakeStringList( "build_tests.c" ),
	};
	options.selfRebuildConfig = &selfConfig;

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "this_doesnt_need_to_have_a_name",
		.binaryName		= "builder-tests",
		.sourceFiles	= MakeStringList( "test_main.c" ),
		.defines		= MakeStringList( "_CRT_SECURE_NO_WARNINGS" ),
		.ignoreWarnings	= MakeStringList( "-Wno-switch" ),
	};

	return Build( &options, argc, argv );
}
