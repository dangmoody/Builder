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

#include "linear_allocator.h"

#include "typecast.h"
#include "helpers.h"
#include "memory.h"
#include "math.h"
#include "debug.h"
#include "os.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

LinearAllocator *Mem_AllocatorCreate( const u64 reservedBytes ) {
	assert( reservedBytes );

	u32 pageSize = OS_GetVirtualMemoryPageSize();

	u64 actualReservedBytes = reservedBytes;

	if ( actualReservedBytes % pageSize != 0 ) {
		actualReservedBytes = AlignUp( actualReservedBytes, pageSize );

		// warning(
		// 	"LinearAllocator: specified reserved bytes (%llu) is not a multiple of the virtual memory page size (%u bytes).\n"
		// 	"The OS dictates that any virtual memory pages that get reserved will automatically be a multiple of %u, so the specified reserved bytes will be rounded up to %llu bytes.\n"
		// 	, reservedBytes, pageSize, pageSize, actualReservedBytes
		// );
	}

	// TODO: DM: 29/12/2025: alloc the whole allocator plus its entire arena in one virtual alloc call
	LinearAllocator *allocator = cast( LinearAllocator *, malloc( sizeof( LinearAllocator ) ) );
	memset( allocator, 0, sizeof( LinearAllocator ) );
	allocator->ptr = cast( u8 *, VirtualReserve( actualReservedBytes ) );
	allocator->reservedBytes = actualReservedBytes;
	allocator->virtualMemoryPageSize = pageSize;

	return allocator;
}

void Mem_AllocatorDestroy( LinearAllocator *allocator ) {
	assert( allocator );

	VirtualFree( allocator->ptr );

	free( allocator );
	allocator = NULL;
}

void* Mem_AllocatorAlloc( LinearAllocator *allocator, const u64 sizeBytes, const u32 alignment ) {
	assert( allocator );
	assert( sizeBytes );

	allocator->offset = AlignUp( allocator->offset, alignment );

	if ( allocator->offset >= allocator->comittedBytes || allocator->offset + sizeBytes > allocator->comittedBytes ) {
		u64 newComittedBytes = AlignUp( allocator->offset + sizeBytes, allocator->virtualMemoryPageSize );

		assert( newComittedBytes <= allocator->reservedBytes );

		void *result = VirtualCommit( allocator->ptr + allocator->comittedBytes, newComittedBytes - allocator->comittedBytes );
		assert( result );
		unused( result );

		allocator->comittedBytes = newComittedBytes;
	}

	u8 *ptr = allocator->ptr + allocator->offset;

	allocator->offset += sizeBytes;

	return ptr;
}

void Mem_AllocatorReset( LinearAllocator *allocator ) {
	allocator->offset = 0;
}

u64 Mem_AllocatorTell( LinearAllocator *allocator ) {
	return allocator->offset;
}

void Mem_AllocatorRewindTo( LinearAllocator *allocator, const u64 offset ) {
	allocator->offset = offset;
}

void Mem_AllocatorRewindBy( LinearAllocator *allocator, const u64 bytes ) {
	allocator->offset -= bytes;
}