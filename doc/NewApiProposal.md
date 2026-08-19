# API Proposal: Builder-owned BuildConfigs + list-setter functions

Status: discussion draft, following a chat with Dan about friction in current build scripts.

## The case

Today a `BuildConfig` is a stack struct full of `const char **` pointing at compound literals. That single decision is the root of everything below.

### 1. The array fields dangle

`AddBuildConfig()` registers a config with `memcpy( dst, config, sizeof( BuildConfig ) )` (builder.h:828). The copy is shallow, so every `const char **` in it still points at the caller's stack.

The trap is that the API looks like a handoff. You pass `BuilderOptions *` into a function, build a config there, register it — exactly what the signature invites — and it dies:

```c
void AddDebugConfig( BuilderOptions *options ) {
	BuildConfig config = {
		.binaryName  = "game",
		.sourceFiles = (const char *[]) { "src/main.c", NULL },
	};

	AddBuildConfig( options, &config );	// looks like Builder owns it now
}	// it doesn't - sourceFiles just died with this frame

// ...
Build( options );	// reads a dangling pointer
```

Nothing here is misuse. `AddBuildConfig()` takes the options, takes the config, copies it in — every signal says ownership transferred. Only the struct came along; everything it pointed at stayed on the caller's stack. The failure shows up later, somewhere else, as garbage source paths or a crash inside `Build()`.

**In C++ it's worse.** A compound literal in C99 has automatic storage matching the enclosing block, so above it at least survives to the closing brace. In C++ it's a *temporary* — storage ends at the end of the full expression, so `config.sourceFiles` is already dangling on the next line, before `AddBuildConfig()` is even reached. Every array literal in every build script is broken by construction when compiled as C++; we only get away with it today because we build them as C.

Removing the arrays removes the entire failure class, and C++ build scripts start working for free.

### 2. Lists can't be composed

`const char **` can only be assigned wholesale, so varying one entry means restating the whole list on both sides of a branch. This is the friction Dan actually hit:

```c
if ( HasCommandLineArg( options, "--debug" ) ) {
	config.defines = (const char *[]) { "DEBUG", "LOGGING", NULL };
} else {
	config.defines = (const char *[]) { "RELEASE", "LOGGING", NULL };
}
```

The shared entries get duplicated, and they drift. Worse, for lists that vary by platform the `#if` has to live *inside* the array literal — which is how the `additionalLibs` bug in `test_build_sdl3/build.c` got in: `":" BINARY_NAME ".so"` is GNU `ld` syntax that only works on Linux, sitting in a list with no clean place to branch.

It also rules out shared helpers. An `ApplyCommonSettings( config )` that layers a few defines and includes onto a config it didn't create can't exist, because there's no way to add to a list without owning all of it.

### 3. Boilerplate and sharp edges

Every script repeats the same `main()` shell and the same `NULL`-terminated literals. The terminators are load-bearing: forget one and Builder walks off the end of the array silently, with no diagnostic.

Config de-duplication is unreliable too. `AddBuildConfigInternal()` dedupes by `name` and skips anything unnamed (builder.h:811) — and `name` is just an optional struct field, so unnamed configs silently never dedupe.

## Proposal

### Builder owns the config

```c
BuildConfig *CreateBuildConfig( BuilderOptions *options, const char *name, BinaryType binaryType );
```

Allocates out of Builder's arena and registers with `options` in one step. Nothing user-owned is ever pointed at, so there is no lifetime to get wrong.

`name` is required rather than an optional field, and it gets copied into the arena on the way in — a caller can build a name at runtime just as easily as passing a literal, and storing their pointer would be the same bug in miniature.

`binaryType` is taken up front too. It's the one scalar that decides what the config fundamentally *is* — which linker path runs, whether an import lib gets produced — so it belongs with the name rather than being patched in afterwards. It costs the simple case an explicit `BINARY_TYPE_EXE` where the zero-initialised default used to cover it, which seems a fair trade for the two identity-defining properties sitting in one place. There's no `SetBinaryType()` as a result; one way to set it.

This replaces `AddBuildConfig()` outright rather than sitting alongside it — creation *is* registration, so there's no second call to forget. `options->configs` also stops being an array of copied structs; it becomes a list of pointers, and the realloc-and-`memcpy` in `AddBuildConfigInternal()` disappears with it.

Cycle detection has to move to the top of `Build()`. This isn't a concession, it's forced: a config has no dependencies at all at `CreateBuildConfig()` time — they're attached afterwards by `AddDependency()` — so there is no point during construction at which the graph is complete enough to check. `Build()` is the first moment it is. Same error, same exit, reported a moment later.

### Setters replace the array fields

```c
void AddSourceFiles( BuildConfig *config, const char *files );      // "main.c, test.c"
void AddDefines( BuildConfig *config, const char *defines );        // "DEBUG, LOGGING" - no -D, Builder adds it
void AddIncludes( BuildConfig *config, const char *paths );
void AddLibPaths( BuildConfig *config, const char *paths );
void AddLibs( BuildConfig *config, const char *libs );
void AddWarningLevels( BuildConfig *config, const char *levels );
void AddIgnoreWarnings( BuildConfig *config, const char *warnings );
void AddLinkerArguments( BuildConfig *config, const char *args );
void AddDependency( BuildConfig *config, BuildConfig *dependency );
```

Each takes a single `const char *`, splits it, and pushes the tokens into the chunked `builderStringList_t` we already use for glob results — the storage and the walk code exist, they just get exposed. A string with no separator in it is just the one-token case, so `AddDefines( config, "LOGGING" )` is as valid as the multi-entry form.

**Each token has to be copied into the arena, not pointed at.** `Builder_StringListPush()` currently stores the pointer it's handed (builder.h:1290), which is safe today only because every entry is a string literal. Once the argument is a parsed string, a caller can reasonably pass a buffer:

```c
char buf[64];
snprintf( buf, sizeof( buf ), "VERSION=%d", version );
AddDefines( config, buf );	// buf is gone at the end of this scope
```

Storing pointers into that buffer would reintroduce the exact bug this proposal exists to remove, one level further down. Splitting in place isn't an option either — the parameter is `const char *` and most callers pass literals. So the parser duplicates each token into the arena as it goes, and the config owns its strings outright.

**The scalars get setters too:**

```c
void SetBinaryName( BuildConfig *config, const char *name );
void SetBinaryFolder( BuildConfig *config, const char *folder );
void SetIntermediateFolder( BuildConfig *config, const char *folder );
void SetLanguageVersion( BuildConfig *config, LanguageVersion version );
void SetOptimization( BuildConfig *config, Optimization optimization );
void SetRemoveSymbols( BuildConfig *config, bool removeSymbols );
void SetWarningsAsErrors( BuildConfig *config, bool warningsAsErrors );
void SetPreBuildCallback( BuildConfig *config, void ( *callback )( BuildConfig * ) );
void SetPostBuildCallback( BuildConfig *config, void ( *callback )( BuildConfig * ) );
```

The three string ones aren't ceremony — they carry the same arena-duplication requirement as the list tokens above. `binaryName`, `binaryFolder` and `intermediateFolder` are all `const char *`, and a caller can hand us a `snprintf`'d buffer as easily as a literal, so the copy has to happen on the way in.

The enums, bools and callbacks have no such need; they're single value writes. They get setters anyway for consistency — a config assembled from a dozen `Add*`/`Set*` calls with one raw `config->warningsAsErrors = true` in the middle reads like something got missed.

**Split on commas.** This keeps some of the sugar we have today. One call can carry several entries, so `AddSourceFiles( config, "src/main.c, src/test1.c, src/test2.c" )` reads much like the array literal it replaces, instead of forcing a call per file. Commas are also safe for the content we're holding — paths and defines can contain almost anything, but not a comma.

`AddDependency()` takes one pointer at a time — nothing to split, and the pointer is already Builder-owned memory, so it's just bookkeeping.

Because `Add*` is additive, problem 2 dissolves:

```c
AddDefines( config, "LOGGING" );
AddDefines( config, HasCommandLineArg( options, "--debug" ) ? "DEBUG" : "RELEASE" );
```

and platform lists stop needing preprocessor directives inside data:

```c
#if defined( _WIN32 )
	AddLibs( config, "SDL" );
#elif defined( __linux__ )
	AddLibs( config, ":SDL.so" );
#endif
```

Note the naming drops the `additional` prefix (`additionalIncludes` → `AddIncludes`); the `Add` verb already carries it. The prefix was inherited VS terminology — "additional" there means `/I` on top of the `INCLUDE` env var — and we already handle the SDK paths separately (builder.h:2400), so there's no non-additional list for a user to confuse it with.

Leaving `AddIncludes()` unqualified also keeps `AddSystemIncludes()` free, should we ever want `-isystem` / `/external:I`. That's a separate list rather than a flavour of this one, and it'd be genuinely useful for vendored deps like SDL where you want `warningsAsErrors` on your code but not theirs.

### `main()` can move into the header

Users declare a callback and nothing else:

```c
void BuildScript( BuilderOptions *options ) {
	// configs go here
}
```

`main()`, `Builder_RebuildSelf()` and the `Build()` call all live inside `BUILDER_IMPLEMENTATION`, so the script is nothing but the configs. Independent of the memory fix — fold in or drop on its own merits.

(`HasCommandLineArg()` taking `BuilderOptions *` is incidental to the examples above. Command line handling probably wants its own `CommandLineArgs` struct, but that's a separate change.)

### `BuildConfig` becomes opaque

The public header gets `typedef struct BuildConfig BuildConfig;` and the definition stays inside `BUILDER_IMPLEMENTATION`.

**The reason is header surface, not enforcement.** Once the list fields are chunked arrays, a publicly-defined `BuildConfig` drags `builderStringList_t`, `builderStringChunk_t` and `STRING_CHUNK_SIZE` up into the public section with it — they'd have to be promoted purely so the struct definition compiles, putting internal machinery in front of users who have no use for it. Keeping the definition down in the implementation block means none of that moves. It's the smaller change as well as the tidier one.

It costs nothing on top of what's already proposed, since every field has a setter regardless.

What it does *not* buy is a guarantee. Build scripts `#define BUILDER_IMPLEMENTATION` before including the header, so the definition is in their translation unit anyway and anyone determined can still reach in. That's fine — it isn't what we're paying for.

The structural guarantee comes from killing `AddBuildConfig()` instead. Once `CreateBuildConfig()` is the only way a config gets registered with `options`, a stack-allocated `BuildConfig` never enters the system — there's no function that will accept it. The one remaining hole is `AddDependency( config, &someStackConfig )`.

That hole is cheaply closable: `AddDependency()` can assert the incoming pointer falls within the bounds of Builder's arena. Arenas know their own extents, so it's a couple of comparisons, and it converts the last piece of latent UB into a clear error message at the call site that caused it.

## Before / after

**`test_build_sdl3/build.c`** — the case that hurts most today. Defines, abridged:

```c
// before
.defines = (const char *[]) {
	"DLL_EXPORT",
#ifdef _WIN32
	"SDL_PLATFORM_WIN32",
	"HAVE_MODF",
#elif defined( __linux__ )
	"HAVE_LIBC",
	"HAVE_STDARG_H",
	"HAVE_STDDEF_H",
	/* ...16 more... */
#endif
	NULL
},

// after
AddDefines( sdl, "DLL_EXPORT" );
#ifdef _WIN32
	AddDefines( sdl, "SDL_PLATFORM_WIN32, HAVE_MODF" );
#elif defined( __linux__ )
	AddDefines( sdl, "HAVE_LIBC, HAVE_STDARG_H, HAVE_STDDEF_H, HAVE_STDINT_H, HAVE_FLOAT_H,"
	                 "HAVE_LIMITS_H, HAVE_MATH_H, HAVE_SIGNAL_H, HAVE_STDIO_H, HAVE_STDLIB_H,"
	                 "HAVE_STRING_H, HAVE_STRINGS_H, HAVE_SYS_TYPES_H, HAVE_WCHAR_H,"
	                 "HAVE_INTTYPES_H, HAVE_MALLOC_H, HAVE_MEMORY_H, HAVE_ALLOCA_H" );
#endif
```

The 100-line `sourceFiles` array collapses the same way, and the `#if` blocks move from inside data to around statements.

**`test_build_static_lib/build.c`** — dependencies and platform-specific link args:

```c
// before
BuildConfig libConfig = {
	.name        = "lib",
	.binaryName  = "test_static_lib",
	.binaryType  = BINARY_TYPE_STATIC_LIBRARY,
	.sourceFiles = (const char *[]) { "lib/mathlib.c", NULL },
};

BuildConfig programConfig = {
	.name        = "program",
	.binaryName  = "test_static_lib_program",
	.dependsOn   = (BuildConfig *[]) { &libConfig, NULL },
	.sourceFiles = (const char *[]) { "program/main.c", NULL },
	.additionalIncludes = (const char *[]) { "lib", NULL },
	.additionalLinkerArguments = (const char *[]) {
#if defined( _WIN32 )
		"test_static_lib.lib",
#else
		"./test_static_lib.a",
#endif
		NULL
	},
};

options.defaultConfig = &programConfig;
AddBuildConfig( &options, &programConfig );

// after
BuildConfig *libConfig = CreateBuildConfig( options, "lib", BINARY_TYPE_STATIC_LIBRARY );
SetBinaryName( libConfig, "test_static_lib" );
AddSourceFiles( libConfig, "lib/mathlib.c" );

BuildConfig *programConfig = CreateBuildConfig( options, "program", BINARY_TYPE_EXE );
SetBinaryName( programConfig, "test_static_lib_program" );
AddDependency( programConfig, libConfig );
AddSourceFiles( programConfig, "program/main.c" );
AddIncludes( programConfig, "lib" );
#if defined( _WIN32 )
	AddLinkerArguments( programConfig, "test_static_lib.lib" );
#else
	AddLinkerArguments( programConfig, "./test_static_lib.a" );
#endif

options->defaultConfig = programConfig;
```

Similar length — but `&libConfig` is no longer a stack address stored in another struct, and there's no registration step to forget.

## Costs

**Cloning from a base config isnt as simple as copy assignment** Two different things both get called "composition", and they move in opposite directions:

- *Layering onto a config* — `ApplyCommonSettings( config )` adding defines and includes to a config the caller owns. Impossible today, easy after. Straight win.
- *Cloning a template* — building a `base` config and deriving `debug` and `release` from it. Today `BuildConfig derived = base;` is a struct copy, and sharing the array pointers is harmless because they point at immutable literals nobody appends to. After the change, the lists are owned chunked arrays in the arena, so a copy needs a real deep copy — a `CopyConfig()` that walks and clones every list. That's exactly the machinery this proposal exists to delete.

The way out is to not clone. Express the shared part as a function and call it per config:

```c
BuildConfig *debug   = CreateBuildConfig( options, "debug", BINARY_TYPE_EXE );
BuildConfig *release = CreateBuildConfig( options, "release", BINARY_TYPE_EXE );

ApplyCommonSettings( debug );
ApplyCommonSettings( release );

AddDefines( debug, "DEBUG" );
AddDefines( release, "RELEASE" );
```

Called twice instead of copied once. Worth confirming with Dan that this covers his real cases before we commit — if something genuinely needs clone semantics, `CopyConfig()` comes back and takes some of the shine off.

**Everything else:**

- Breaking change. Every build script and all seven tests rewrite. No deprecation path worth having for a header-only library at this version.
- Cycle errors surface at `Build()` rather than at construction (forced, see above).
- Configs read as a run of `Add*`/`Set*` statements instead of one designated initializer.
