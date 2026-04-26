# Builder

by Dan Moody.

Configure C/C++ builds by writing C++ instead of learning a separate build language.

**BUILDER IS NOT A COMPILER.** - it turns your build config into compiler arguments and calls the compiler.

On windows it supports Clang, MSVC, and GCC.

On Linux it supports Clang and GCC.

## Installation

1. Download the latest release.
2. Extract the archive somewhere.
3. **Optional:** Add Builder to your `PATH`.

## Quick Start

When pointed at a **single** C/C++ source file, Builder will compile it:

```
builder main.cpp
```

By default, Builder uses its bundled Clang, outputs the binary in the same folder, and names it after the source file. No config required.

### Configuring your build

For multiple source files, and extra options, add `SetBuilderOptions` to your source file.

```cpp
#include <builder.h> // Builder will automatically resolve this include for you.

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	BuildConfig config = {
		.binaryName   = "my-program",
		.binaryFolder = "bin",
		.sourceFiles  = { "src/**/*.cpp" },
		.defines      = { "MY_DEFINE=1" },
	};

	AddBuildConfig( options, &config );
}

int main() { ... }
```

Any problems pass `-v` or `--verbose` when calling builder.

### Separating build code from program code

`BUILDER_DOING_USER_CONFIG_BUILD` is defined when Builder is compiling your source file into a config DLL. Use it to keep build code out of your program, and/or put `SetBuilderOptions` in a dedicated build file entirely:

```cpp
#if BUILDER_DOING_USER_CONFIG_BUILD
#include <builder.h>
BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) { ... }
#endif
```

Adding `SetBuilderOptions` to a dedicated build file is recommended as it better supports incremental building.

See [`include/builder.h`](include/builder.h) for the full API reference.

## Custom Command Line Arguments

Any unrecognised flags are forwarded to your build script via `CommandLineArgs`:

```cpp
/* build.cpp */
BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	bool release = HasCommandLineArg( args, "--release" );

	options->forceRebuild = HasCommandLineArg( args, "--clean" );

	// --jobs=8  →  "8"
	const char *jobs = GetCommandLineArgValue( args, "--jobs" );
}
```

```
builder build.cpp --release --jobs=8 --clean
```

Run `builder -h` for built-in flags.

## Multiple Configs

```cpp
/* build.cpp */
BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	BuildConfig appConfig = {
		.name         = "app",
		.binaryName   = "my-program",
		.binaryFolder = "bin",
		.sourceFiles  = { "src/**/*.cpp" },
	};

	BuildConfig testsConfig = {
		.name         = "tests",
		.binaryName   = "my-program-tests",
		.binaryFolder = "bin",
		.sourceFiles  = { "src/**/*.cpp", "tests/**/*.cpp" },
		.defines      = { "TESTS_ENABLED" },
	};

	AddBuildConfig( options, &appConfig );
	AddBuildConfig( options, &testsConfig );
}
```

```
builder build.cpp --config=app
builder build.cpp --config=tests
```

If two `BuildConfig`s have the same name, Builder will fail to do the build.  All `BuildConfig`s MUST have unique names.

## Building Libraries

Set `binaryType` on a `BuildConfig`:

```cpp
BuildConfig myLib = {
	.name       = "my-lib",
	.binaryType = BINARY_TYPE_STATIC_LIBRARY,  // or BINARY_TYPE_DYNAMIC_LIBRARY
	// ...
};
```

Use `dependsOn` to express build order. Dependencies are built first and registered automatically - only call `AddBuildConfig` on the top-level config:

```cpp
BuildConfig program = {
	.name      = "program",
	.dependsOn = { myLib },
	// ...
};

AddBuildConfig( options, &program );  // also registers myLib
```

## Extra Build Steps

`OnPreBuild` and `OnPostBuild` let you run custom build steps such as copying files and codegen. These are available at two scopes:
*  Export level (`BUILDER_CALLBACK`): runs before/after the entire build and is exported like `SetBuilderOptions`.
* `BuildConfig` level: only runs before/after that specific config builds.

```cpp
#include <builder.h>
#include <stdio.h>
#include <time.h>

// this happens before ANY build step
BUILDER_CALLBACK void OnPreBuild() {}

// this happens after EVERY build step
BUILDER_CALLBACK void OnPostBuild() {}

static void PreBuild() {
    FILE *buildInfoHeader = fopen( "src/generated/build_info.h", "w" );
    fprintf( buildInfoHeader, "#pragma once\n" );
    fprintf( buildInfoHeader, "#define BUILD_TIMESTAMP %lldLL\n", (long long) time( NULL ) );
    fclose( buildInfoHeader );
}

static void PostBuild() {
#ifdef _WIN32
    system( "copy bin\\engine.dll bin\\game\\" );
#else
    system( "cp bin/engine.dll bin/game/" );
#endif
}

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
    BuildConfig config = {
        .binaryName   = "game",
        .binaryFolder = "bin/game",
        .sourceFiles  = { "src/**/*.cpp" },
        .OnPreBuild   = PreBuild,
        .OnPostBuild  = PostBuild,
    };

    AddBuildConfig( options, &config );
}
```
In this example, the flow would be `OnPreBuild` -> `PreBuild` -> Config Builds -> `PostBuild` -> `OnPostBuild`.

## Choosing a Compiler

By default Builder uses its bundled Clang install. Override it in your build script:

```cpp
options->compilerPath    = "C:/path/to/gcc";
options->compilerVersion = "15.1.0";  // optional - warns on mismatch
```

### MSVC

Set `compilerPath` to `"cl"` and Builder will locate the MSVC toolchain and Windows SDK automatically. A hard-coded path works too but requires you to manage SDK paths yourself.

## Visual Studio and Rider

```cpp
BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	BuildConfig config = {
		.binaryName   = "my-game",
		.binaryFolder = "bin",
		.sourceFiles  = { "src/**/*.cpp" },
		// ...
	};

	AddBuildConfig( options, &config );

	// pass --sln to generate; skips compilation
	options->generateSolution = HasCommandLineArg( args, "--sln" );
	options->solution = {
		.name      = "my-game",
		.path      = "visual_studio",
		.platforms = { "x64" },
		.projects  = {
			{
				.name    = "my-game",
				.configs = {
					{ "debug",   config, {             }, {} },
					{ "release", config, { "--release" }, {} },
				},
			},
		},
	};
}
```

Generated projects call Builder - Visual Studio project property edits have no effect. Re-run Builder to update them. Generated solutions also open in JetBrains Rider.

## VS Code

```cpp
// pass --vscode to generate; skips compilation
options->generateVSCodeJSONFiles = HasCommandLineArg( args, "--vscode" );
options->vsCodeJSONOptions = {
	.builderPath   = "builder",
	.taskConfigs   = {
		{ debugConfig                   },
		{ releaseConfig, { "--release" } },
	},
	.launchConfigs = {
		{ .binaryName = "bin/debug/my-program",   .debuggerType = VSCODE_DEBUGGER_TYPE_CPPVSDBG   },
		{ .binaryName = "bin/release/my-program", .debuggerType = VSCODE_DEBUGGER_TYPE_CPPDBG_GDB },
	},
};
```

Generates `.vscode/tasks.json` and `.vscode/launch.json`. `builderPath` defaults to `"builder"`, assuming it is on your `PATH`.

## Zed

```cpp
// pass --zed to generate; skips compilation
options->generateZedJSONFiles = HasCommandLineArg( args, "--zed" );
options->zedJSONOptions = {
	.builderPath  = "builder",
	.taskConfigs  = {
		{ debugConfig                   },
		{ releaseConfig, { "--release" } },
	},
	.debugConfigs = {
		{ .binaryName = "bin/debug/my-program"   },
		{ .binaryName = "bin/release/my-program" },
	},
};
```

Generates `.zed/tasks.json` and `.zed/debug.json`.

## Compilation Database

```cpp
options->generateCompilationDatabase = true;
```

Generates `compile_commands.json` on a successful build. Compatible with clangd, CLion, VS Code, and other tools that support the [JSON Compilation Database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) format.


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
* [Ed Owen](https://github.com/eddyowen) (Compilation database support, QoL improvements)
* Yann Richeux (Bug fixes)
* Tom Whitcombe (Visual Studio project generation)
* Mike Young (Linux port)
