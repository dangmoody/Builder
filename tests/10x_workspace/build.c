#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#define BUILDER_10X_IMPLEMENTATION
#include "../../builder_10x.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	BuildConfig *debug = CreateBuildConfig( &options );
	*debug = (BuildConfig) {
		.name			= "Debug",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_generate_10x_workspace",
		.binaryFolder	= "bin/debug",
		.sourceFiles	= MakeStringList( "main.c" ),
		.defines		= MakeStringList( "TEST_DEBUG" ),
	};

	BuildConfig *release = CreateBuildConfig( &options );
	*release = (BuildConfig) {
		.name			= "Release",
		.binaryType		= BINARY_TYPE_EXE,
		.binaryName		= "test_generate_10x_workspace",
		.binaryFolder	= "bin/release",
		.sourceFiles	= MakeStringList( "main.c" ),
		.defines		= MakeStringList( "TEST_RELEASE" ),
		.optimization	= OPTIMIZATION_PROGRAM_SPEED,
		.removeSymbols	= true,
	};

	options.defaultConfig = debug;

	if ( HasCommandLineArg( argc, argv, "--10x" ) ) {
		TenXPlatform platforms[] = {
			{ .name = "x64",	.defines = MakeStringList( "TEST_PLATFORM_X64" ) },
			{ .name = "Win32",	.defines = MakeStringList( "TEST_PLATFORM_WIN32" ) },
		};

		TenXConfig configs[] = {
			{
				.name					= "Debug",
				.config					= debug,
				.additionalBuildArgs	= MakeStringList( "--config=Debug" ),
				.runArgs				= "--verbose",
			},
			{
				.name					= "Release",
				.config					= release,
				.additionalBuildArgs	= MakeStringList( "--config=Release" ),
				.prioritiseFiles		= MakeStringList( "main.c" ),
				.binaryName				= "test_generate_10x_workspace_renamed",
				.binaryRelativePath		= "bin/custom",
				.runCommand				= "$(Executable) --from-run-command",
				.runArgs				= "--test --fast",
			},
		};

		TenXWorkspace workspace = {
			.name					= "test_generate_10x_workspace",
			.configs				= configs,
			.configsCount			= BUILDER_COUNT_OF( configs ),
			.platforms				= platforms,
			.platformCount			= BUILDER_COUNT_OF( platforms ),
			.globalDefines			= MakeStringList( "TEST_GLOBAL_DEFINE" ),
			.includeFilterList		= MakeStringList( "*.c", "*.h" ),
			.excludeFilterList		= MakeStringList( "bin/*" ),
			.syncFiles				= true,
			.recursive				= true,
			.captureExeOutput		= true,
			.allowAutoErrorUnderlining	= true,
		};

		// lets the test check the workspace file ends up in TenXWorkspace::path when one is set
		if ( HasCommandLineArg( argc, argv, "--10x-path" ) ) {
			workspace.path = "workspace_out";
		}

		return Builder_Generate10xWorkspace( &options, &workspace, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
