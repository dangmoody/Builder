#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define BUILDER_COMPILATION_DATABASE_IMPLEMENTATION
#include "../../builder_compilation_database.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name				= "config",
		.binaryName			= "test_compilation_database",
		.binaryFolder		= "bin",
		.sourceFiles		= MakeStringList( "src/*.c" ),
		.additionalIncludes	= MakeStringList( "include" ),
		.defines			= MakeStringList( "HELPER_VALUE=5" ),
	};

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--compile-commands" ) ) {
		CompilationDatabaseOptions compilationDatabaseOptions = { 0 };

		return Builder_GenerateCompilationDatabase( &options, &compilationDatabaseOptions, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
