#include "temper/temper.h"



int main( int argc, char **argv ) {
	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	if ( exitCode != 0 ) {
		DEBUG_BREAK();
	}

	return exitCode;
}
