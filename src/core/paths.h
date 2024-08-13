/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

struct Paths;

void		paths_init();
void		paths_shutdown();

const char* paths_get_app_path();