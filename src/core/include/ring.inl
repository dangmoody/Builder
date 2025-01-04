/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

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
