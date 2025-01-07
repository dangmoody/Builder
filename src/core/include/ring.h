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

	void		init();
	void		reset();

	bool8		empty() const;
	bool8		full() const;

	void		push( const T& element );
	T&			pop();
};