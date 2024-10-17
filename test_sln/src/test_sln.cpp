#include <builder.h>

// TODO(DM): 10/10/2024: figure out how we give the user a nice way of doing this
static VisualStudioProject* add_visual_studio_project( VisualStudioSolution* solution ) {
	solution->projects.push_back( {} );

	return &solution->projects[solution->projects.size() - 1];
}

static VisualStudioConfig* add_visual_studio_config( VisualStudioProject* project ) {
	project->configs.push_back( {} );

	return &project->configs[project->configs.size() - 1];
}

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioSolution* solution ) {
	solution->name = "test-sln";
	solution->path = "..";
	solution->platforms.push_back( "win64" );

	// project
	VisualStudioProject* project = add_visual_studio_project( solution );
	project->name = "test-project";
	project->source_files.push_back( "src/*.cpp" );

	BuilderOptions options = {};
	options.source_files.push_back( "main.cpp" );
	options.binary_name = "test";

	// project configs
	VisualStudioConfig* configDebug = add_visual_studio_config( project );
	configDebug->name = "debug";
	configDebug->build_source_file = "src/main.cpp";
	configDebug->output_directory = "bin/debug";
	configDebug->options = options;
	configDebug->options.binary_folder = "../bin/debug";
	configDebug->options.optimization_level = OPTIMIZATION_LEVEL_O0;
	configDebug->options.defines.push_back( "_DEBUG" );

	VisualStudioConfig* configRelease = add_visual_studio_config( project );
	configRelease->name = "release";
	configRelease->build_source_file = "src/main.cpp";
	configRelease->output_directory = "bin/release";
	configRelease->options = options;
	configRelease->options.binary_folder = "../bin/release";
	configRelease->options.optimization_level = OPTIMIZATION_LEVEL_O3;
	configRelease->options.defines.push_back( "NDEBUG" );
}