#include "geometry.h"

MATHLIB_API float Geometry_DistanceSquared( const Vec2 a, const Vec2 b ) {
	float dx = b.x - a.x;
	float dy = b.y - a.y;

	return ( dx * dx ) + ( dy * dy );
}
