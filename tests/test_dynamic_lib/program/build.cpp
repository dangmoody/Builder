#include <builder.h>

// this is just for the sake of the example
// your own codebase probably has its own implementation of this
#ifdef _WIN64
#include <Windows.h>
static void copy_file( const char* from, const char* to ) {
	CopyFileA( from, to, FALSE );
}
#else
#error TODO(DGM): 11/10/2024: fill me in!
#endif

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig config = {
		.binary_folder			= "bin",
		.binary_name			= "test_dynamic_library_program",
		.source_files			= { "program.cpp" },
		.additional_includes	= { "../lib" },
		.additional_lib_paths	= { "../lib/bin" },
		.additional_libs		= { "test_dynamic_lib.lib" },
	};

	options->configs.push_back( config );
}

BUILDER_CALLBACK void on_pre_build() {
	copy_file( "tests\\test_dynamic_lib\\lib\\bin\\test_dynamic_lib.dll", "tests\\test_dynamic_lib\\program\\bin\\test_dynamic_lib.dll" );
}