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
// Include this file in your source files.

#pragma once

#include "array.h"

#include "debug.h"
#include "allocation_context.h"
#include "core_math.h"

#include <memory.h>

/*
================================================================================================

	StaticArray

================================================================================================
*/

template<class T, u64 N>
StaticArray<T, N>::StaticArray() {
	memset( data, 0, N * sizeof( T ) );
}

template<class T, u64 N>
T& StaticArray<T, N>::operator[]( const u64 index ) {
	assert( index < N );
	return data[index];
}

template<class T, u64 N>
const T& StaticArray<T, N>::operator[]( const u64 index ) const {
	assert( index < N );
	return data[index];
}

template<class T, u64 N>
void StaticArray<T, N>::zero() {
	memset( data, 0, N * sizeof( T ) );
}


/*
================================================================================================

	Array

================================================================================================
*/

template<class T>
Array<T>::Array()
	: data( NULL )
	, count( 0 )
	, alloced( 0 )
	, allocator( NULL )
{

}

template<class T>
Array<T>::~Array() {
	if ( data ) {
		mem_free( data );
		data = NULL;
	}
}

template<class T>
Array<T>::Array( const Array<T>& other )
: data( NULL )
	, count( 0 )
	, alloced( 0 )
	, allocator( NULL )
{
	copy( other );
}

template<class T>
void Array<T>::copy( const Array<T>* src ) {
	resize( src->count );
	memcpy( data, src->data, src->count * sizeof( T ) );
	count = src->count;
}

template<class T>
void Array<T>::add( const T& element ) {
	add_range( &element, 1 );
}

template<class T>
void Array<T>::add_range( const T* ptr, const u64 num_items ) {
	reserve( count + num_items );
	memcpy( data + count, ptr, num_items * sizeof( T ) );
	count += num_items;
}

template<class T>
inline void Array<T>::remove_at( const u64 index ) {
	assert( index < count );
	memcpy( data + index, data + index + 1, ( count - index ) * sizeof( T ) );
	count--;
}

template<class T>
inline void	Array<T>::swap_remove_at( const u64 index ) {
	assert( index < count );
	data[index] = data[count-1];
	count--;
}

template<class T>
void Array<T>::resize( const u64 num_items ) {
	reserve( num_items );
	count = num_items;
}

template<class T>
void Array<T>::reserve( const u64 bytes ) {
	if ( bytes > alloced ) {
		u64 previous_alloced = alloced;
		alloced = next_multiple_of_4_up( bytes );

		allocator = allocator == nullptr ? mem_get_current_allocator() : allocator;

		mem_push_allocator( allocator );
		// TODO(DM): 07/02/2025: using our own cast( T, x ) function doesnt work here when we enable CORE_MEMORY_TRACKING
		// this is because in that instance mem_realloc calls track_allocation_internal() followed immediately by track_free_internal(), which the compiler wont like if we were to do the following:
		//
		//	data = cast( T*, mem_realloc( data, alloced * sizeof( T ) ) );
		//
		// therefore we need to make the mem_realloc() define call just the one function instead of one after the other
		data = (T*) mem_realloc( data, alloced * sizeof( T ) );
		mem_pop_allocator();
	}
}

template<class T>
void Array<T>::reset() {
	count = 0;
}

template<class T>
void Array<T>::zero() {
	memset( data, 0, alloced * sizeof( T ) );
}

template<class T>
Array<T>& Array<T>::operator=( const Array<T>& other ) {
	copy( &other );
	return *this;
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