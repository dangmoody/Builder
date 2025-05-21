/*
===========================================================================

Core

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

#ifdef __linux__

#include <timer.h>

#include <time.h>

/*
================================================================================================

	Timer

================================================================================================
*/

// TODO(DM): how do we get clock cycles on linux?
// this isnt it!
s64 time_cycles( void ) {
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );

	int64_t clocks = cast( int64_t, now.tv_sec * 1000000000 + now.tv_nsec );

	return clocks;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"

float64 time_seconds( void ) {
	return cast( float64, time_cycles() / 1000000000.0 );
}

float64 time_ms( void ) {
	return cast( float64, time_cycles() / 1000000.0 );
}

float64 time_us( void ) {
	return cast( float64, time_cycles() / 1000.0 );
}

float64 time_ns( void ) {
	return cast( float64, time_cycles() );
}

#pragma clang diagnostic pop

#endif // __linux__