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

	Defer macro for C++ like what Go and Jai have
	Taken from https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/

================================================================================================
*/

template <typename CodeToRun>
struct RunOnExitScope {
	CodeToRun	code_to_run;
	RunOnExitScope( CodeToRun the_code_to_run )
		: code_to_run( the_code_to_run )
	{
	}

	~RunOnExitScope() {
		code_to_run();
	}
};

template <typename CodeToRun>
RunOnExitScope<CodeToRun> defer_func( CodeToRun code_to_run ) {
	return RunOnExitScope<CodeToRun>( code_to_run );
}

#define DEFER_1( x, y )	x ## y
#define DEFER_2( x, y )	DEFER_1( x, y )
#define DEFER_3( x )	DEFER_2( x, __COUNTER__ )

#define defer( code )	auto DEFER_3( _defer_ ) = defer_func( [&]() { code; } )