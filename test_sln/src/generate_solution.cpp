#include <builder.h>

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioSolution* solution ) {
	solution->name = "test-sln";
	solution->path = "..";
	solution->platforms.push_back( "win64" );

	// project
	VisualStudioProject* project = add_visual_studio_project( solution );
	project->name = "test-project";
	project->source_files.push_back( "src/*.cpp" );

	BuilderOptions options = {};
	options.source_files.push_back( "src/main.cpp" );
	options.binary_name = "test";

	// project configs
	VisualStudioConfig* configDebug = add_visual_studio_config( project );
	configDebug->name = "debug";
	configDebug->output_directory = "bin/debug";
	configDebug->options = options;
	configDebug->options.binary_folder = "../bin/debug";
	configDebug->options.optimization_level = OPTIMIZATION_LEVEL_O0;
	configDebug->options.defines.push_back( "_DEBUG" );

	VisualStudioConfig* configRelease = add_visual_studio_config( project );
	configRelease->name = "release";
	configRelease->output_directory = "bin/release";
	configRelease->options = options;
	configRelease->options.binary_folder = "../bin/release";
	configRelease->options.optimization_level = OPTIMIZATION_LEVEL_O3;
	configRelease->options.defines.push_back( "NDEBUG" );
}