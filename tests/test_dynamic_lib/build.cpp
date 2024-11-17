#include <builder.h>

// this is just for the sake of the example
// your own codebase probably has its own implementation of this
#ifdef _WIN64
#include <Windows.h>
static void copy_file( const char* from, const char* to ) {
	CopyFileA( from, to, FALSE );
}
#else
#error Platform not recognised.
#endif

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig library = {
		.binary_folder	= "bin",
		.binary_name	= "test_dynamic_lib",
		.binary_type	= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files	= { "lib/lib.cpp" },
		.defines		= { "DYNAMIC_LIBRARY_EXPORTS" },
	};

	BuildConfig program = {
		.depends_on				= { library },
		.binary_folder			= "bin",
		.binary_name			= "test_dynamic_library_program",
		.source_files			= { "program/program.cpp" },
		.additional_includes	= { "lib" },
		.additional_lib_paths	= { "bin" },
		.additional_libs		= { "test_dynamic_lib.lib" },
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	add_build_config( options, &program );
}