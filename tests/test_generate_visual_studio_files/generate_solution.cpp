#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options ) {
	BuildConfig libraryDebug = {
		.name				= "library-debug",
		.sourceFiles		= { "src/library1/*.cpp", "src/library2/*.cpp" },
#if defined( _WIN32 )
		.binaryName			= "the-library",
#else
		.binaryName			= "libthe-library",
#endif
		.binaryFolder		= "bin/debug",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.optimizationLevel	= OPTIMIZATION_LEVEL_O0,
		.defines			= { "LIBRARY_EXPORTS", "_DEBUG" },
	};

	BuildConfig libraryRelease = {
		.name				= "library-release",
		.sourceFiles		= { "src/library1/*.cpp", "src/library2/*.cpp" },
#if defined( _WIN32 )
		.binaryName			= "the-library",
#else
		.binaryName			= "libthe-library",
#endif
		.binaryFolder		= "bin/release",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.optimizationLevel	= OPTIMIZATION_LEVEL_O3,
		.defines			= { "LIBRARY_EXPORTS", "NDEBUG" },
	};

	BuildConfig appDebug = {
		.dependsOn			= { libraryDebug },
		.name				= "app-debug",
		.sourceFiles		= { "src/app/main.cpp" },
		.binaryName			= "the-app",
		.binaryFolder		= "bin/debug",
		.optimizationLevel	= OPTIMIZATION_LEVEL_O0,
		.defines			= { "_DEBUG" },
		.additionalIncludes	= { "src" },
		.additionalLibPaths	= { "bin/debug" },
#if defined( _WIN32 )
		.additionalLibs		= { "the-library.lib" },
#else
		.additionalLibs		= { "libthe-library" },
#endif
	};

	BuildConfig appRelease = {
		.dependsOn			= { libraryRelease },
		.name				= "app-release",
		.sourceFiles		= { "src/app/main.cpp" },
		.binaryName			= "the-app",
		.binaryFolder		= "bin/release",
		.optimizationLevel	= OPTIMIZATION_LEVEL_O3,
		.defines			= { "NDEBUG" },
		.additionalIncludes	= { "src" },
		.additionalLibPaths	= { "bin/release" },
#if defined( _WIN32 )
		.additionalLibs		= { "the-library.lib" },
#else
		.additionalLibs		= { "libthe-library" },
#endif
	};

	// if you know that you only want to build with visual studio then you dont need to add the build configs like this
	// it will happen for you automatically
	AddBuildConfig( options, &appDebug );
	AddBuildConfig( options, &appRelease );

	// if you know you dont wanna build with visual studio then either set the bool to false or just dont write this part
	options->generateSolution = true;

	options->solution = {
		.name = "test-sln",
		.path = "visual_studio",
		.platforms = { "x64" },
		.projects = {
			{
				.name = "the-library",
				.codeFolders = { "src/library1", "src/library2" },
				.configs = {
					{ "debug",   libraryDebug,   { /* debugger arguments */ } },
					{ "release", libraryRelease, { /* debugger arguments */ } },
				}
			},

			{
				.name = "app",
				.codeFolders = { "src/app" },
				.configs = {
					{ "debug",   appDebug,   { /* debugger arguments */ } },
					{ "release", appRelease, { /* debugger arguments */ } },
				}
			}
		},
	};
}