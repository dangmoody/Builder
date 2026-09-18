#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#include "../test_compiler_override.h"

// the self rebuild config is the only one registered, so there's nothing for Builder to actually build
// Build() should refuse with an error rather than trying to build the self rebuild config as the target
int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };
	ApplyCompilerOverride( &options, argc, argv );

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	return Build( &options, argc, argv );
}
