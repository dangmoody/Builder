#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {};

	BuildConfig libConfig = {
		.name			= "lib",
		.binaryName		= "test_dynamic_lib",
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles	= (const char *[]) {
			"lib/mathlib.c",
			NULL
		},
		.defines		= (const char *[]) {
			"MATHLIB_BUILDING",
			NULL
		},
	};

	BuildConfig programConfig = {
		.name			= "program",
		.binaryName		= "test_dynamic_lib_program",
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
			"test_dynamic_lib.lib",
#else
			"./test_dynamic_lib.so",
			"-Wl,-rpath,$ORIGIN",
#endif
			NULL
		},
	};

	AddBuildConfig( &options, &programConfig );

	return Build( &options );
}
