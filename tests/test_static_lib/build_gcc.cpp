// this file was auto generated
// do not edit

#include <builder.h>

#include "build_configs.cpp"

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions* options ) {
	options->compilerPath = "../../tools/gcc/bin/gcc";
	options->compilerVersion = "15.1.0";
	GetBuildConfigs( options );
}
