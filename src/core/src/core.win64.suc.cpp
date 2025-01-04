/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#ifdef CORE_SUC

#ifdef _WIN64

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>

#include "win64/init.win64.cpp"
#include "win64/timer.win64.cpp"
#include "win64/date_and_time.win64.cpp"
#include "win64/debug.win64.cpp"
#include "win64/file.win64.cpp"
#include "win64/library.win64.cpp"
#include "win64/paths.win64.cpp"

#endif // _WIN64

#endif // CORE_SUC