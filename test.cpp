#include <stdio.h>

#include "builder.h"

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	array_add( &options->defines, "TITS=1" );
}

int main( int argc, char** argv ) {
	((void) argc);
	((void) argv);

	printf( "Wow, what a shit language.\n" );

	return 0;
}