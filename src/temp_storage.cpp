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

#include "temp_storage.h"

#include "linear_allocator.h"
#include "typecast.h"
#include "debug.h"

/*
================================================================================================

	temp storage

================================================================================================
*/

// DM!!! this cant live here!
#ifdef _WIN32
#define THREAD_LOCAL __declspec( thread )
#else
#define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL linearAllocator_t *gTempStorage = NULL;

linearAllocator_t *Mem_GetTempStorage() {
	return gTempStorage;
}

void Mem_InitTempStorage( const u64 sizeBytes ) {
	gTempStorage = Mem_AllocatorCreate( sizeBytes );
}

void Mem_ShutdownTempStorage() {
	Mem_AllocatorDestroy( gTempStorage );
	gTempStorage = NULL;
}

void* Mem_TempAlloc( const u64 sizeBytes, const u32 alignment ) {
	return Mem_AllocatorAlloc( gTempStorage, sizeBytes, alignment );
}

void Mem_ResetTempStorage() {
	Mem_AllocatorReset( gTempStorage );
}

u64 Mem_TempTell() {
	return Mem_AllocatorTell( gTempStorage );
}

void Mem_TempRewindTo( const u64 pos ) {
	Mem_AllocatorRewindTo( gTempStorage, pos );
}
