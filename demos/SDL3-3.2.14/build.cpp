#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig win64Common = {
		.source_files	= { "src" },
		.binary_name	= "SDL",
		.binary_folder	= "bin",
		.binary_type	= BINARY_TYPE_DYNAMIC_LIBRARY,
	};

	add_build_config( options, &win64Common );
}