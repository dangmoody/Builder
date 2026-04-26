
#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
  options->linkAgainstWindowsDynamicRuntime = true;
}

#else

// _mbsdup is exported from ucrtbase.dll for backwards compatibility but was dropped from the
// static runtime libs (libucrt.lib / libcmt.lib).  Unlike __imp_malloc or __imp___acrt_iob_func,
// there is no static fallback for the linker to silently resolve against, so this produces a
// hard LNK2001 error (not just a LNK4286 warning) if the static runtime is used.
#pragma comment(linker, "/include:__imp__mbsdup")

#include <iostream>

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

  std::cout << "Dynamic runtime required." << std::endl;

  return 0;
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD