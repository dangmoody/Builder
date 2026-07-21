/*
===========================================================================

Builder

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

#include "../library.h"

#include "../debug.h"

#include <dlfcn.h>
#include <errno.h>
#include <string.h>

/*
================================================================================================

	library_t

================================================================================================
*/

library_t Library_Load( const char *name ) {
	return library_t {
		.ptr = dlopen( name, RTLD_LAZY ),
	};
}

bool8 Library_Unload( library_t *library ) {
	Assert( library );
	Assert( library->ptr );

	if ( dlclose( library->ptr ) != 0 ) {
		int err = errno;
		Error( "Failed to close library handle: %s\n", strerror( err ) );	// TODO: DM: 20/03/2026: I think we want to get rid of this
		return false;
	}

	library->ptr = NULL;

	return true;
}

void *Library_GetSymbol( const library_t library, const char *symbolName ) {
	Assert( library.ptr );
	Assert( symbolName );

	return dlsym( library.ptr, symbolName );
}

#endif // __linux__
