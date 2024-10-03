#include "../builder.h"

#include <core/array.inl>

static VisualStudioProject* add_visual_studio_project( VisualStudioSolution* solution ) {
	array_add( &solution->projects, {} );

	return &solution->projects[solution->projects.count - 1];
}

static VisualStudioConfig* add_visual_studio_config( VisualStudioProject* project ) {
	array_add( &project->configs, {} );

	return &project->configs[project->configs.count - 1];
}

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioSolution* solution ) {
	solution->name = "test-sln";
	solution->path = "test_sln";
	array_add( &solution->platforms, "win64" );

	// project
	VisualStudioProject* project = add_visual_studio_project( solution );
	project->name = "test-project";
	array_add( &project->source_files, "src/*.cpp" );

	BuilderOptions options = {};
	array_add( &options.source_files, "main.cpp" );
	options.binary_name = "test";

	// project configs
	VisualStudioConfig* configDebug = add_visual_studio_config( project );
	configDebug->name = "debug";
#if !VS_GENERATE_BUILD_SOURCE_FILES
	configDebug->build_source_file = "src/main.cpp";
#endif
	configDebug->options = options;
	configDebug->options.binary_folder = "../bin/debug";
	configDebug->options.optimization_level = OPTIMIZATION_LEVEL_O0;
	array_add( &configDebug->options.defines, "_DEBUG" );

	VisualStudioConfig* configRelease = add_visual_studio_config( project );
	configRelease->name = "release";
#if !VS_GENERATE_BUILD_SOURCE_FILES
	configRelease->build_source_file = "src/main.cpp";
#endif
	configRelease->options = options;
	configRelease->options.binary_folder = "../bin/release";
	configRelease->options.optimization_level = OPTIMIZATION_LEVEL_O3;
	array_add( &configRelease->options.defines, "NDEBUG" );
}