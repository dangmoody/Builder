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

// Include this file in your header file.

#pragma once

#include "core_types.h"

struct Allocator;

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
	Allocator*	allocator;
	
				Array();
				Array( const Array<T>& other );
				~Array();

	void		copy( const Array<T>* src );

	void		add( const T& element );

	void		add_range( const T* ptr, const u64 count );
	void		add_range( const Array<T>* array );

	void		remove_at( const u64 index );
	void		swap_remove_at( const u64 index );

	void		resize( const u64 count );
	void		reserve( const u64 count );

	void		reset();
	void		zero();

	Array<T>&	operator=( const Array<T>& other );

	T&			operator[]( const u64 index );
	const T&	operator[]( const u64 index ) const;
};