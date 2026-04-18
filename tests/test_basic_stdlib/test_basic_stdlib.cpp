// this is the "basic" test but requires the standard library, so we can test that Builder can link against the standard library without any special configuration
// its meant to serve as the easiest, most minimal example of what builder can build
// we should still be able to build programs with Builder without using any of the Builder-specific entry points

#include <iostream>

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	std::cout << "libcmt.lib required." << std::endl;

	return 0;
}