// this file was auto generated
// do not edit

#include <builder.h>

#include "build_configs.cpp"

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->compiler_path = "../../clang/bin/clang";
	options->compiler_version = "20.1.5";
	get_build_configs( options );
}
