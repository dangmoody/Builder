# Builder

by Dan Moody.

## What is Builder?

Builder is a build tool that lets you configure the compilation of your program by writing C++ code.

## Motivation

If you're a C++ programmer you've almost certainly had some trouble with your build system at one point or another.  As an example: CMake, while popular, requires you to learn a whole other language and has a ton of friction that comes with it.

It's easy to see why tools like CMake have appeal on the surface.  Using C++ compilers from just the command line for even small sized codebases is not feasible given the sheer number of arguments that they require.  This is why things like Visual Studio project configurations exist.  There's a whole host of alternative options though: programs like Ninja, build scripts written in shell/bash script, Makefiles, Python, the list goes on.  Unreal Engine uses C# for this for some reason.

Why don't we just configure our builds in the same language we write our programs in? It's so much more intuitive, there's a lot less friction, and you don't have to learn another language.  Enter Builder.

With Builder you can build your program from the same language you write your program with, given a single C++ source file containing some code that configures your build.  This is a much more intuitive way of configuring builds and it's a wonder no-one has come up with this sooner.

## Installation

1. Download the latest release.
2. Extract the contents of the archive to anywhere on your computer that you like.

## Usage

Instead of compiling your program like this:

```
clang -std=c++20 -g -o my-program.exe -DIS_AWESOME=1 game.cpp ...
```

Or suffering through the Visual Studio project settings or whatever you were doing before, you instead now do it like this:

```cpp
// build.cpp

#include <builder.h> // builder will automatically resolve this include for you

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	options->binary_name = "my-program"; // file extension is appended automatically for you
	options->binary_folder = "bin/win64";

	options->source_files.push_back( "src/*.cpp" ); // can also be individual files

	options->defines.push_back( "IS_AWESOME=1" );
}
```

And then at a command line, do this:

```
builder build.cpp
```

**Builder is not a compiler.**  Builder will just call Clang under the hood.  Builder just figures out what to tell Clang to do based on your build source file that you specify.

If you don't write `set_builder_options` then Builder can still build your program, it will just use the defaults:
* The program name will be the name of the source file you specified, except it will end with `.exe` instead of `.cpp`.
* The program will be put in the same folder as the source file.

### Configs

Configs are a totally optional part of building with Builder.  You don't have to use them if you don't want to.

```cpp
builder build.cpp --config=debug
```

The name of the config can be whatever you want it to be.  `BuilderOptions::config` will be set to the config that you pass in via the command line, which you can then use inside `set_builder_options` to configure your build by config.

See `BuilderOptions` inside `builder.h` for a full list of options, what they do, and how Builder uses them.

Builder also has other entry points:
* `on_pre_build( BuilderOptions* )` - This gets run just before your program actually gets compiled.
* `on_post_build( BuilderOptions* )` - This gets run just after your program actually gets compiled.
* `set_visual_studio_options( VisualStudioSolution* )` - Generates a Visual Studio solution (see below).

## Visual Studio

Builder supports generating a Visual Studio solution, but this time you must use the entry point `set_visual_studio_options( VisualStudioSolution* )`.

Example usage:

```cpp
// build.cpp

#include <builder.h> // builder will automatically resolve this include for you

BUILDER_CALLBACK void set_visual_studio_options( VisualStudioSolution* solution ) {
	solution->name = "my-program";
	solution->path = "../visual_studio";
	solution->platforms.push_back( "win64" );

	// project
	VisualStudioProject* project = add_visual_studio_project( solution );
	project->name = "my-program";
	project->source_files.push_back( "*.cpp" );

	// project configs
	VisualStudioConfig* configDebug = add_visual_studio_config( project );
	configDebug->name = "debug";
	configDebug->output_directory = "bin/debug";
	configDebug->options.source_files.push_back( "main.cpp" );
	configDebug->options.binary_name = "test";
	configDebug->options.binary_folder = "../bin/debug";
	configDebug->options.optimization_level = OPTIMIZATION_LEVEL_O0;
	configDebug->options.defines.push_back( "_DEBUG" );

	VisualStudioConfig* configRelease = add_visual_studio_config( project );
	configRelease->name = "release";
	configRelease->output_directory = "bin/release";
	configRelease->options.source_files.push_back( "main.cpp" );
	configRelease->options.binary_name = "test";
	configRelease->options.binary_folder = "../bin/release";
	configRelease->options.optimization_level = OPTIMIZATION_LEVEL_O3;
	configRelease->options.defines.push_back( "NDEBUG" );
}
```

All projects that get generated are Makefile projects, where the build commands pass the `--config` argument from Visual Studio.