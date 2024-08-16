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

//const char* paths_get_absolute_path( const char* file );
//const char* paths_remove_file_from_path( const char* path );
//const char* paths_remove_path_from_file( const char* path );