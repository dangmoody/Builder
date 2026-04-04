#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	BuildConfig library = {
		.name			= "library",
		.sourceFiles	= { "src/library1/*.cpp", "src/library2/*.cpp" },
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.defines		= { "LIBRARY_EXPORTS" },
	};

#if defined( _WIN32 )
	library.binaryName = "the-library";
#else
	library.binaryName = "libthe-library";
#endif

	if ( HasCommandLineArg( args, "--release" ) ) {
		library.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		library.binaryFolder = "bin/release";
		library.defines.push_back( "NDEBUG" );
	} else {
		library.optimizationLevel = OPTIMIZATION_LEVEL_O0;
		library.binaryFolder = "bin/debug";
		library.defines.push_back( "_DEBUG" );
	}

	BuildConfig app = {
		.dependsOn			= { library },
		.name				= "app",
		.sourceFiles		= { "src/app/main.cpp" },
		.binaryName			= "the-app",
		.additionalIncludes	= { "src" },
#if defined( _WIN32 )
		.additionalLibs		= { "the-library" },
#else
		.additionalLibs		= { "libthe-library" },
#endif
	};

	if ( HasCommandLineArg( args, "--release" ) ) {
		app.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		app.binaryFolder = "bin/release";
		app.defines.push_back( "NDEBUG" );
		app.additionalLibPaths.push_back( "bin/release" );
	} else {
		app.optimizationLevel = OPTIMIZATION_LEVEL_O0;
		app.binaryFolder = "bin/debug";
		app.defines.push_back( "_DEBUG" );
		app.additionalLibPaths.push_back( "bin/debug" );
	}

	// if you know that you only want to build with visual studio then you dont need to add the build configs like this
	// it will happen for you automatically
	AddBuildConfig( options, &app );

	// if you know you dont wanna build with visual studio then either set the bool to false or just dont write this part
	options->generateSolution = true;

	options->solution = {
		.name = "test-sln",
		.path = "visual_studio",
		.platforms = { "x64" },
		.projects = {
			{
				.name = "the-library",
				.configs = {
					{ "debug",   library, {             }, { /* debugger arguments */ } },
					{ "release", library, { "--release" }, { /* debugger arguments */ } },
				}
			},

			{
				.name = "app",
				.configs = {
					{ "debug",   app, {             }, { /* debugger arguments */ } },
					{ "release", app, { "--release" }, { /* debugger arguments */ } },
				}
			}
		},
	};
}