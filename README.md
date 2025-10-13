# Builder

by Dan Moody.


## What is Builder?

Builder is a build tool that lets you configure the compilation of your C++ program by writing C++ code.

**BUILDER IS NOT A COMPILER.**  Builder just turns some C++ code into compiler arguments and then calls the compiler to build your code

Builder supports Clang, MSVC, and GCC.


## Installation

1. Download the latest release.
2. Extract the archive somewhere.
3. **Optional:** Add Builder to your PATH.


## Usage

Builds are configured via a source file which you pass as an argument when calling Builder.

```cpp
// build.cpp

#include <builder.h> // Builder will automatically resolve this include for you.

// This is the entry point that Builder searches for.
BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_name = "my-awesome-program",
		.binary_folder = "bin/win64",
		.source_files = { "src/**/*.cpp" },
		.defines = { "IS_AWESOME=1" },
	};

	add_build_config( options, &config );
}
```

And then at a command line, do this:

```
builder build.cpp
```

Builder will now build your program.

If you don't write `set_builder_options` then Builder can still build your program, it will just use the defaults:
* Builder will use the portable install of Clang that it comes with as the compiler.
* The program name will be the name of the source file you specified, except it will end with `.exe` (on Windows, on Linux there won't be a file extension for executables) instead of `.cpp`.
* The program will be put in the same folder as the source file.

Run `builder -h` or `builder --help` for help in the command line.


### Configs

Builder supports building different "configs" in the way you may know them from Visual Studio.  A config is name that's applied to a series of build settings.

To specify a config in Builder you have to do two things:

1. When writing your `BuildConfig`, set the `name` member to the name of your config.
2. When you run Builder, pass in the command line argument `--config=<config name here>`.

Example usage:

```cpp
// build.cpp

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig debug = {
		.name = "debug",	// If you wanted to use this config, you'd pass --config=debug in the command line.
		.binary_name = "kenneth",
		.binary_folder = "bin/debug",
		.remove_symbols = false,
		.optimization_level = OPTIMIZATION_LEVEL_O0,
	};

	BuildConfig release = {
		.name = "release",	// If you wanted to use this config, you'd pass --config=release in the command line.
		.binary_name = "kenneth",
		.binary_folder = "bin/release",
		.remove_symbols = true,
		.optimization_level = OPTIMIZATION_LEVEL_O3,
	};

	add_build_config( options, &debug );
	add_build_config( options, &release );
}
```

And then at the command line add the `--config=` arg:

```
builder build.cpp --config=debug
```

The name of your config in code and the name of the config you pass via the command line MUST match exactly (case sensitive).

If you only have the one config then you don't need to pass `--config=` at the command line.  Builder will know to build the only config that's there.

See the `BuildConfig` struct inside `builder.h` for a full list of all the things that you can configure in your build.

When the build source file is getting compiled, Builder will a custom `#define` called `BUILDER_DOING_USER_CONFIG_BUILD`.  This can be useful if you need to seperate your `build.cpp` file from the other code files.

Builder also has other entry points:
* `on_pre_build()` - This gets run just before your program gets compiled.
* `on_post_build()` - This gets run just after your program gets compiled.


### Changing compilers

By default, Builder comes with it's own portable install of Clang and, by default, Builder will use that to compile with (if no compiler is set manually).  However, you absolutely don't have to use this as your compiler if you don't want to.

To override this, and to make Builder use whatever compiler you want (so long as it's either Clang, MSVC, or GCC) then do the following inside `set_builder_options`:

```cpp
options->compiler_path = "C:/Program Files/mingw64/bin/gcc";
```

You can also tell Builder that you want to use a specific compiler version like so:

```cpp
options->compiler_version = "15.1.0";
```

If Builder detects that your compiler version doesn't match the one specified it will throw a warning before trying to build your program.  This can be useful (for example) for teams who need to make sure they're using the same toolchain.


#### MSVC

Because of the completely ridiculous way Microsoft has structured the folder layout of all its tools, we recommend just setting the compiler path to `cl` and then setting the compiler version that you want to use and let Builder figure out everything for you.

You don't have to do this, and you can totally use a hard-coded path to `cl` if you want to, but Builder does support automatic resolution of MSVC and the Windows SDK files on its own if you pass just `cl` as the compiler path.


## Visual Studio

Builder also supports generating Visual Studio solutions.  You still fill out your `BuildConfig`s like before, but you also need to do two additional things:

1. Set `BuilderOptions::generate_solution` to `true`.
2. Fill out `BuilderOptions::solution`.

Code example:

```cpp
// build.cpp

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig debug = {
		.name = "debug",
		.source_files = { "src/*.cpp" },
		.binary_name = "test",
		.binary_folder = "bin/debug",
		.optimization_level = OPTIMIZATION_LEVEL_O0,
		.defines = { "_DEBUG" },
	};

	BuildConfig release = {
		.name = "release",
		.source_files = { "src/*.cpp" },
		.binary_name = "test",
		.binary_folder = "bin/release",
		.optimization_level = OPTIMIZATION_LEVEL_O3,
		.defines = { "NDEBUG" },
	};

	// If you know you're only building with Visual Studio, then you could optionally comment out these two lines.
	add_build_config( options, &debug );
	add_build_config( options, &release );

	// When this bool is true, Builder will always generate a Visual Studio solution, and it won't do a build.
	// So if, suddenly, you decide you want to do a build without using Visual Studio, just set this to false and then pass this file to Builder.
	// Alternatively, you could move this code into a separate .cpp file and pass that file to Builder instead when wishing to re-generate your solution.
	options->generate_solution = true;

	options->solution.name = "test-sln";
	options->solution.path = "visual_studio";
	options->solution.platforms = { "x64" };
	options->solution.projects = {
		{
			.name = "test-project",
			.code_folders = { "src" },
			.file_extensions = { "cpp", "h", "inl" },
			.configs = {
				{ "debug",   debug,   { /* debugger arguments */ } },
				{ "release", release, { /* debugger arguments */ } },
			}
		}
	};
}
```

Every project that Builder generates is a Makefile project that calls Builder.  Therefore, changes that you make to any of the project properties in Visual Studio will not actually do anything (this is a Visual Studio problem).  If you want to change your project properties you must re-generate your Visual Studio solution.


## Motivation

If you're a C++ programmer you've almost certainly had some trouble with your build system at one point or another.  As an example: CMake, while popular, requires you to learn a whole other language and has a ton of friction that comes with it.

It's easy to see why tools like CMake have appeal on the surface.  Using C++ compilers from just the command line for even small sized codebases is not feasible given the sheer number of arguments that they require.  This is why things like Visual Studio project configurations exist.  There's a whole host of alternative options though: programs like Ninja, build scripts written in shell/bash script, Makefiles, Python, the list goes on.  Unreal Engine uses C# for this for some reason.

Why don't we just configure our builds in the same language we write our programs in? It's so much more intuitive, there's a lot less friction, and you don't have to learn another language.  Enter Builder.

With Builder you can build your program from the same language you write your program with, given a single C++ source file containing some code that configures your build.  This is a much more intuitive way of programmers configuring builds.


## Contributing

Yes!

Please see [Contributing.md](doc/Contributing.md).


## Credits

Builder would not have been possible without the following people who deserve, at the very least, a special thanks:

* Dale Green
* Yann Richeux
* Tom Whitcombe (Visual Studio project generation)
* Mike Young (Linux port)