#include <stdio.h>
#include <stdlib.h>

// deliberately branches on an uninitialized value, so MemorySanitizer should catch this as use-of-uninitialized-value
int main( int argc, char **argv ) {
	int *value = (int *) malloc( sizeof( int ) );

	if ( *value == 0 ) {
		printf( "value was zero\n" );
	} else {
		printf( "value was not zero\n" );
	}

	free( value );

	return 0;
}
