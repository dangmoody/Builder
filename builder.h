/*
===========================================================================

Builder

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include <vector>
#include <string>

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
	BINARY_TYPE_EXE	= 0,			// .exe on Windows
	BINARY_TYPE_DYNAMIC_LIBRARY,	// .dll on Windows
	BINARY_TYPE_STATIC_LIBRARY,		// .lib on Windows
};

enum OptimizationLevel {
	OPTIMIZATION_LEVEL_O0	= 0,
	OPTIMIZATION_LEVEL_O1,
	OPTIMIZATION_LEVEL_O2,
	OPTIMIZATION_LEVEL_O3,
};

struct BuildConfig {
	std::vector<BuildConfig>	depends_on;

	// The source files that you want to build.
	// Any files/paths you add to this will be made relative to the .cpp file you passed in via the command line.
	// Supports paths and wildcards.
	std::vector<const char*>	source_files;

	// Additional #defines to set for Clang.
	// Example: IS_AWESOME=1.
	std::vector<const char*>	defines;

	// Additional include paths to set for Clang.
	std::vector<const char*>	additional_includes;

	// Additional library paths to set for Clang.
	std::vector<const char*>	additional_lib_paths;

	// Additional libraries to set for Clang.
	std::vector<const char*>	additional_libs;

	// Additional warnings to tell Clang to ignore.
	// Uses the Clang syntax (E.G.: -Wno-newline-eof).
	std::vector<const char*>	ignore_warnings;

	// The name that the built binary is going to have.
	// This will be placed inside binary_folder, if you set that.
	std::string					binary_name;

	// The folder you want the binary to be put into.
	// If the folder does not exist, then Builder will create it for you.
	// This path is relative to the source file you are building.
	std::string					binary_folder;

	// The name of the config.
	// If you have multiple BuildConfigs (E.G. one for debug and one for release) you need to set this for each config.
	// Then when you build, you'll tell Builder which config to use by using the command line argument:
	//
	//	builder.exe --config=name
	//
	// Where 'name' is whatever you set this to.
	std::string					name;

	// What kind of binary do you want to build?
	// Defaults to EXE.
	BinaryType					binary_type;

	// What level of optimization do you want in your binary?
	OptimizationLevel			optimization_level;

	// Do you want to remove symbols from your binary?
	bool						remove_symbols;

	// Do you want to remove the file extension from the name of the binary?
	bool						remove_file_extension;

	// Do you want warnings to count as errors?
	bool						warnings_as_errors;
};

struct VisualStudioConfig {
	const char*					name;

	BuildConfig					options;

	std::vector<const char*>	debugger_arguments;

	// TODO(DM): 06/10/2024: this shouldnt exist
	// we should figure this out by taking the binary_folder from the build options and making it relative to the solution instead
	const char*					output_directory;
};

struct VisualStudioProject {
	// Configs that this project knows about.
	// For example: Debug, Profiling, Shipping, and so on.
	// You must define at least one of these to make Visual Studio happy.
	std::vector<VisualStudioConfig>	configs;

	// These are the source files that will be included in the "Source Files" filter in the project.
	// This is a separate list to the build options as you likely want the superset of
	// all files in your Solution, but may conditionally exclude a subset of files based on config/target etc
	std::vector<const char*>		source_files;

	// Visual Studio project name.
	const char*						name;
};

struct VisualStudioSolution {
	std::vector<VisualStudioProject>	projects;

	std::vector<const char*>			platforms;

	// The name of the solution.
	// For the sake of simplicity we keep the name of the Solution in Visual Studio and the Solution's filename the same.	TODO: make it actually do that
	const char*							name;

	// The folder where the solution (and it's projects) are going to live.
	// This is relative to the source file that fills in the entry point set_visual_studio_options.
	const char*							path;
};

struct BuilderOptions {
	// All the possible configs that you could build with.
	// Pass the one you actually want to build with via the --config= command line argument.
	// If you want use Visual Studio only, then don't fill this out.
	std::vector<BuildConfig>	configs;

	// If you don't use Visual Studio then ignore this.
	VisualStudioSolution		solution;

	// Do you want to generate a Visual Studio solution?
	// If this is set to true, then a code build will NOT happen.
	// If you don't use Visual Studio then ignore this.
	bool						generate_solution;
};

static void add_build_config_unique( BuildConfig* config, std::vector<BuildConfig>& outConfigs ) {
	bool duplicate = false;
	for ( size_t i = 0; i < outConfigs.size(); i++ ) {
		if ( memcmp( &outConfigs[i], config, sizeof( BuildConfig ) ) == 0 ) {
			duplicate = true;
			break;
		}
	}

	if ( !duplicate ) {
		outConfigs.push_back( *config );
	}
}

static void add_build_config( BuilderOptions* options, BuildConfig* config ) {
	for ( size_t i = 0; i < config->depends_on.size(); i++ ) {
		add_build_config( options, &config->depends_on[i] );
	}

	add_build_config_unique( config, options->configs );
}