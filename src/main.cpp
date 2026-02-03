#include "builder_local.h"

// the first arg here is just "builder.exe" which we dont care about
// so skip that here and process args[1] onwards

int main( int argc, char** argv ) {
	return BuilderMain( 1, argc, argv );
}