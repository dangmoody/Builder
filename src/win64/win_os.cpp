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

#ifdef _WIN32

#include "../os.h"

#include <Windows.h>

/*
================================================================================================

	OS helpers

	TODO: DM: 29/12/2025: calling GetSystemInfo() every time like this is obviously bad
	but having core cache state at startup caused problems before
	so I'm gun-shy about doing that again

================================================================================================
*/

u32 OS_GetVirtualMemoryPageSize() {
	SYSTEM_INFO sysInfo = {};
	GetSystemInfo( &sysInfo );
	return sysInfo.dwPageSize;
}

u32 OS_GetNumCpuCores() {
	SYSTEM_INFO sysInfo = {};
	GetSystemInfo( &sysInfo );
	return sysInfo.dwNumberOfProcessors;
}

#endif // _WIN32
