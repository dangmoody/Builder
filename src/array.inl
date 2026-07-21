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

#include "array.h"

#include "math.h"
#include "linear_allocator.h"
#include "typecast.h"

#include <memory.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#endif

template<class T>
void Array<T>::Init( LinearAllocator *alloc ) {
	allocator = alloc;
	data = NULL;
	count = 0;
	alloced = 0;
}

template<class T>
void Array<T>::Zero() {
	allocator = NULL;
	data = NULL;
	count = 0;
	alloced = 0;
}


template<class T>
void Array<T>::Reset() {
	count = 0;
}

template<class T>
void Array<T>::Add( const T &element ) {
	AddRange( &element, 1 );
}

template<class T>
void Array<T>::AddRange( const T *ptr, const u64 numItems ) {
	Reserve( count + numItems );
	memcpy( data + count, ptr, numItems * sizeof( T ) );
	count += numItems;
}

template<class T>
void Array<T>::AddRange( const Array<T> *array ) {
	if ( array->count > 0 ) {
		AddRange( array->data, array->count );
	}
}

template<class T>
void Array<T>::Reserve( const u64 newAlloced ) {
	if ( newAlloced > alloced ) {
		alloced = NextPowerOf2Up( newAlloced );

		T *newData = cast( T*, Mem_AllocatorAlloc( allocator, alloced * sizeof( T ) ) );
		if ( count > 0 ) {
			memcpy( newData, data, count * sizeof( T ) );
		}
		data = newData;
	}
}

template<class T>
void Array<T>::Resize( const u64 newCount ) {
	Reserve( newCount );
	this->count = newCount;
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
