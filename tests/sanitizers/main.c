#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "sanitizer_test_args.h"

static int sharedCounter = 0;

#if defined( _WIN32 )
static DWORD WINAPI ThreadFunc( LPVOID arg ) {
#else
static void *ThreadFunc( void *arg ) {
#endif
	(void) arg;

	for ( int i = 0; i < 100000; i++ ) {
		sharedCounter++;	// unguarded, so two threads hitting this concurrently is a data race
	}

#if defined( _WIN32 )
	return 0;
#else
	return NULL;
#endif
}

// argv[1] picks which bug to deliberately trigger, so the matching sanitizer should catch it
int main( int argc, char **argv ) {
	if ( argc < 2 ) {
		printf( "Usage: %s <sanitizer>\n", argv[0] );
		return 1;
	}

	const char *bug = argv[1];

	// signed integer overflow (argc is always at least 1)
	if ( strcmp( bug, SANITIZER_TEST_ARG_UNDEFINED_BEHAVIOR ) == 0 ) {
		int value = INT_MAX;

		value += argc;

		printf( "%d\n", value );

		return 0;
	}

	// branch on an uninitialized value
	if ( strcmp( bug, SANITIZER_TEST_ARG_MEMORY ) == 0 ) {
		int *value = (int *) malloc( sizeof( int ) );

		if ( *value == 0 ) {
			printf( "value was zero\n" );
		} else {
			printf( "value was not zero\n" );
		}

		free( value );

		return 0;
	}

	// heap buffer overflow
	if ( strcmp( bug, SANITIZER_TEST_ARG_ADDRESS ) == 0 ) {
		char *buffer = (char *) malloc( 8 );

		buffer[0] = 'h';
		buffer[8] = 'i';	// out of bounds write

		printf( "%c\n", buffer[8] );

		free( buffer );

		return 0;
	}

	// memory leak
	// null the pointer so leak sanitizer cant find it still lingering on the stack
	if ( strcmp( bug, SANITIZER_TEST_ARG_LEAK ) == 0 ) {
		void *leaked = malloc( 64 );

		printf( "leaked %p\n", leaked );

		leaked = NULL;

		return 0;
	}

	// data race on an unguarded shared variable
	if ( strcmp( bug, SANITIZER_TEST_ARG_THREAD ) == 0 ) {
#if defined( _WIN32 )
		HANDLE threads[2];
		threads[0] = CreateThread( NULL, 0, ThreadFunc, NULL, 0, NULL );
		threads[1] = CreateThread( NULL, 0, ThreadFunc, NULL, 0, NULL );
		WaitForSingleObject( threads[0], INFINITE );
		WaitForSingleObject( threads[1], INFINITE );
		CloseHandle( threads[0] );
		CloseHandle( threads[1] );
#else
		pthread_t threads[2];
		pthread_create( &threads[0], NULL, ThreadFunc, NULL );
		pthread_create( &threads[1], NULL, ThreadFunc, NULL );
		pthread_join( threads[0], NULL );
		pthread_join( threads[1], NULL );
#endif

		printf( "sharedCounter = %d\n", sharedCounter );

		return 0;
	}

	printf( "Unknown sanitizer \"%s\".\n", bug );

	return 1;
}
