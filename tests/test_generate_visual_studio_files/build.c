#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define BUILDER_VISUAL_STUDIO_IMPLEMENTATION
#include "../../builder_visual_studio.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = { 0 };

	BuildConfig *mathlib = CreateBuildConfig( &options );
	*mathlib = (BuildConfig) {
		.name				= "mathlib",
		.binaryName			= "mathlib",
		// .binaryFolder		= "bin/debug",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles		= MakeStringList( "src/mathlib/**/*.c" ),
		.additionalIncludes	= MakeStringList( "src" ),
	};

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		mathlib->binaryFolder = "bin/release";
		mathlib->optimization = OPTIMIZATION_PROGRAM_SPEED;
		AddDefines( mathlib, "MATHLIB_BUILDING", "NDEBUG" );
	} else {
		mathlib->binaryFolder = "bin/debug";
		AddDefines( mathlib, "MATHLIB_BUILDING", "_DEBUG" );
	}

	BuildConfig *app = CreateBuildConfig( &options );
	*app = (BuildConfig) {
		.name				= "app",
		.binaryName			= "app",
		// .binaryFolder		= "bin/debug",
		.binaryType			= BINARY_TYPE_EXE,
		.dependsOn			= MakeDependencies( mathlib ),
		.sourceFiles		= MakeStringList( "src/app/**/*.c" ),
		.additionalIncludes	= MakeStringList( "src" ),
		.additionalLibPaths	= MakeStringList( "bin/debug" ),
#if defined( _WIN32 )
		.additionalLibs		= MakeStringList( "mathlib.lib" ),
#else
		.additionalLibs		= MakeStringList( "./bin/debug/mathlib.so" ),
		.additionalLinkerArguments = MakeStringList( "-Wl,-rpath,$ORIGIN" ),
#endif
	};

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		app->binaryFolder = "bin/release";
		app->optimization = OPTIMIZATION_PROGRAM_SPEED;
		AddDefines( app, "NDEBUG" );
	} else {
		app->binaryFolder = "bin/debug";
		AddDefines( app, "_DEBUG" );
	}

	// only the top-level config needs naming as the default - mathlib is pulled in automatically via dependsOn
	options.defaultConfig = app;

	if ( HasCommandLineArg( argc, argv, "--sln" ) ) {
		VisualStudioConfig mathlibVsConfigs[] = {
			{ .name = "Debug",   .config = mathlib },
			{ .name = "Release", .config = mathlib, .additionalBuildArgs = (const char *[]) { "--release", NULL }, .nmakeOutput = "bin/release/mathlib.dll" },
		};

		VisualStudioConfig appVsConfigs[] = {
			{ .name = "Debug",   .config = app },
			{ .name = "Release", .config = app, .additionalBuildArgs = (const char *[]) { "--release", NULL }, .nmakeOutput = "bin/release/app.exe" },
		};

		VisualStudioProject vsProjects[] = {
			{
				.name			= "mathlib",
				.configs		= mathlibVsConfigs,
				.configsCount	= BUILDER_COUNT_OF( mathlibVsConfigs ),
				.extraFiles		= MakeStringList( "src/mathlib/**/*.h", "src/common.h" ),
			},
			{
				.name			= "app",
				.configs		= appVsConfigs,
				.configsCount	= BUILDER_COUNT_OF( appVsConfigs ),
				.extraFiles		= MakeStringList( "src/app/**/*.h", "src/common.h" ),
			},
		};

		VisualStudioSolution solution = {
			.name			= "test_generate_visual_studio_files",
			.path			= "visual_studio",
			.platforms		= (const char *[]) { "x64", NULL },
			.projects		= vsProjects,
			.projectsCount	= BUILDER_COUNT_OF( vsProjects ),
		};

		return Builder_GenerateVisualStudioSolution( &options, &solution, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
