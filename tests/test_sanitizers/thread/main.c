#include <stdio.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <pthread.h>
#endif

static int sharedCounter = 0;

#if defined( _WIN32 )
static DWORD WINAPI ThreadFunc( LPVOID arg ) {
#else
static void *ThreadFunc( void *arg ) {
#endif
	for ( int i = 0; i < 100000; i++ ) {
		sharedCounter++;	// unguarded, so two threads hitting this concurrently is a data race
	}

#if defined( _WIN32 )
	return 0;
#else
	return NULL;
#endif
}

// deliberately races two threads on an unguarded shared variable, so ThreadSanitizer should catch this as a data race
int main( int argc, char **argv ) {
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
