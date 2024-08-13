/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "core_types.h"
#include "array.h"

/*
================================================================================================

	Ring Buffer

	Fixed-size circular buffer.

	Based on implementation idea provided by Fabien Giesen:
	https://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/

	We use "Method 1" from that link.

	The array that the ring buffer holds is always created with N + 1 elements.  This is
	because when completely filling the queue you end up with a case where read_pos == write_pos
	so we get around this by not allowing you to write to the queue when the queue contains
	N - 1 elements (where N is the size you requested + 1).  The total cost of this for any
	program (in bytes) using this approach is 1 * sizeof( T ) per queue.

	See the link above for more information.

================================================================================================
*/

template<class T, u64 N>
struct Ring {
	Array<T>	data;	// TODO(DM): 18/02/2024 make this StaticArray<T, N>
	u64			read_pos;
	u64			write_pos;
	u64			current_elements;

				Ring();
				~Ring() {}
};

template<class T, u64 N>
inline void		ring_init( Ring<T, N>* ring );

template<class T, u64 N>
inline void		ring_reset( Ring<T, N>* ring );

template<class T, u64 N>
inline bool8	ring_empty( Ring<T, N>* ring );

template<class T, u64 N>
inline bool8	ring_full( Ring<T, N>* ring );

template<class T, u64 N>
inline void		ring_push( Ring<T, N>* ring, const T& element );

template<class T, u64 N>
inline T&		ring_pop( Ring<T, N>* ring );
