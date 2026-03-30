#pragma once

static void ApplyCompilerOverride( BuilderOptions *options, CommandLineArgs *args ) {
	bool clang = HasCommandLineArg( args, "--clang" );
	bool gcc = HasCommandLineArg( args, "--gcc" );
	bool msvc = HasCommandLineArg( args, "--msvc" );

	if ( clang ) {
		options->compilerPath = "../../clang/bin/clang";
		options->compilerVersion = "20.1.5";
	} else if ( gcc ) {
		options->compilerPath = "../../tools/gcc/bin/gcc";
		options->compilerVersion = "15.1.0";
	} else if ( msvc ) {
		options->compilerPath = "cl";
		options->compilerVersion = "14.44.35207";
	}
}
