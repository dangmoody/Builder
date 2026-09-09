#include <stdio.h>
#include <limits.h>

// deliberately overflows a signed integer, so UndefinedBehaviorSanitizer should catch this as signed-integer-overflow
int main( int argc, char **argv ) {
	int value = INT_MAX;

	value += argc;	// signed overflow (argc is always at least 1)

	printf( "%d\n", value );

	return 0;
}
