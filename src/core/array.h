/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

// Include this file in your header file.

#pragma once

#include "core_types.h"

template<class T, u64 N>
struct StaticArray {
	T			data[N];

				StaticArray();
				~StaticArray() {}

	T&			operator[]( const u64 index );
	const T&	operator[]( const u64 index ) const;
};

template<class T, u64 N>
inline void		array_zero( StaticArray<T, N>* array );


template<class T>
struct Array {
	T*			data;
	u64			count;
	u64			alloced;

				Array();
				~Array();

				Array( const Array<T>& other );

	Array<T>&	operator=( const Array<T>& other );

	T&			operator[]( const u64 index );
	const T&	operator[]( const u64 index ) const;
};

template<class T>
inline void		array_copy( Array<T>* dst, const Array<T>* src );

template<class T>
inline void		array_add( Array<T>* array, const T& element );

// we need special overrides for char* and wchar_t* because C++ template bullshit
inline void		array_add( Array<const char*>* array, const char* element );
inline void		array_add( Array<const wchar_t*>* array, const wchar_t* element );

template<class T>
inline void		array_add_range( Array<T>* arary, const T* ptr, const u64 count );

template<class T>
inline void		array_remove_at( Array<T>* array, const u64 index );

template<class T>
inline void		array_resize( Array<T>* array, const u64 count );

template<class T>
inline void		array_reserve( Array<T>* array, const u64 count );

template<class T>
inline void		array_reset( Array<T>* array );

template<class T>
inline void		array_zero( Array<T>* array );