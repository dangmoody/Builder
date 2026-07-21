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

#include "int_types.h"

struct LinearAllocator;

/*
================================================================================================

	String Builder

	Use this if you want to dynamically append content to a string.

	Every time you append to a StringBuilder it puts that string into a "buffer".  Buffers are
	stored in a linked list.

================================================================================================
*/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

// TODO: DM: 23/12/2025: add 4KB static char array and use that if the string fits in that
struct StringBuilderBuffer {
	u32						length;
	char					*data;
	StringBuilderBuffer		*next;
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

struct StringBuilder {
	LinearAllocator		*allocator;
	StringBuilderBuffer	*head;
	StringBuilderBuffer	*tail;
};

StringBuilder	string_builder_create( LinearAllocator *allocator );

void			string_builder_appendf( StringBuilder *builder, const char *fmt, ... );

const char		*string_builder_to_string( StringBuilder *builder );
