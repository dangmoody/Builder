# Builder

Distributed under the [MIT License](LICENSE).

## Intro

Builder is a single header file that you can use to build your C/C++ programs through C/C++ code.

Builder is not a compiler.  Builder turns BuildConfigs into compiler and linker arguments, then calls the compiler that you want to use, and then calls the linker.

It works on Windows (supporting Clang, GCC, and MSVC) and Linux (Clang and GCC).

## Installation

1. Download the latest release.
2. Put the header file(s) in your project.
3. ?????
4. Profit!

## Quick Start Guide

You'll need a build script, which is just C/C++ code:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.name			= "self",
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	return Build( &options, argc, argv );
}
```

For the first ever build you'll need to invoke your compiler manually:

```
clang -o build.exe build.c	# Windows
clang -o build build.c		# Linux
```

After that, once your `build.exe` is built you can just call it and, if it needs to, it will rebuild itself (if `BuilderOptions::selfRebuildConfig` is set):

```
build.exe
```

Builder sets the working directory to the folder containing your build executable, so all relative paths in your build script are relative to that.

## Multiple Build Configs

If you have multiple BuildConfigs, you must tell Builder which one you want to build with, specifically.

You can do this by passing `--config=<name>` at the command line where `<name>` is the name of the BuildConfig, specified via `BuildConfig::name`.

Using the above example, if you wanted to build `"my_awesome_program"` instead of the other configs you'd need to pass at the command line:

```
build.exe --config=my_awesome_program
```

If you have multiple BuildConfigs but don't specify which one to build at the command line, Builder will error asking you to tell it which one you want to build.

You can also use `BuilderOptions::defaultConfig` to tell Builder to build a specific BuildConfig by default:

```c
options.defaultConfig = config;
```

## BuildConfig Dependencies

BuildConfigs can be built before/after other BuildConfigs through explicit ordering via `BuildConfig::dependsOn`:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *mathlib = CreateBuildConfig( &options );
	*mathlib = (BuildConfig) {
		.name			= "mathlib",
		.binaryName		= "mathlib",	// the file extension is automatically appended for you
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.binaryFolder	= "bin",
		.sourceFiles	= MakeStringList( "src/mathlib/lib.c" ),
	};

	BuildConfig *app = CreateBuildConfig( &options );
	*app = (BuildConfig) {
		.name				= "app",
		.binaryFolder		= "bin",
		.binaryName			= "app",
		.dependsOn			= MakeDependencies( mathlib ),
		.sourceFiles		= MakeStringList( "src/app/program.c" ),
		.additionalIncludes	= MakeStringList( "src/mathlib" ),
		.additionalLibPaths	= MakeStringList( "bin" ),
		.additionalLibs		= MakeStringList( "mathlib" ),
		.binaryType			= BINARY_TYPE_EXE,
	};

	return Build( &options, argc, argv );
}
```

You can then build the `"app"` config through the command line like normal:

```
build.exe --config=app
```

Building `"app"` builds `"mathlib"` first, since it's listed in `dependsOn`.  You only ever need to tell Builder to build the top-level config.

`dependsOn` only controls build order, it does not link anything for you.  `"app"` links against `"mathlib"` because it lists it in `additionalLibs` and `additionalLibPaths`.

## Custom Command Line Arguments

Builder allows you to create your own command line arguments for your builds.

You can use the `HasCommandLineArg` function to check if the argument was passed at the command line:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	if ( HasCommandLineArg( argc, argv, "--release" ) ) {
		config->optimization = OPTIMIZATION_PROGRAM_SPEED;
	}

	return Build( &options, argc, argv );
}
```

You can then pass that command line argument through as normal:

```
build.exe --release
```

## Choosing a Compiler

By default, Builder will generate compiler arguments for Clang.

If you want to use a different compiler you can do this via `BuilderOptions::compilerPath` and `BuilderOptions::compilerVersion`:

```c
options.compilerPath = "C:/path/to/gcc";
options.compilerVersion = "15.1.0";	// this one is optional and warns you on a mismatch
```

For MSVC it's recommended you just set your compiler path to `"cl"` and Builder will locate the MSVC toolchain and Windows SDK automatically.

## Visual Studio

Builder can be used to generate Visual Studio Solutions (`.sln` and `.vcxproj`).  These are also compatible with Rider:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

#define BUILDER_VISUAL_STUDIO_IMPLEMENTATION
#include "builder_visual_studio.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--sln" ) ) {
		VisualStudioConfig vsConfigs[] = {
			{ .name = "Debug",   .config = config },
			{ .name = "Release", .config = config, .additionalBuildArgs = MakeStringList( "--release" ) },
		};

		VisualStudioProject vsProjects[] = {
			{
				.name			= "my_awesome_program",
				.configs		= vsConfigs,
				.configsCount	= BUILDER_COUNT_OF( vsConfigs ),
			},
		};

		VisualStudioSolution solution = {
			.name			= "my_awesome_program",
			.platforms		= MakeStringList( "x64" ),
			.projects		= vsProjects,
			.projectsCount	= BUILDER_COUNT_OF( vsProjects ),
		};

		return Builder_GenerateVisualStudioSolution( &options, &solution, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
```

You'd then generate the solution with:

```
build.exe --sln
```

Generated projects will call to your `build.exe`.

## VS Code

Builder can be used to generate VS Code's `c_cpp_properties.json`, `tasks.json`, and `launch.json` files:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

#define BUILDER_VS_CODE_IMPLEMENTATION
#include "builder_vs_code.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--vscode" ) ) {
		VSCodeCppPropertiesConfig cppPropertiesConfigs[] = {
			{ .config = config, .intelliSenseMode = VSCODE_INTELLISENSE_MODE_LINUX_CLANG_X64 },
		};

		VSCodeTaskConfig taskConfigs[] = {
			{ .config = config },
		};

		VSCodeLaunchConfig launchConfigs[] = {
			{ .binaryName = "bin/my_awesome_program", .debuggerType = VSCODE_DEBUGGER_TYPE_CPPDBG_GDB },
		};

		VSCodeJSONOptions vsCodeOptions = {
			.cppPropertiesConfigs		= cppPropertiesConfigs,
			.cppPropertiesConfigsCount	= BUILDER_COUNT_OF( cppPropertiesConfigs ),
			.taskConfigs				= taskConfigs,
			.taskConfigsCount			= BUILDER_COUNT_OF( taskConfigs ),
			.launchConfigs				= launchConfigs,
			.launchConfigsCount			= BUILDER_COUNT_OF( launchConfigs ),
		};

		return Builder_GenerateVSCodeJSONFiles( &options, &vsCodeOptions, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
```

You'd then generate the JSON files with:

```
build.exe --vscode
```

By default, if you call `Builder_GenerateVSCodeJSONFiles()` without filling in any other settings Builder will generate a JSON entry for each BuildConfig you created.

## Zed

Builder can be used to generate Zed's `tasks.json` and `debug.json` files:

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

#define BUILDER_ZED_IMPLEMENTATION
#include "builder_zed.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--zed" ) ) {
		ZedTaskConfig taskConfigs[] = {
			{ .config = config },
		};

		ZedDebugConfig debugConfigs[] = {
			{
				.label		= "Debug my_awesome_program",
				.binaryName	= "bin/my_awesome_program",
				.adapter	= ZED_DEBUGGER_ADAPTER_CODELLDB,
				.request	= ZED_DEBUGGER_REQUEST_LAUNCH,
			},
		};

		ZedJSONOptions zedOptions = {
			.taskConfigs		= taskConfigs,
			.taskConfigsCount	= BUILDER_COUNT_OF( taskConfigs ),
			.debugConfigs		= debugConfigs,
			.debugConfigsCount	= BUILDER_COUNT_OF( debugConfigs ),
		};

		return Builder_GenerateZedJSONFiles( &options, &zedOptions, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
```

You'd then generate the JSON files with:

```
build.exe --zed
```

By default, if you call `Builder_GenerateZedJSONFiles()` without filling in any other settings Builder will generate a JSON entry for each BuildConfig you created.

## Compilation Database

Builder can be used to generate a `compile_commands.json` file (a JSON Compilation Database).

Clangd, CLion, VS Code, and anything else that supports the format can use it for code completion and navigation:
[https://clang.llvm.org/docs/JSONCompilationDatabase.html](https://clang.llvm.org/docs/JSONCompilationDatabase.html)

```c
#define BUILDER_IMPLEMENTATION
#include "builder.h"

#define BUILDER_COMPILATION_DATABASE_IMPLEMENTATION
#include "builder_compilation_database.h"

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	BuildConfig *config = CreateBuildConfig( &options );
	*config = (BuildConfig) {
		.name			= "my_awesome_program",
		.sourceFiles	= MakeStringList( "src/my_code.c" ),
	};

	options.defaultConfig = config;

	if ( HasCommandLineArg( argc, argv, "--compile-commands" ) ) {
		CompilationDatabaseOptions compilationDatabaseOptions = { 0 };

		return Builder_GenerateCompilationDatabase( &options, &compilationDatabaseOptions, argc, argv ) ? 0 : 1;
	}

	return Build( &options, argc, argv );
}
```

You'd then generate the `compile_commands.json` file with:

```
build.exe --compile-commands
```

By default, if you call `Builder_GenerateCompilationDatabase()` without filling in any other settings Builder will generate an entry for every source file of every BuildConfig you created.

Nothing gets compiled - the commands are the same ones Builder would run, minus the output file.

## Motivation

C/C++ has no standard build system, so at some point every C/C++ programmer has to pick one and every option asks the same thing of you: learn a new language.  CMake has its own DSL, Makefiles have their own syntax and rules, Premake uses Lua, Meson uses Python.  Even if you learn one well, the knowledge doesn't transfer to the next project that uses a different one and you have to learn a project's build system all over again.

You already know C/C++.  Why should configuring a C/C++ build require learning anything else?

Builder's answer is to not require it.  Your build config is just a C/C++ source file.  The types are C structs.  The logic is C/C++.  If you can write C/C++, you already know how to use Builder.

## Contributing

Yes!

Please see [Contributing.md](doc/Contributing.md).

## Credits

Builder would not have been possible without the following people who deserve, at the very least, a special thanks:

* Dale Green
* [Aiden Knight](https://github.com/aiden-knight) (File globbing, better incremental compilation, Windows dynamic runtime, and lots of other small things)
* [Ed Owen](https://github.com/eddyowen) (Compilation database support, QoL improvements)
* Yann Richeux (Bug fixes)
* Tom Whitcombe (Visual Studio project generation)
* Mike Young (Linux platform code)
