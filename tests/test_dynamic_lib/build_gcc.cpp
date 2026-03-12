// this file was auto generated
// do not edit

#include <builder.h>

#include "build_configs.cpp"

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions* options ) {
#if defined( _WIN32 )
	options->compilerPath = "../../tools/gcc/bin/gcc";
#elif defined( __linux__ )
	options->compilerPath = "gcc";
#else
#error Unrecognised platform.
#endif
	options->compilerVersion = "15.1.0";
	GetBuildConfigs( options );
}
