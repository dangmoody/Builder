#include "utils.h"

#include <stdio.h>

void Utils_PrintVec2( const char *label, const Vec2 v ) {
	printf( "%s: (%.2f, %.2f)\n", label, v.x, v.y );
}
