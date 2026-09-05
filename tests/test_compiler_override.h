#pragma once

static void ApplyCompilerOverride( BuilderOptions *options, int argc, char **argv ) {
	if ( HasCommandLineArg( argc, argv, "--clang" ) ) {
		options->compilerPath = "../../tools/clang/bin/clang";
		options->compilerVersion = "20.1.5";
	} else if ( HasCommandLineArg( argc, argv, "--clang-cl" ) ) {
		options->compilerPath = "../../tools/clang/bin/clang-cl";
		options->compilerVersion = "20.1.5";
	} else if ( HasCommandLineArg( argc, argv, "--gcc" ) ) {
#if defined( _WIN32 )
		options->compilerPath = "../../tools/gcc/bin/gcc";
		options->compilerVersion = "15.1.0";
#elif defined( __linux__ )
		options->compilerPath = "gcc";
#endif
	} else if ( HasCommandLineArg( argc, argv, "--msvc" ) ) {
		options->compilerPath = "cl";
		options->compilerVersion = "14.44.35207";
	} else if ( HasCommandLineArg( argc, argv, "--clang++" ) ) {
		options->compilerPath = "../../tools/clang/bin/clang++";
		options->compilerVersion = "20.1.5";
	} else if ( HasCommandLineArg( argc, argv, "--g++" ) ) {
#if defined( _WIN32 )
		options->compilerPath = "../../tools/gcc/bin/g++";
		options->compilerVersion = "15.1.0";
#elif defined( __linux__ )
		options->compilerPath = "g++";
#endif
	}
}
