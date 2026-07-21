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

#ifdef __linux__

#include "../memory.h"

#include <stddef.h>

#include <sys/mman.h>

void	*virtual_reserve( const u64 size_bytes ) {
	return mmap( NULL, size_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
}

void	*virtual_commit( void *ptr, const u64 size_bytes ) {
	if ( mprotect( ptr, size_bytes, PROT_READ | PROT_WRITE ) == -1 ) {
		return NULL;
	}

	return ptr;	// TODO: DM: 21/03/2026: is that correct?
}

void	virtual_decommit( void *ptr, const u64 size_bytes ) {
	if ( madvise( ptr, size_bytes, MADV_DONTNEED ) == -1 ) {
		// TODO: DM: 21/03/2026: handle error
	}
}

void	virtual_free( void *ptr ) {
	// TODO: DM: 21/03/2026: is -1 OK here?
	if ( munmap( ptr, 0 ) == -1 ) {
		// TODO: DM: 21/03/2026: handle error
	}
}

#endif // __linux__