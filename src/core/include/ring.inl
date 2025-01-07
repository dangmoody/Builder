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

#include "ring.h"
#include "debug.h"
#include "array.inl"

template<class T, u64 N>
Ring<T, N>::Ring()
	: read_pos( 0 )
	, write_pos( 0 )
	, current_elements( 0 )
{
}

template<class T, u64 N>
void Ring<T, N>::init() {
	read_pos = 0;
	write_pos = 0;
	current_elements = 0;

	data.zero();
	data.resize( N + 1 );
}

template<class T, u64 N>
void Ring<T, N>::reset() {
	read_pos = 0;
	write_pos = 0;
	current_elements = 0;
}

template<class T, u64 N>
bool8 Ring<T, N>::empty() const {
	return read_pos == write_pos;
}

template<class T, u64 N>
bool8 Ring<T, N>::full() const {
	return current_elements == N /*- 1*/;
}

template<class T, u64 N>
void Ring<T, N>::push( const T& element ) {
	assertf( !full(), "Ring overflow." );

	data[write_pos] = element;
	write_pos = ( write_pos + 1 ) % ( data.count );
	current_elements++;
}

template<class T, u64 N>
T& Ring<T, N>::pop() {
	assertf( !empty(), "Ring underflow." );

	T& result = data[read_pos];

	read_pos = ( read_pos + 1 ) % ( data.count );
	current_elements--;

	return result;
}
