/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "cmd_line_args.h"

/*
================================================================================================

	command line args

================================================================================================
*/

CommandLineArgs g_cmd_line_args = {};

void set_command_line_args( int argc, char** argv ) {
	g_cmd_line_args.count = argc;
	g_cmd_line_args.data = argv;
}

CommandLineArgs get_command_line_args() {
	return g_cmd_line_args;
}