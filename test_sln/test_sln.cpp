#include "../builder.h"

#include <core/array.inl>

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioOptions* options ) {
	options->project_name = "FUCK_YEAH";

	array_add( &options->platforms, "win64" );

	array_add( &options->configs, "debug" );
	array_add( &options->configs, "release" );

	array_add( &options->source_files, "main.cpp" );
}