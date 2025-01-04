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

	void		zero();

	T&			operator[]( const u64 index );
	const T&	operator[]( const u64 index ) const;
};


template<class T>
struct Array {
	T*			data;
	u64			count;
	u64			alloced;

				Array();
				~Array();

				Array( const Array<T>& other );

	void		copy( const Array<T>* src );

	void		add( const T& element );

	void		add_range( const T* ptr, const u64 count );

	void		remove_at( const u64 index );

	void		resize( const u64 count );
	void		reserve( const u64 count );

	void		reset();
	void		zero();

	Array<T>&	operator=( const Array<T>& other );

	T&			operator[]( const u64 index );
	const T&	operator[]( const u64 index ) const;
};