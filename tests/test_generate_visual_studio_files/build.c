#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define BUILDER_VISUAL_STUDIO_IMPLEMENTATION
#include "../../builder_visual_studio.h"

int main( int argc, char **argv ) {
	Builder_RebuildSelf( argc, argv );

	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	BuildConfig mathlib = {
		.name				= "mathlib",
		.binaryName			= "mathlib",
		// .binaryFolder		= "bin/debug",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles		= (const char *[]) {
			"src/mathlib/**/*.c",
			NULL
		},
		.additionalIncludes	= (const char *[]) {
			"src",
			NULL
		},
	};

	if ( HasCommandLineArg( &options, "--release" ) ) {
		mathlib.binaryFolder = "bin/release";
		mathlib.optimization = OPTIMIZATION_PROGRAM_SPEED;
		mathlib.defines = (const char *[]) { "MATHLIB_BUILDING", "NDEBUG", NULL };
	} else {
		mathlib.binaryFolder = "bin/debug";
		mathlib.defines = (const char *[]) { "MATHLIB_BUILDING", "_DEBUG", NULL };
	}

	BuildConfig app = {
		.name				= "app",
		.binaryName			= "app",
		// .binaryFolder		= "bin/debug",
		.binaryType			= BINARY_TYPE_EXE,
		.dependsOn			= (BuildConfig *[]) {
			&mathlib,
			NULL
		},
		.sourceFiles		= (const char *[]) {
			"src/app/**/*.c",
			NULL
		},
		.additionalIncludes	= (const char *[]) {
			"src",
			NULL
		},
		.additionalLibPaths	= (const char *[]) {
			"bin/debug",
			NULL
		},
		.additionalLibs		= (const char *[]) {
#if defined( _WIN32 )
			"mathlib.lib",
#else
			"./bin/debug/mathlib.so",
#endif
			NULL
		},
		.additionalLinkerArguments = (const char *[]) {
#ifdef __linux__
			"-Wl,-rpath,$ORIGIN",
#endif
			NULL
		},
	};

	if ( HasCommandLineArg( &options, "--release" ) ) {
		app.binaryFolder = "bin/release";
		app.optimization = OPTIMIZATION_PROGRAM_SPEED;
		app.defines = (const char *[]) { "NDEBUG", NULL };
	} else {
		app.binaryFolder = "bin/debug";
		app.defines = (const char *[]) { "_DEBUG", NULL };
	}

	options.defaultConfig = &app;

	// only the top-level config needs registering - mathlib is pulled in automatically via dependsOn
	AddBuildConfig( &options, &app );

	if ( HasCommandLineArg( &options, "--sln" ) ) {
		VisualStudioConfig mathlibVsConfigs[] = {
			{ .name = "Debug",   .config = &mathlib },
			{ .name = "Release", .config = &mathlib, .additionalBuildArgs = (const char *[]) { "--release", NULL }, .nmakeOutput = "bin/release/mathlib.dll" },
		};

		VisualStudioConfig appVsConfigs[] = {
			{ .name = "Debug",   .config = &app },
			{ .name = "Release", .config = &app, .additionalBuildArgs = (const char *[]) { "--release", NULL }, .nmakeOutput = "bin/release/app.exe" },
		};

		VisualStudioProject vsProjects[] = {
			{
				.name			= "mathlib",
				.configs		= mathlibVsConfigs,
				.configsCount	= BUILDER_COUNT_OF( mathlibVsConfigs ),
				.extraFiles		= (const char *[]) {
					"src/mathlib/**/*.h",
					"src/common.h",
					NULL
				},
			},
			{
				.name			= "app",
				.configs		= appVsConfigs,
				.configsCount	= BUILDER_COUNT_OF( appVsConfigs ),
				.extraFiles		= (const char *[]) {
					"src/app/**/*.h",
					"src/common.h",
					NULL
				},
			},
		};

		VisualStudioSolution solution = {
			.name			= "test_generate_visual_studio_files",
			.path			= "visual_studio",
			.platforms		= (const char *[]) { "x64", NULL },
			.projects		= vsProjects,
			.projectsCount	= BUILDER_COUNT_OF( vsProjects ),
		};

		return Builder_GenerateVisualStudioSolution( &options, &solution ) ? 0 : 1;
	}

	return Build( &options );
}
