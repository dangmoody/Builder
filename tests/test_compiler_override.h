#pragma once

static void ApplyCompilerOverride( BuilderOptions *options, CommandLineArgs *args ) {
	bool clang = HasCommandLineArg( args, "--clang" );
	bool gcc = HasCommandLineArg( args, "--gcc" );
	bool msvc = HasCommandLineArg( args, "--msvc" );
	const char *msvcFullPath = GetCommandLineArgValue( args, "--msvc-full-path" );

	if ( clang ) {
		options->compilerPath = "../../clang/bin/clang";
		options->compilerVersion = "20.1.5";
	} else if ( gcc ) {
#if defined( _WIN32 )
		options->compilerPath = "../../tools/gcc/bin/gcc";
		options->compilerVersion = "15.1.0";
#elif defined( __linux__ )
		options->compilerPath = "gcc";
#endif
	} else if ( msvc ) {
		options->compilerPath = "cl";
		options->compilerVersion = "14.44.35207";
	} else if ( msvcFullPath ) {
		options->compilerPath = msvcFullPath;
		options->compilerVersion = "14.44.35207";
	}
}
