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

template<class T, u64 N>
Ring<T, N>::Ring()
	: read_pos( 0 )
	, write_pos( 0 )
	, current_elements( 0 )
{
}

template<class T, u64 N>
void ring_init( Ring<T, N>* ring ) {
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->current_elements = 0;

	array_zero( &ring->data );
	array_resize( &ring->data, N + 1 );
}

template<class T, u64 N>
void ring_reset( Ring<T, N>* ring ) {
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->current_elements = 0;
}

template<class T, u64 N>
bool8 ring_empty( Ring<T, N>* ring ) {
	return ring->read_pos == ring->write_pos;
}

template<class T, u64 N>
bool8 ring_full( Ring<T, N>* ring ) {
	return ring->current_elements == N /*- 1*/;
}

template<class T, u64 N>
void ring_push( Ring<T, N>* ring, const T& element ) {
	assertf( !ring_full( ring ), "Ring overflow." );

	ring->data[ring->write_pos] = element;
	ring->write_pos = ( ring->write_pos + 1 ) % ( ring->data.count );
	ring->current_elements++;
}

template<class T, u64 N>
T& ring_pop( Ring<T, N>* ring ) {
	assertf( !ring_empty( ring ), "Ring underflow." );

	T& result = ring->data[ring->read_pos];

	ring->read_pos = ( ring->read_pos + 1 ) % ( ring->data.count );
	ring->current_elements--;

	return result;
}
