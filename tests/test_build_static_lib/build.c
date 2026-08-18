#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig *libConfig = CreateBuildConfig( &options, "lib", BINARY_TYPE_STATIC_LIBRARY );
	SetBinaryName( libConfig, "test_static_lib" );
	AddSourceFiles( libConfig, "lib/mathlib.c" );

	BuildConfig *programConfig = CreateBuildConfig( &options, "program", BINARY_TYPE_EXE );
	SetBinaryName( programConfig, "test_static_lib_program" );
	AddDependencies( programConfig, libConfig );
	AddSourceFiles( programConfig, "program/main.c" );
	AddIncludes( programConfig, "lib" );
#if defined( _WIN32 )
	AddLinkerArguments( programConfig, "test_static_lib.lib" );
#else
	AddLinkerArguments( programConfig, "./test_static_lib.a" );
#endif

	options.defaultConfig = programConfig;

	return Build( &options );
}
