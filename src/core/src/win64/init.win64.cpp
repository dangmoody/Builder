/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef _WIN64

#include "../core_local.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>

void core_init_platform() {
	HANDLE process = GetCurrentProcess();

	SymInitialize( process, NULL, TRUE );
}

void core_shutdown_platform() {
	HANDLE process = GetCurrentProcess();

	SymCleanup( process );
}

#endif // _WIN64