// this is the "basic" test
// its meant to serve as the easiest, most minimal example of what builder can build
// we should still be able to build programs with Builder without using any of the Builder-specific entry points

#include <stdio.h>

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	printf( "Hello world.\n" );

	return 0;
}