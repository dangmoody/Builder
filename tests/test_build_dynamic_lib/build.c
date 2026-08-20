#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig *libConfig = CreateBuildConfig( &options );
	*libConfig = (BuildConfig) {
		.name			= "lib",
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.binaryName		= "test_dynamic_lib",
		.sourceFiles	= MakeStringList( "lib/mathlib.c" ),
		.defines		= MakeStringList( "MATHLIB_BUILDING" ),
	};

	BuildConfig *programConfig = CreateBuildConfig( &options );
	*programConfig = (BuildConfig) {
		.name				= "program",
		.binaryType			= BINARY_TYPE_EXE,
		.binaryName			= "test_dynamic_lib_program",
		.dependsOn			= MakeDependencies( libConfig ),
		.sourceFiles		= MakeStringList( "program/main.c" ),
		.additionalIncludes	= MakeStringList( "lib" ),
#if defined( _WIN32 )
		.additionalLibs		= MakeStringList( "test_dynamic_lib.lib" ),
#else
		.additionalLibs				= MakeStringList( "./test_dynamic_lib.so" ),
		.additionalLinkerArguments	= MakeStringList( "-Wl,-rpath,$ORIGIN" ),
#endif
	};

	options.defaultConfig = programConfig;

	return Build( &options );
}
