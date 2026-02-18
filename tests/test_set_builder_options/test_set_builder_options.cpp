// you can use this #define if you need to seperate your build script from your actual program code
#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options ) {
	BuildConfig debug = {
		.name				= "debug",
		.binaryName			= "kenneth",
		.binaryFolder		= "bin/debug",
		.removeSymbols		= false,
		.optimizationLevel	= OPTIMIZATION_LEVEL_O0,
	};

	BuildConfig release = {
		.name				= "release",
		.binaryName			= "kenneth",
		.binaryFolder		= "bin/release",
		.removeSymbols		= true,
		.optimizationLevel	= OPTIMIZATION_LEVEL_O3,
	};

	AddBuildConfig( options, &debug );
	AddBuildConfig( options, &release );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD

#include <stdio.h>

int main() {
	int exitCode = 69;

	printf( "This should return %d...\n", exitCode );

	return exitCode;
}