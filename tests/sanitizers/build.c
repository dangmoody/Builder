#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#include "../test_compiler_override.h"
#include "sanitizer_test_args.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };
	ApplyCompilerOverride( &options, argc, argv );

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	SanitizerFlags sanitizers = 0;
	if ( HasCommandLineArg( argc, argv, "--" SANITIZER_TEST_ARG_UNDEFINED_BEHAVIOR ) ) {
		sanitizers |= SANITIZER_UNDEFINED_BEHAVIOR;
	}

	if ( HasCommandLineArg( argc, argv, "--" SANITIZER_TEST_ARG_MEMORY ) ) {
		sanitizers |= SANITIZER_MEMORY;
	}

	if ( HasCommandLineArg( argc, argv, "--" SANITIZER_TEST_ARG_ADDRESS ) ) {
		sanitizers |= SANITIZER_ADDRESS;
	}

	if ( HasCommandLineArg( argc, argv, "--" SANITIZER_TEST_ARG_LEAK ) ) {
		sanitizers |= SANITIZER_LEAK;
	}

	if ( HasCommandLineArg( argc, argv, "--" SANITIZER_TEST_ARG_THREAD ) ) {
		sanitizers |= SANITIZER_THREAD;
	}

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "sanitizers",
		.binaryName		= "test_sanitizers",
		.sourceFiles	= MakeStringList( "main.c" ),
		.binaryType		= BINARY_TYPE_EXE,
		.sanitizers		= sanitizers,
	};

	return Build( &options, argc, argv );
}
