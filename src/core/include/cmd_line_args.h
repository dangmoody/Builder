/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "core_types.h"

struct CommandLineArgs {
	char**					data;
	s32						count;
};

extern CommandLineArgs		g_cmd_line_args;

void						set_command_line_args( int argc, char** argv );
CommandLineArgs				get_command_line_args( void );