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
#include "memory.h"
#include "math.h"
#include "debug.h"
#include "os.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

linearAllocator_t *Mem_AllocatorCreate( const u64 reservedBytes ) {
	Assert( reservedBytes );

	u32 pageSize = OS_GetVirtualMemoryPageSize();

	u64 actualReservedBytes = reservedBytes;

	if ( actualReservedBytes % pageSize != 0 ) {
		actualReservedBytes = AlignUp( actualReservedBytes, pageSize );

		// Warning(
		// 	"linearAllocator_t: specified reserved bytes (%llu) is not a multiple of the virtual memory page size (%u bytes).\n"
		// 	"The OS dictates that any virtual memory pages that get reserved will automatically be a multiple of %u, so the specified reserved bytes will be rounded up to %llu bytes.\n"
		// 	, reservedBytes, pageSize, pageSize, actualReservedBytes
		// );
	}

	// TODO: DM: 29/12/2025: alloc the whole allocator plus its entire arena in one virtual alloc call
	linearAllocator_t *allocator = Cast( linearAllocator_t *, malloc( sizeof( linearAllocator_t ) ) );
	memset( allocator, 0, sizeof( linearAllocator_t ) );
	allocator->ptr = Cast( u8 *, Mem_VirtualReserve( actualReservedBytes ) );
	allocator->reservedBytes = actualReservedBytes;
	allocator->virtualMemoryPageSize = pageSize;

	return allocator;
}

void Mem_AllocatorDestroy( linearAllocator_t *allocator ) {
	Assert( allocator );

	Mem_VirtualFree( allocator->ptr );

	free( allocator );
	allocator = NULL;
}

void* Mem_AllocatorAlloc( linearAllocator_t *allocator, const u64 sizeBytes, const u32 alignment ) {
	Assert( allocator );
	Assert( sizeBytes );

	allocator->offset = AlignUp( allocator->offset, alignment );

	if ( allocator->offset >= allocator->comittedBytes || allocator->offset + sizeBytes > allocator->comittedBytes ) {
		u64 newComittedBytes = AlignUp( allocator->offset + sizeBytes, allocator->virtualMemoryPageSize );

		Assert( newComittedBytes <= allocator->reservedBytes );

		void *result = Mem_VirtualCommit( allocator->ptr + allocator->comittedBytes, newComittedBytes - allocator->comittedBytes );
		Assert( result );
		UNUSED( result );

		allocator->comittedBytes = newComittedBytes;
	}

	u8 *ptr = allocator->ptr + allocator->offset;

	allocator->offset += sizeBytes;

	return ptr;
}

void Mem_AllocatorReset( linearAllocator_t *allocator ) {
	allocator->offset = 0;
}

u64 Mem_AllocatorTell( linearAllocator_t *allocator ) {
	return allocator->offset;
}

void Mem_AllocatorRewindTo( linearAllocator_t *allocator, const u64 offset ) {
	allocator->offset = offset;
}

void Mem_AllocatorRewindBy( linearAllocator_t *allocator, const u64 bytes ) {
	allocator->offset -= bytes;
}