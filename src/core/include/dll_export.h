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

/*
================================================================================================

	DLL Export

	Core is an header-only library and does not compile out to a DLL.  However, you may want to
	use Core in a project that does.  This means that if you want any program that's including
	and linking to your DLL program to also use code from Core then you will need to expose
	Core to the DLL.  This header lets you do that.

	If you're familiar with exporting functions to DLLs then you already know what's happening
	in this header, but if not here's what all the #defines mean:

	All you really need to know as the user is this: If you're using Core in a program that
	compiles out to a dynamic library then you want to #define both CORE_DLL and CORE_EXPORTS
	in your build system somewhere.

	CORE_API will either export the function to the DLL, or import the function from it
	depending on whether or not CORE_EXPORTS is #defined.

================================================================================================
*/

#ifdef CORE_DLL
	#ifdef CORE_EXPORTS
		#define CORE_API	__declspec( dllexport )
	#else
		#define CORE_API	__declspec( dllimport )
	#endif
#else
	#define CORE_API
#endif