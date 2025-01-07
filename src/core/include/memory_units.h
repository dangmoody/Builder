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

enum MemoryAlignment {
	MEMORY_ALIGNMENT_ONE = 1,
	MEMORY_ALIGNMENT_TWO = 2,
	MEMORY_ALIGNMENT_FOUR = 4,
	MEMORY_ALIGNMENT_EIGHT = 8,
	MEMORY_ALIGNMENT_SIXTEEN = 16
};

// memory conversion helpers
#define MEM_KILOBYTES( x )	( cast( u64 ) (x) * 1000 )
#define MEM_MEGABYTES( x )	( MEM_KILOBYTES( x ) * 1000 )
#define MEM_GIGABYTES( x )	( MEM_MEGABYTES( x ) * 1000 )
