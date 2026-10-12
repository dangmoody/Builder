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

	BuildConfig *config = CreateBuildConfig( &options );
	config->name = "Debug";
	config->binaryType = BINARY_TYPE_EXE;
	config->binaryName = "test_generate_10x_workspace";
	AddSourceFiles( config, "main.c" );

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		config->binaryFolder = "bin/release";
	} else {
		config->binaryFolder = "bin/debug";
	}

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--10x" ) ) {

		TenXPlatform platforms[] = {
			{ .name = "Win32" }
		};

		TenXConfig configs[] = {
			{ 
				.name = "Debug",
				.config = config,
				.additionalBuildArgs = MakeStringList("--config=Debug"),
				.rebuildCommand = "Heyhey.exe"
			}
		};

		TenXWorkspace workspace = { 0 };
		workspace.name = "Builder_10x_workspaces";
		workspace.configs = configs;
		workspace.configsCount = BUILDER_COUNT_OF( configs );
		workspace.platforms = platforms;
		workspace.platformCount = BUILDER_COUNT_OF( platforms );

		return Builder_Generate10xWorkspace( &options, &workspace, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
