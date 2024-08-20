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

// Returns the absolute path of where the current program is running from.
const char*	paths_get_app_path();

// Returns the absolute path of 'file'.
const char*	paths_get_absolute_path( const char* file );

// Given a file path that also includes a filename, will remove the filename part, leaving just the path.
const char*	paths_remove_file_from_path( const char* path );

// Given a file path that also includes a filename, will remove the path part, leaving just the filename.
const char*	paths_remove_path_from_file( const char* path );

// Returns the name of a file without its file extension, if there is one.
const char*	paths_remove_file_extension( const char* filename );