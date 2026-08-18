#include "common.h"
#include "mathlib/mathlib.h"
#include "mathlib/geometry/geometry.h"
#include "utils/utils.h"

#include <stdio.h>

int main( int argc, char **argv ) {
#if defined( _DEBUG )
	printf( "DEBUG MODE\n" );
#elif defined( NDEBUG )
	printf( "RELEASE MODE\n" );
#endif

	int sum = MathLib_Add( 2, 3 );
	int diff = MathLib_Subtract( 5, 2 );

	printf( "sum = %d, diff = %d\n", sum, diff );

	Vec2 a = { 0.0f, 0.0f };
	Vec2 b = { 3.0f, 4.0f };

	float distSq = Geometry_DistanceSquared( a, b );

	printf( "distSq = %.2f\n", distSq );

	Utils_PrintVec2( "b", b );

	return 0;
}
