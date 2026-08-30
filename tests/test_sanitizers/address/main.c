#include <stdio.h>
#include <stdlib.h>

// deliberately reads past the end of a heap allocation, so AddressSanitizer should catch this as a heap-buffer-overflow
int main( int argc, char **argv ) {
	char *buffer = (char *) malloc( 8 );

	buffer[0] = 'h';
	buffer[8] = 'i';	// out of bounds write

	printf( "%c\n", buffer[8] );

	free( buffer );

	return 0;
}
