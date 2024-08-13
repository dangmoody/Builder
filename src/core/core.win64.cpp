/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef _WIN64

#include "paths.h"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>

/*
================================================================================================

	Core init

================================================================================================
*/

//void core_init_platform();
//void core_shutdown_platform();

static void core_init_platform() {
	HANDLE process = GetCurrentProcess();

	SymInitialize( process, NULL, TRUE );

	paths_init();
}

static void core_shutdown_platform() {
	paths_shutdown();

	HANDLE process = GetCurrentProcess();

	SymCleanup( process );
}

#include "timer.win64.cpp"
#include "date_and_time.win64.cpp"
#include "debug.win64.cpp"
#include "file.win64.cpp"
#include "library.win64.cpp"
#include "paths.win64.cpp"

#endif // _WIN64