// this file was auto generated
// do not edit

#include <builder.h>

#include "build_configs.cpp"

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions* options ) {
	options->compilerPath = "../../clang/bin/clang";
	options->compilerVersion = "20.1.5";
	GetBuildConfigs( options );
}
