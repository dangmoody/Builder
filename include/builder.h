/*
===========================================================================

Builder

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#pragma once

#include <vector>
#include <string.h>
#include <string>

#ifdef __linux__
#include <cstring> // ctring is not a part of std string on linux and needs a manual include
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif // __linux__

// If you override SetBuilderOptions() you will need preface the function with the BUILDER_CALLBACK #define.
// This is because when Builder does its user config build stage it will search your code for the function SetBuilderOptions() and BUILDER_DOING_USER_CONFIG_BUILD will be defined.
// This means that you need to have SetBuilderOptions() exposed so that Builder can find the function and call it, hence it gets exported as a symbol in the binary.
// Then Builder will compile your program proper, so that function isn't needed anymore.
#if defined( _WIN32 )
	#ifdef BUILDER_DOING_USER_CONFIG_BUILD
		#define BUILDER_CALLBACK	extern "C" __declspec( dllexport )
	#else
		#define BUILDER_CALLBACK	static
	#endif
#elif defined( __linux__ )
	#ifdef BUILDER_DOING_USER_CONFIG_BUILD
		#define BUILDER_CALLBACK	extern "C" __attribute__( ( visibility( "default" ) ) )
	#else
		#define BUILDER_CALLBACK
	#endif
#endif

enum LanguageVersion {
	LANGUAGE_VERSION_UNSET	= 0,
	LANGUAGE_VERSION_C89,
	LANGUAGE_VERSION_C99,
	LANGUAGE_VERSION_C11,
	LANGUAGE_VERSION_C17,
	LANGUAGE_VERSION_C23,
	LANGUAGE_VERSION_CPP11,
	LANGUAGE_VERSION_CPP14,
	LANGUAGE_VERSION_CPP17,
	LANGUAGE_VERSION_CPP20,
	LANGUAGE_VERSION_CPP23,
};

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
	// The other BuildConfigs that this build needs to have happened first.
	std::vector<BuildConfig>	dependsOn;

	// The source files that you want to build.
	// Any files/paths you add to this will be made relative to the .cpp file you passed in via the command line.
	// Also supports wildcards.
	std::vector<std::string>	sourceFiles;

	// Additional #defines to set for Clang.
	// Example: IS_AWESOME=1.
	std::vector<std::string>	defines;

	// Additional include paths to set for Clang.
	std::vector<std::string>	additionalIncludes;

	// Additional library paths to set for Clang.
	std::vector<std::string>	additionalLibPaths;

	// Additional libraries to set for Clang.
	std::vector<std::string>	additionalLibs;

	// The warning/diagnostic groups that you want to enable.
	// Allowed values: -Weverything, -Wall, -Wextra, -Wpedantic.
	std::vector<std::string>	warningLevels;

	// Additional warnings to tell the compiler to ignore.
	// For Clang and GCC this array will be filled with things like "-Wno-newline-eof".
	// For MSVC you'd use /wd.
	std::vector<std::string>	ignoreWarnings;

	// Anything else that you want to pass to the compiler that there isn't already an existing option for.
	// These will get added to the end of all the other compiler arguments.
	std::vector<std::string>	additionalCompilerArguments;

	// The name that the built binary is going to have.
	// It's not necessary to include the file extension unless 'removeFileExtension' is false.
	// This will be placed inside binaryFolder, if you set that.
	std::string					binaryName;

	// The folder you want the binary to be put into.
	// If the folder does not exist, then Builder will create it for you.
	// This path is relative to the file you pass into Builder.
	std::string					binaryFolder;

	// The name of the config.
	// If you have multiple BuildConfigs (E.G. one for debug and one for release) you need to set this for each config.
	// Then when you build, you'll tell Builder which config to use by using the command line argument:
	//
	//	builder.exe --config=name
	//
	// Where 'name' is whatever you set this to.
	std::string					name;

	// What version of C or C++ do you want to build with?
	// For Clang: This sets the -std argument.
	// For MSVC: This sets the /std argument.
	LanguageVersion				languageVersion;

	// What kind of binary do you want to build?
	// Defaults to EXE.
	BinaryType					binaryType;

	// What level of optimization do you want in your binary?
	OptimizationLevel			optimizationLevel;

	// Do you want to remove symbols from your binary?
	bool						removeSymbols;

	// Do you want to remove the file extension from the name of the binary?
	bool						removeFileExtension;

	// Do you want warnings to count as errors?
	bool						warningsAsErrors;
};

struct VisualStudioConfig {
	// The name of the config as it appears in Visual Studio.
	// This is different from BuildConfig::name because this one doesn't have to be unique.
	// You can have lots of VisualStudioConfigs with a name of "Debug", for instance.
	std::string					name;

	BuildConfig					options;

	std::vector<std::string>	debuggerArguments;
};

struct VisualStudioProject {
	// Configs that this project knows about.
	// For example: Debug, Profiling, Shipping, and so on.
	// You must define at least one of these to make Visual Studio happy.
	std::vector<VisualStudioConfig>	configs;

	// All the files that are in these folders (based on 'fileExtensions') will be included in your project.
	// This is a separate list to the build options as you likely want the superset of all files in your Solution, but may conditionally exclude a subset of files based on config/target etc.
	// The folders you include here are relative to your build script.
	// This list must NOT contain any search filters.
	std::vector<std::string>		codeFolders;

	// All files that have any of these extensions (based on 'codeFolders') will be included in your project.
	// These must NOT start with a dot.  Only the extension is required (Examples: cpp, h, inl).
	// If you do not fill this in and leave it empty, then the following default file extensions will be used: c, cpp, cc, cxx, h, hpp, inl
	std::vector<std::string>		fileExtensions;

	// The name of the project as it shows in Visual Studio.
	std::string						name;
};

struct VisualStudioSolution {
	// All the projects in the Solution.
	std::vector<VisualStudioProject>	projects;

	// All the target platforms that this Solution supports.
	std::vector<std::string>			platforms;

	// The name of the Solution as it appears in Visual Studio.
	// For the sake of simplicity we keep the name of the Solution in Visual Studio and the Solution's filename the same.
	std::string							name;

	// The folder where the solution (and it's projects) are going to live.
	// If you don't set this then the solution is generated in the same path as the build file.
	// The path is relative to the source file that you specify at the command line.
	std::string							path;
};

struct BuilderOptions {
	// The path to the compiler that you want to build with.
	// If you want to use MSVC then you can just set this to "cl.exe" or "cl" and set 'compilerVersion' and Builder will figure it out for you.
	// If you leave this unset then Builder will use the portable install of Clang that it came with.
	std::string					compilerPath;

	// What version of your compiler are you using?
	// When the compiler version you specify in SetBuilderOptions() doesn't match the version we get when we run your compiler then this will generate a warning.
	// This is useful when working in a team and you want to make sure that people use the same compiler version.
	// For Clang this would be something like "20.1.5".
	// For MSVC this would be something like "14.44.35207".
	std::string					compilerVersion;

	// All the possible configs that you could build with.
	// Pass the one you actually want to build with via the --config= command line argument.
	// If all you're doing is generating Visual Studio Solutions then you don't need to fill this out.
	std::vector<BuildConfig>	configs;

	// If you don't use Visual Studio then ignore this.
	VisualStudioSolution		solution;

	// Set this to true if you want Builder to force-rebuild your program.
	// All binaries and intermediate files will get rebuilt.
	// This is really only useful to those who are either using an editor + command line workflow, or just hate incremental builds.
	bool						forceRebuild;

	// Do you want to generate a Visual Studio solution?
	// If this is set to true, then a code build will NOT happen.
	// If you don't use Visual Studio then ignore this.
	bool						generateSolution;

	// Do you want to generate a compilation_commands.json for Clang tooling?
	// If true, the file will be generated IF the build is successful.
	bool						generateCompilationDatabase;
};

static void AddBuildConfigUnique( BuildConfig *config, std::vector<BuildConfig> &outConfigs );

static void AddBuildConfig( BuilderOptions *options, BuildConfig *config ) {
	for ( size_t i = 0; i < config->dependsOn.size(); i++ ) {
		AddBuildConfig( options, &config->dependsOn[i] );
	}

	AddBuildConfigUnique( config, options->configs );
}


//
// The following is not for users.
// Don't use or touch any of this unless you're either a Builder developer or you know exactly what you're doing.
//

static unsigned int BuilderHashSDBM( void *data, const unsigned int seed, const size_t length ) {
	unsigned char *c = (unsigned char *) data;

	unsigned int hash = seed;

	for ( size_t i = 0; i < length; i++ ) {
		hash = ( c[i] ) + ( hash << 6 ) + ( hash << 16 ) - hash;
	}

	return hash;
}

static unsigned int BuilderHashCString( const unsigned int seed, const char *str, const size_t length ) {
	return BuilderHashSDBM( (void *) str, seed, length );
};

static unsigned int BuilderHashCStringArray( const unsigned int seed, const std::vector<const char *> &strings ) {
	unsigned int hash = seed;

	for ( size_t stringIndex = 0; stringIndex < strings.size(); stringIndex++ ) {
		const char *str = strings[stringIndex];

		hash = BuilderHashCString( hash, str, strlen( str ) );
	}

	return hash;
};

static unsigned int BuilderHashStringArray( const unsigned int seed, const std::vector<std::string> &strings ) {
	unsigned int hash = seed;

	for ( size_t stringIndex = 0; stringIndex < strings.size(); stringIndex++ ) {
		const std::string &str = strings[stringIndex];

		hash = BuilderHashCString( hash, str.c_str(), str.size() );
	}

	return hash;
};

static unsigned int BuilderGetConfigHash( BuildConfig *config, const unsigned int seed ) {
	unsigned int hash = seed;

	for ( size_t dependencyIndex = 0; dependencyIndex < config->dependsOn.size(); dependencyIndex++ ) {
		hash = BuilderGetConfigHash( &config->dependsOn[dependencyIndex], hash );
	}

	hash = BuilderHashStringArray( hash, config->sourceFiles );
	hash = BuilderHashStringArray( hash, config->defines );
	hash = BuilderHashStringArray( hash, config->additionalIncludes );
	hash = BuilderHashStringArray( hash, config->additionalLibPaths );
	hash = BuilderHashStringArray( hash, config->additionalLibs );
	hash = BuilderHashStringArray( hash, config->ignoreWarnings );

	hash = BuilderHashCString( hash, config->binaryName.c_str(), config->binaryName.length() );
	hash = BuilderHashCString( hash, config->binaryFolder.c_str(), config->binaryFolder.length() );
	hash = BuilderHashCString( hash, config->name.c_str(), config->name.length() );

	hash = BuilderHashSDBM( &config->binaryType, hash, sizeof( BinaryType ) );
	hash = BuilderHashSDBM( &config->optimizationLevel, hash, sizeof( OptimizationLevel ) );

	hash = BuilderHashSDBM( &config->removeSymbols, hash, sizeof( bool ) );
	hash = BuilderHashSDBM( &config->removeFileExtension, hash, sizeof( bool ) );
	hash = BuilderHashSDBM( &config->warningsAsErrors, hash, sizeof( bool ) );

	return hash;
}

static bool BuildConfigEquals( BuildConfig *configA, BuildConfig *configB ) {
	unsigned int hashA = BuilderGetConfigHash( configA, 0 );
	unsigned int hashB = BuilderGetConfigHash( configB, 0 );

	return hashA == hashB;
}

static void AddBuildConfigUnique( BuildConfig *config, std::vector<BuildConfig> &outConfigs ) {
	bool duplicate = false;
	for ( size_t configIndex = 0; configIndex < outConfigs.size(); configIndex++ ) {
		if ( BuildConfigEquals( &outConfigs[configIndex], config ) ) {
			duplicate = true;
			break;
		}
	}

	if ( !duplicate ) {
		outConfigs.push_back( *config );
	}
}

#ifdef __linux__
#pragma clang diagnostic pop
#endif // __linux__
