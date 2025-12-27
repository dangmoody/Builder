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

#include "int_types.h"
#include "dll_export.h"

/*
================================================================================================

	String Builder

	Use this if you want to dynamically append content to a string.

	Every time you append to a StringBuilder it puts that string into a "buffer".  Buffers are
	stored in a linked list.

================================================================================================
*/

// TODO(DM): 23/12/2025: add 4KB static char array and use that if the string fits in that
struct StringBuilderBuffer {
	u32						length;
	char*					data;
	StringBuilderBuffer*	next;
};

struct StringBuilder {
	StringBuilderBuffer*	head;
	StringBuilderBuffer*	tail;
};

CORE_API void			string_builder_reset( StringBuilder* builder );
CORE_API void			string_builder_destroy( StringBuilder* builder );

CORE_API void			string_builder_appendf( StringBuilder* builder, const char* fmt, ... );

CORE_API const char*	string_builder_to_string( StringBuilder* builder );
