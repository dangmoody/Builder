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

#pragma once

#include "int_types.h"

/*
================================================================================================

	Core Helpers

	TODO: DM: 23/12/2025: core_helpers.h doesnt seem like the right name for this header

================================================================================================
*/

// returns number of elements in static array
#define count_of( x )					( sizeof( (x) ) / sizeof( (x)[0] ) )

// use this to avoid compiler warning about unused variable if you need to keep it
#define unused( x )						( (void) (x) )

// for loop helper macro
// DM: these exist because I'm getting bored of typing the whole thing out every time and it feels like boiler-plate
#define For( Type, it, start, count )	for ( Type it = (start); it < (count); it++ )

// reverse for loop helper macro
#define RFor( Type, it, start, count )	for ( Type it = (count); it-- > (start); )

// memory conversion helpers
#define MEM_KILOBYTES( x )	( cast( u64, (x) ) * 1000 )
#define MEM_MEGABYTES( x )	( MEM_KILOBYTES( x ) * 1000 )
#define MEM_GIGABYTES( x )	( MEM_MEGABYTES( x ) * 1000 )
