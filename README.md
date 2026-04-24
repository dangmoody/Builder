# Builder

by Dan Moody.

Configure C/C++ builds by writing C++ instead of learning a separate build language. **Builder is not a compiler** - it turns your build config into compiler arguments and calls the compiler.

Supports Clang, MSVC, and GCC on Windows and Linux.

## Installation

1. Download the latest release.
2. Extract the archive somewhere.
3. **Optional:** Add Builder to your `PATH`.

## Quick Start

Point Builder at any C/C++ source file and it will compile it:

```
builder main.cpp
```

Builder uses its bundled Clang, outputs the binary in the same folder, and names it after the source file. No config required.

### Configuring your build

Add `SetBuilderOptions` to your source file for more options.

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

Any problems add -v or --verbose calling builder.

### Separating build code from program code

`BUILDER_DOING_USER_CONFIG_BUILD` is defined when Builder is compiling your source file into a config DLL. Use it to keep build code out of your program, or put `SetBuilderOptions` in a dedicated build file entirely:

```cpp
#if BUILDER_DOING_USER_CONFIG_BUILD
#include <builder.h>
BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) { ... }
#endif
```

Adding `SetBuilderOptions` to a dedicated build file is recommended as it better supports incremental building.

See [`include/builder.h`](include/builder.h) or [`BuilderAPI.md`](BuilderAPI.md) for the full API reference.

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

`OnPreBuild` and `OnPostBuild` within `BuildConfig` are called just before and after compilation - useful for copying files, running codegen, etc.

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

C++ build systems require learning a separate language (CMake, Makefiles, Ninja, MSBuild). Builder skips that entirely: your build config is just C++ code. Less friction, no new syntax, same language you're already writing.

## Contributing

Contributions are welcomed and much appreciated.
Please see [Contributing.md](doc/Contributing.md).


## Credits

Builder would not have been possible without the following people who deserve, at the very least, a special thanks:

* Dale Green
* [Ed Owen](https://github.com/eddyowen) (Compilation database support, QoL improvements)
* Yann Richeux (Bug fixes)
* Tom Whitcombe (Visual Studio project generation)
* Mike Young (Linux port)
