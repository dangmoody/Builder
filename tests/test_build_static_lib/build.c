#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = { 0 };

	BuildConfig *libConfig = CreateBuildConfig( &options );
	*libConfig = (BuildConfig) {
		.name			= "lib",
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
		.binaryName		= "test_static_lib",
		.sourceFiles	= MakeStringList( "lib/mathlib.c" ),
	};

	BuildConfig *programConfig = CreateBuildConfig( &options );
	*programConfig = (BuildConfig) {
		.name				= "program",
		.binaryType			= BINARY_TYPE_EXE,
		.binaryName			= "test_static_lib_program",
		.dependsOn			= MakeDependencies( libConfig ),
		.sourceFiles		= MakeStringList( "program/main.c" ),
		.additionalIncludes	= MakeStringList( "lib" ),
#if defined( _WIN32 )
		.additionalLinkerArguments = MakeStringList( "test_static_lib.lib" ),
#else
		.additionalLinkerArguments = MakeStringList( "./test_static_lib.a" ),
#endif
	};

	options.defaultConfig = programConfig;

	return Build( &options, argc, argv );
}
