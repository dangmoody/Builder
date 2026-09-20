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

	BuildConfig *libConfig = CreateBuildConfig( &options );
	*libConfig = (BuildConfig) {
		.name			= "lib",
		.binaryName		= "test_dynamic_lib",
		.sourceFiles	= MakeStringList( "lib/mathlib.c" ),
		.defines		= MakeStringList( "MATHLIB_BUILDING" ),
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
	};

	BuildConfig *programConfig = CreateBuildConfig( &options );
	*programConfig = (BuildConfig) {
		.name				= "program",
		.dependsOn			= MakeDependencies( libConfig ),
		.binaryName			= "test_dynamic_lib_program",
		.sourceFiles		= MakeStringList( "program/main.c" ),
		.additionalIncludes	= MakeStringList( "lib" ),
		.additionalLibPaths	= MakeStringList( "." ),
#if defined( _WIN32 )
		.additionalLibs		= MakeStringList( "test_dynamic_lib.lib" ),
#else
		.additionalLibs		= MakeStringList( "test_dynamic_lib.so" ),
#endif
		.binaryType			= BINARY_TYPE_EXE,
	};

	options.defaultConfig = programConfig;

	return Build( &options, argc, argv );
}
