/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

// Include this file in your source files.

#pragma once

#include "array.h"

#include "debug.h"
#include "allocation_context.h"
#include "math.h"

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
Array<T>::Array( const Array<T>& other ) {
	copy( this, other );
}

template<class T>
void Array<T>::copy( const Array<T>* src ) {
	resize( src->count );
	memcpy( data, src->data, src->count * sizeof( T ) );
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
void Array<T>::resize( const u64 num_items ) {
	reserve( num_items );
	count = num_items;
}

template<class T>
void Array<T>::reserve( const u64 bytes ) {
	if ( bytes > alloced ) {
		alloced = next_po2_up( bytes );
		data = cast( T* ) mem_realloc( data, alloced * sizeof( T ) );
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
	copy( this, &other );
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