#include "../builder.h"

#include <core/array.inl>

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioSolution* solution ) {
	solution->name = "test-sln";
	solution->path = "test_sln";
	array_add( &solution->platforms, "win64" );
	array_add( &solution->configs, "debug" );
	array_add( &solution->configs, "release" );

	// project
	array_add( &solution->projects, {} );
	VisualStudioProject* project = &solution->projects[solution->projects.count - 1];
	project->name = "test-project";
	project->out_path = "bin";
	array_add( &project->source_files, "src/*.cpp" );
}