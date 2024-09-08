/*
===========================================================================

Builder

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "src/core/array.h"
#include "src/core/string_helpers.h"

// If you override set_builder_options() you will need preface the function with the BUILDER_CALLBACK #define.
// This is because when Builder does its user config build stage it will search your code for the function set_builder_options() and BUILDER_DOING_USER_CONFIG_BUILD will be defined.
// This means that you need to have set_builder_options() exposed so that Builder can find the function and call it, hence it gets exported as a symbol in the binary.
// Then Builder will compile your program proper, so that function isn't needed anymore.
#ifdef BUILDER_DOING_USER_CONFIG_BUILD
#define BUILDER_CALLBACK	extern "C" __declspec( dllexport )
#else
#define BUILDER_CALLBACK	static
#endif

enum BinaryType {
	BINARY_TYPE_EXE	= 0,	// Build an executable (.exe on Windows)
	BINARY_TYPE_DLL,		// Build a dynamic linked library (.dll on Windows)
	BINARY_TYPE_LIB,		// Build a static library (.lib on Windows)
};

enum OptimizationLevel {
	OPTIMIZATION_LEVEL_O0	= 0,
	OPTIMIZATION_LEVEL_O1,
	OPTIMIZATION_LEVEL_O2,
	OPTIMIZATION_LEVEL_O3,
};

struct BuilderOptions {
	// The source files that you want to build.
	// Any files/paths you add to this will be made relative to the .cpp file you passed in via the command line.
	// Supports paths and wildcards.
	Array<const char*>	source_files;

	// Additional #defines to set for Clang.
	// Example: IS_AWESOME=1.
	Array<const char*>	defines;

	// Additional include paths to set for Clang.
	Array<const char*>	additional_includes;

	// Additional library paths to set for Clang.
	Array<const char*>	additional_lib_paths;

	// Additional libraries to set for Clang.
	Array<const char*>	additional_libs;

	// Additional warnings to tell Clang to ignore.
	// Uses the Clang syntax (E.G.: -Wno-newline-eof).
	Array<const char*>	ignore_warnings;

	// What kind of binary do you want to build?
	// Defaults to EXE.
	BinaryType			binary_type;

	// What level of optimization do you want in your binary?
	// Having optimization disabled helps when debugging, but you definitely want optimizations enabled when you build your retail/shipping binary.
	OptimizationLevel	optimization_level;

	// Do you want to remove symbols from your binary?
	// You will probably want symbols for debugging, but then not have these in your retail/shipping binary.
	bool				remove_symbols;

	// Do you want to remove the file extension from the name of the binary?
	bool				remove_file_extension;

	// The name of the config that you want to build with.
	// You need to set this via the command line argument "--config=name" where "name" is the name of your config.
	// If you do not set this then it will just be null.
	const char*			config;

	// The folder you want the binary to be put into.
	// If the folder does not exist, then Builder will create it for you.
	// This will be relative to the source file you are building.
	const char*			binary_folder;

	// The name that the built binary is going to have.
	// This will be placed inside binary_folder, if you set that.
	const char*			binary_name;
};

struct VisualStudioProject {
	// Visual studio project name
	const char*			name;

	// These are the source files that will be included in the "Source Files" filter in the project.
	// This is a separate list to the build options as you likely want the superset of
	// all files in your Solution, but may conditionally exclude a subset of files based on config/target etc
	Array<const char*>	source_files;

	Array<const char*>	include_paths;

	Array<const char*>	lib_paths;

	Array<const char*>	definitions;

	const char*			out_path;

	// Location of the build script
	const char*			build_script_path;
};

struct VisualStudioSolution {
	// The folder where the solution (and it's projects) are going to live.
	// This is relative to the source file that calls set_visual_studio_options().
	const char*					path;

	// The name of the solution.
	// For the sake of simplicity we keep the name of the Solution in Visual Studio and the Solution's filename the same.	TODO: make it actually do that
	const char*					name;

	// The names of the configs that VS should allow to select from.
	// There are no options associated with these configs here as those settings are 
	// defined in the build script's logic
	Array<const char*>			configs;

	Array<const char*>			platforms;

	Array<VisualStudioProject>	projects;
};