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

#include "core_array.h"

#include "core_math.h"
#include "linear_allocator.h"
#include "typecast.inl"

#include <memory.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#endif

template<class T>
void Array<T>::init( LinearAllocator *alloc ) {
	allocator = alloc;
	data = NULL;
	count = 0;
	alloced = 0;
}

template<class T>
void Array<T>::zero() {
	allocator = NULL;
	data = NULL;
	count = 0;
	alloced = 0;
}


template<class T>
void Array<T>::reset() {
	count = 0;
}

template<class T>
void Array<T>::add( const T &element ) {
	add_range( &element, 1 );
}

template<class T>
void Array<T>::add_range( const T *ptr, const u64 num_items ) {
	reserve( count + num_items );
	memcpy( data + count, ptr, num_items * sizeof( T ) );
	count += num_items;
}

template<class T>
void Array<T>::add_range( const Array<T> *array ) {
	if ( array->count > 0 ) {
		add_range( array->data, array->count );
	}
}

template<class T>
void Array<T>::swap_remove_at( const u64 index ) {
	T temp = data[count - 1];
	data[count - 1] = data[index];
	data[index] = temp;

	count -= 1;
}

template<class T>
void Array<T>::reserve( const u64 new_alloced ) {
	if ( new_alloced > alloced ) {
		alloced = next_power_of_2_up( new_alloced );

		T *new_data = cast( T*, linear_allocator_alloc( allocator, alloced * sizeof( T ) ) );
		if ( count > 0 ) {
			memcpy( new_data, data, count * sizeof( T ) );
		}
		data = new_data;
	}
}

template<class T>
void Array<T>::resize( const u64 new_count ) {
	reserve( new_count );
	this->count = new_count;
}

template<class T>
T& Array<T>::operator[]( const u64 index ) {
	assert( index < count );
	return data[index];
}

template<class T>
const T& Array<T>::operator[]( const u64 index ) const {
	assert( index < count );
	return data[index];
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
