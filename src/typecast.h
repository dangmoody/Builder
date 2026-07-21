/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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
#include "debug.h"

/*
================================================================================================

	Typecasting

================================================================================================
*/

// Call these ones!
#define cast( Type, x )				(Type) (x)
#define trunc_cast( Type, x )		TruncCastInternal<Type>( (x) )


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"

// DO NOT CALL THIS DIRECTLY!
// USE THE MACROS ABOVE!
template<class OutType, class InType>
OutType TruncCastInternal( const InType in ) {
	//bool8 isInputFloatingPoint = cast( InType, 0.5 ) != 0;
	bool8 isOutputFloatingPoint = cast( OutType, 0.5 ) != 0;

	bool8 isInputSigned = cast( InType, -1 ) < 0;
	bool8 isOutputSigned = cast( OutType, -1 ) < 0;

	OutType minOutputValue = 0;
	OutType maxOutputValue = 0;

	if ( isOutputFloatingPoint ) {
		minOutputValue = cast( OutType, -FLOAT32_MAX );
		maxOutputValue = cast( OutType, FLOAT32_MAX );
	} else {
		if ( isOutputSigned ) {
			maxOutputValue = cast( OutType, ( 1ULL << ( sizeof( OutType ) * 8 - 1 ) ) - 1 );
			minOutputValue = cast( OutType, -maxOutputValue - 1 );
		} else {
			minOutputValue = 0;
			maxOutputValue = cast( OutType, ~0 );
		}
	}

	if ( isInputSigned ) {
		assert( in >= cast( OutType, minOutputValue ) );
	}

	assert( in <= cast( OutType, maxOutputValue ) );

	return cast( OutType, in );
}

#pragma clang diagnostic pop
