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

	// project configs
	VisualStudioConfig* configDebug = add_visual_studio_config( project );
	configDebug->name = "debug";
	configDebug->out_path = "bin\\debug";
	array_add( &configDebug->definitions, "_DEBUG" );

	VisualStudioConfig* configRelease = add_visual_studio_config( project );
	configRelease->name = "release";
	configRelease->out_path = "bin\\release";
	array_add( &configRelease->definitions, "NDEBUG" );
}