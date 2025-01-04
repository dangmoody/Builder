/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef _WIN64

#include <timer.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
================================================================================================

	Timer

================================================================================================
*/

static s64 get_frequency( void ) {
	LARGE_INTEGER frequency = {};
	QueryPerformanceFrequency( &frequency );
	return frequency.QuadPart;
}

s64 time_cycles( void ) {
	LARGE_INTEGER now = {};
	QueryPerformanceCounter( &now );
	return now.QuadPart;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"

float64 time_seconds( void ) {
	return cast( float64 ) time_cycles() / cast( float64 ) get_frequency();
}

float64 time_ms( void ) {
	return ( cast( float64 ) ( time_cycles() * 1000 ) / get_frequency() );
}

float64 time_us( void ) {
	return ( cast( float64 ) ( time_cycles() * 1000000 ) / get_frequency() );
}

float64 time_ns( void ) {
	return ( cast( float64 ) ( time_cycles() * 1000000000 ) / get_frequency() );
}

#pragma clang diagnostic pop

#endif // _WIN64