/*
===========================================================================

Builder

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#include "math.h"

#include <math.h>

bool8 Float64Equals( const float64 a, const float64 b ) {
	return fabs( a - b ) < 0.0001;
}

u32 Min( const u32 a, const u32 b ) {
	return ( a < b ) ? a : b;
}

u64 Min( const u64 a, const u64 b ) {
	return ( a < b ) ? a : b;
}

u32 Max( const u32 a, const u32 b ) {
	return ( a > b ) ? a : b;
}

u64 Max( const u64 a, const u64 b ) {
	return ( a > b ) ? a : b;
}

u64	NextPowerOf2Up( const u64 number ) {
	return ( number == 1 ) ? 1 : 1 << ( 64 - GetNumLeadingZeros( number - 1 ) );
}

