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
void array_zero( StaticArray<T, N>* array ) {
	assert( array );
	memset( array->data, 0, N * sizeof( T ) );
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
	array_copy( this, other );
}

template<class T>
Array<T>& Array<T>::operator=( const Array<T>& other ) {
	array_copy( this, &other );
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

template<class T>
void array_copy( Array<T>* dst, const Array<T>* src ) {
	array_resize( dst, src->count );
	memcpy( dst->data, src->data, src->count * sizeof( T ) );
}

template<class T>
void array_add( Array<T>* array, const T& element ) {
	array_add_range( array, &element, 1 );
}

void array_add( Array<const char*>* array, const char* element ) {
	array_reserve( array, array->count + 1 );
	array->data[array->count++] = element;
}

void array_add( Array<const wchar_t*>* array, const wchar_t* element ) {
	array_reserve( array, array->count + 1 );
	array->data[array->count++] = element;
}

template<class T>
void array_add_range( Array<T>* array, const T* ptr, const u64 count ) {
	array_reserve( array, array->count + count );
	memcpy( array->data + array->count, ptr, count * sizeof( T ) );
	array->count += count;
}

template<class T>
inline void array_remove_at( Array<T>* array, const u64 index ) {
	assert( index < array->count );
	memcpy( array->data + index, array->data + index + 1, ( array->count - index ) * sizeof( T ) );
	array->count--;
}

template<class T>
void array_resize( Array<T>* array, const u64 count ) {
	array_reserve( array, count );
	array->count = count;
}

template<class T>
void array_reserve( Array<T>* array, const u64 count ) {
	if ( count > array->alloced ) {
		array->alloced = next_po2_up( count );
		array->data = cast( T* ) mem_realloc( array->data, array->alloced * sizeof( T ) );
	}
}

template<class T>
void array_reset( Array<T>* array ) {
	array->count = 0;
}

template<class T>
void array_zero( Array<T>* array ) {
	memset( array->data, 0, array->alloced * sizeof( T ) );
}