#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig win64Common = {
		.binary_name			= "SDL",
		.binary_folder			= "bin",
		.binary_type			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files			= { "src/*.c" },
		.defines				= { "_CRT_SECURE_NO_WARNINGS" },
		.additional_includes = {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
		},
	};

	add_build_config( options, &win64Common );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD