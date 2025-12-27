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

#ifdef _WIN32

#include <timer.h>
#include <typecast.inl>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
	return cast( float64, time_cycles() ) / cast( float64, get_frequency() );
}

float64 time_ms( void ) {
	return cast( float64, time_cycles() * 1000 ) / get_frequency();
}

float64 time_us( void ) {
	return cast( float64, time_cycles() * 1000000 ) / get_frequency();
}

float64 time_ns( void ) {
	return cast( float64, time_cycles() * 1000000000 ) / get_frequency();
}

#pragma clang diagnostic pop

#endif // _WIN32