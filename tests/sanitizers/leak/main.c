#include <stdlib.h>

// deliberately leaks a heap allocation, so LeakSanitizer should catch this as a memory leak on exit
int main( int argc, char **argv ) {
	void *leaked = malloc( 64 );

	(void) leaked;

	return 0;
}
