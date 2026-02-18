#include <stdio.h>

extern int helper_function();

int main( int argc, char** argv ) {
    ( (void) argc );
    ( (void) argv );

    printf( "Result: %d\n", helper_function() );

    return 0;
}