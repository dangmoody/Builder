/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

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