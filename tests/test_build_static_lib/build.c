#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig libConfig = {
		.name			= "lib",
		.binaryName		= "test_static_lib",
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
		.sourceFiles	= (const char *[]) {
			"lib/mathlib.c",
			NULL
		},
	};

	BuildConfig programConfig = {
		.name			= "program",
		.binaryName		= "test_static_lib_program",
		.dependsOn		= (BuildConfig *[]) {
			&libConfig,
			NULL
		},
		.sourceFiles	= (const char *[]) {
			"program/main.c",
			NULL
		},
		.additionalIncludes = (const char *[]) {
			"lib",
			NULL
		},
		.additionalLinkerArguments = (const char *[]) {
#if defined( _WIN32 )
			"test_static_lib.lib",
#else
			"./test_static_lib.a",
#endif
			NULL
		},
	};

	options.defaultConfig = &programConfig;

	AddBuildConfig( &options, &programConfig );

	return Build( &options );
}